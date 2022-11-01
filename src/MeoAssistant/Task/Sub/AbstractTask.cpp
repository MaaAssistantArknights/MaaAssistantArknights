#include "AbstractTask.h"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <thread>
#include <utility>

#include "Utils/Demangle.hpp"
#include "Utils/NoWarningCV.h"

#include "../Plugin/AbstractTaskPlugin.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "Resource/GeneralConfiger.h"
#include "Utils/AsstImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

using namespace asst;

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain)
    : m_callback(std::move(callback)), m_callback_arg(callback_arg), m_task_chain(std::move(task_chain))
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
        int delay = Configer.get_options().task_delay;
        sleep(delay);

        if (!on_run_fails()) {
            callback(AsstMsg::SubTaskError, basic_info());
            return false;
        }
    }
    callback(AsstMsg::SubTaskError, basic_info());
    return false;
}

AbstractTask& asst::AbstractTask::set_exit_flag(bool* exit_flag) noexcept
{
    m_exit_flag = exit_flag;
    return *this;
}

AbstractTask& asst::AbstractTask::set_retry_times(int times) noexcept
{
    m_retry_times = times;
    return *this;
}

AbstractTask& asst::AbstractTask::set_ctrler(std::shared_ptr<Controller> ctrler) noexcept
{
    m_ctrler = std::move(ctrler);
    return *this;
}

AbstractTask& asst::AbstractTask::set_status(std::shared_ptr<RuntimeStatus> status) noexcept
{
    m_status = std::move(status);
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
            { "taskchain", m_task_chain },
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

bool AbstractTask::sleep(unsigned millisecond)
{
    if (need_exit()) {
        return false;
    }
    if (millisecond == 0) {
        std::this_thread::yield();
        return true;
    }
    auto start = std::chrono::steady_clock::now();
    Log.trace("ready to sleep", millisecond);
    auto millisecond_ms = std::chrono::milliseconds(millisecond);
    auto interval = millisecond_ms / 5;

    while (!need_exit()) {
        std::this_thread::sleep_for(interval);
        if (std::chrono::steady_clock::now() - start > millisecond_ms) {
            break;
        }
    }
    Log.trace("end of sleep", millisecond);

    return !need_exit();
}

bool asst::AbstractTask::need_exit() const
{
    return m_exit_flag != nullptr && *m_exit_flag;
}

void asst::AbstractTask::callback(AsstMsg msg, const json::value& detail)
{
    for (const TaskPluginPtr& plugin : m_plugins) {
        plugin->set_exit_flag(m_exit_flag);
        plugin->set_ctrler(m_ctrler);
        plugin->set_task_id(m_task_id);
        plugin->set_status(m_status);
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
        json::value proced_detail = detail;
        if (std::string task = detail.get("details", "task", std::string()); !task.empty()) {
            if (size_t pos = task.rfind('@'); pos != std::string::npos) {
                proced_detail["details"]["task"] = task.substr(pos + 1);
            }
        }

        m_callback(msg, proced_detail, m_callback_arg);
    }
}

void asst::AbstractTask::click_return_button()
{
    ProcessTask(*this, { "Return" }).run();
}

bool asst::AbstractTask::save_img(const std::string& dirname)
{
    auto image = m_ctrler->get_image();
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
