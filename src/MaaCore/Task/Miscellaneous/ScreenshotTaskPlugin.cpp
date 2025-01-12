#include "ScreenshotTaskPlugin.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

asst::ScreenshotTaskPlugin::ScreenshotTaskPlugin(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    AbstractTaskPlugin(callback, inst, task_chain)
{
    m_screenshot_tasks.clear();
    if (auto ptr = Task.get(config_name)) {
        ranges::copy(ptr->next, std::back_inserter(m_screenshot_tasks));
    }
    else {
        Log.info(__FUNCTION__, "| no config found");
    }

#ifndef ASST_DEBUG
    static const bool need_save_debug_img = std::filesystem::exists("DEBUG.txt");
    if (!need_save_debug_img) {
        return;
    }
#endif
    if (auto ptr = Task.get(debug_config_name)) {
        ranges::copy(ptr->next, std::back_inserter(m_screenshot_tasks));
    }
    else {
        Log.info(__FUNCTION__, "| no debug config found");
    }
}

bool asst::ScreenshotTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    using namespace std::chrono_literals;
    auto now = std::chrono::steady_clock::now();
    if (task == m_last_triggered_task && now - m_last_triggered_time < 2min) {
        // 2min 内同一个任务重复触发一般是卡在某个地方了，不再截图但更新时间
        Log.trace(__FUNCTION__, "| task", task, "triggered recently, skip");
        m_last_triggered_time = now;
        return false;
    }
    if (ranges::any_of(m_screenshot_tasks, [&task](std::string_view item) { return task.ends_with(item); })) {
        m_last_triggered_task = task;
        m_last_triggered_time = now;
        return true;
    }

    return false;
}

bool asst::ScreenshotTaskPlugin::_run()
{
    save_img(utils::path("debug") / utils::path(std::string(m_task_chain)));
    return true;
}
