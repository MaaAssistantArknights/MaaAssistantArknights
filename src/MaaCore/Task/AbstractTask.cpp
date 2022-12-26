#include "AbstractTask.h"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <thread>
#include <utility>

#include "Utils/Demangle.hpp"
#include "Utils/NoWarningCV.h"

#include "AbstractTaskPlugin.h"
#include "Assistant.h"
#include "Config/GeneralConfig.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

AbstractTask::AbstractTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain)
    : InstHelper(inst), m_callback(callback), m_task_chain(task_chain)
{}

bool asst::AbstractTask::run()
{
    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }
    callback(AsstMsg::SubTaskStart, basic_info());
    for (m_cur_retry = 0; m_cur_retry <= m_retry_times; ++m_cur_retry) {
        if (_run()) {
            callback(AsstMsg::SubTaskCompleted, basic_info());
            return true;
        }
        if (need_exit()) {
            return false;
        }
        int delay = Config.get_options().task_delay;
        sleep(delay);

        if (!on_run_fails()) {
            callback(AsstMsg::SubTaskError, basic_info());
            return false;
        }
    }
    callback(AsstMsg::SubTaskError, basic_info());
    return false;
}

AbstractTask& asst::AbstractTask::set_retry_times(int times) noexcept
{
    m_retry_times = times;
    return *this;
}

AbstractTask& asst::AbstractTask::set_enable(bool enable) noexcept
{
    m_enable = enable;
    return *this;
}

AbstractTask& asst::AbstractTask::set_ignore_error(bool ignore) noexcept
{
    m_ignore_error = ignore;
    return *this;
}

AbstractTask& asst::AbstractTask::set_task_id(int task_id) noexcept
{
    m_task_id = task_id;
    return *this;
}

void asst::AbstractTask::clear_plugin() noexcept
{
    m_plugins.clear();
}

json::value asst::AbstractTask::basic_info() const
{
    if (m_basic_info_cache.empty()) {
        std::string class_name = utils::demangle(typeid(*this).name());
        std::string task_name;
        // typeid.name() 结果可能和编译器有关，所以这里使用正则尽可能保证结果正确。
        // 但还是不能完全保证，如果不行的话建议 override
        std::regex regex("[a-zA-Z]+Task");
        std::smatch match_obj;
        if (std::regex_search(class_name, match_obj, regex)) {
            task_name = match_obj[0].str();
        }
        else {
            task_name = class_name;
        }

        m_basic_info_cache = json::object {
            { "taskchain", std::string(m_task_chain) },
            { "taskid", m_task_id },
            { "class", class_name },
            { "subtask", task_name },
            { "details", json::object() }
            //{ "CurRetryTimes", m_cur_retry },
            //{ "MaxRetryTimes", m_retry_times }
        };
    }
    return m_basic_info_cache;
}

json::value asst::AbstractTask::basic_info_with_what(std::string what) const
{
    json::value info = basic_info();
    info["what"] = std::move(what);
    return info;
}

void asst::AbstractTask::callback(AsstMsg msg, const json::value& detail)
{
    for (const TaskPluginPtr& plugin : m_plugins) {
        plugin->set_task_id(m_task_id);
        plugin->set_task_ptr(this);

        if (!plugin->verify(msg, detail)) {
            continue;
        }

        plugin->run();

        if (plugin->block()) {
            break;
        }
    }
    if (m_callback) {
        // TODO 屎山: task 字段需要忽略 @ 和前面的字符，否则回调大改
        if (std::string task = detail.get("details", "task", std::string()); !task.empty()) {
            if (size_t pos = task.rfind('@'); pos != std::string::npos) {
                json::value proced_detail = detail;
                proced_detail["details"]["task"] = task.substr(pos + 1);
                m_callback(msg, proced_detail, m_inst);
                return;
            }
        }
        m_callback(msg, detail, m_inst);
    }
}

void asst::AbstractTask::click_return_button()
{
    ProcessTask(*this, { "Return" }).run();
}

bool asst::AbstractTask::save_img(const std::string& dirname)
{
    auto image = ctrler()->get_image();
    if (image.empty()) {
        return false;
    }

    std::string stem = utils::get_format_time();
    stem = utils::string_replace_all(stem, { { ":", "-" }, { " ", "_" } });
    std::filesystem::create_directories(dirname);
    std::string full_path = dirname + stem + "_raw.png";
    Log.trace("Save image", full_path);
    return asst::imwrite(full_path, image);
}
