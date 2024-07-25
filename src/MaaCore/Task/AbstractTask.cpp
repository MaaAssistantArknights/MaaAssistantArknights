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
#include "Controller/Controller.h"
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

        if (!plugin->get_enable() || !plugin->verify(msg, detail)) {
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
                json::value proceed_detail = detail;
                proceed_detail["details"]["task"] = task.substr(pos + 1);
                m_callback(msg, proceed_detail, m_inst);
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

bool asst::AbstractTask::save_img(const std::filesystem::path& relative_dir, bool use_cache, bool auto_clean)
{
    auto image = use_cache ? ctrler()->get_image_cache() : ctrler()->get_image();
    if (image.empty()) {
        return false;
    }
    std::string stem = utils::get_time_filestem();

    if (auto_clean) {
        // 第1次或每执行 debug.clean_files_freq(100) 次后执行清理
        // 限制文件数量 debug.max_debug_file_num
        if (m_save_file_cnt[relative_dir] == 0) {
            filenum_ctrl(relative_dir, Config.get_options().debug.max_debug_file_num);
            m_save_file_cnt[relative_dir] = 0;
        }
        m_save_file_cnt[relative_dir] =
            (m_save_file_cnt[relative_dir] + 1) % Config.get_options().debug.clean_files_freq;
    }

    auto relative_path = relative_dir / (stem + "_raw.png");
    Log.trace("Save image", relative_path);
    return asst::imwrite(relative_path, image);
}

size_t asst::AbstractTask::filenum_ctrl(const std::filesystem::path& relative_dir, size_t max_files)
{
    std::filesystem::path absolute_path;
    if (relative_dir.is_relative()) [[likely]] {
        const auto& user_dir = UserDir.get();
        absolute_path = user_dir / relative_dir;
    }
    else {
        absolute_path = relative_dir;
    }
    if (!std::filesystem::exists(absolute_path)) {
        return 0;
    }

    size_t file_nums = 0;
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> filepaths;
    std::filesystem::directory_iterator iter(absolute_path);
    for (auto& file : iter) {
        if (file.is_regular_file()) {
            ++file_nums;
            filepaths.emplace_back(last_write_time(file.path()), file.path());
        }
    }

    std::sort(filepaths.begin(), filepaths.end(),
              [](const std::pair<std::filesystem::file_time_type, std::filesystem::path>& a,
                 const std::pair<std::filesystem::file_time_type, std::filesystem::path>& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });

    long long to_del = file_nums - max_files;
    size_t deleted = 0;

    for (int i = 0; i < to_del; ++i) {
        auto path = filepaths[i].second;
        if (std::filesystem::remove(path)) {
            deleted++;
        }
    }
    LogTrace << "Finish folder cleanup delete " << deleted << " files from " << absolute_path;
    return deleted;
}
