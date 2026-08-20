#include "FightStartWaitTaskPlugin.h"

#include <algorithm>
#include <chrono>

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/PipelineAnalyzer.h"

namespace
{
constexpr std::string_view StartButtonWaitTask = "Fight@StartButton2WaitTime";
constexpr auto PollInterval = std::chrono::seconds(3);
}

bool asst::FightStartWaitTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    return msg == AsstMsg::SubTaskStart && details.get("subtask", std::string()) == "ProcessTask" &&
           details.get("details", "task", std::string()) == StartButtonWaitTask &&
           (Config.get_options().battle_start_timeout_seconds != Options::BattleStartTimeoutSecondsDefault ||
            m_post_delay_overridden);
}

void asst::FightStartWaitTaskPlugin::set_task_ptr(AbstractTask* ptr)
{
    AbstractTaskPlugin::set_task_ptr(ptr);
    m_process_task_ptr = dynamic_cast<ProcessTask*>(ptr);
}

bool asst::FightStartWaitTaskPlugin::_run()
{
    const auto wait_task = Task.get(StartButtonWaitTask);
    if (!m_process_task_ptr || !wait_task) {
        Log.error(__FUNCTION__, "invalid Fight start wait task");
        return false;
    }

    const int timeout_seconds = Config.get_options().battle_start_timeout_seconds;
    if (timeout_seconds == Options::BattleStartTimeoutSecondsDefault) {
        m_process_task_ptr->set_post_delay(std::string(StartButtonWaitTask), wait_task->post_delay);
        m_post_delay_overridden = false;
        return true;
    }

    // The plugin owns the wait when the hidden timeout differs from the default. Keep
    // up to the original initial delay, then poll without clicking until the deadline.
    m_process_task_ptr->set_post_delay(std::string(StartButtonWaitTask), 0);
    m_post_delay_overridden = true;
    const auto start = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(timeout_seconds);
    const auto deadline = start + timeout;
    const auto initial_delay =
        std::min(std::chrono::milliseconds(wait_task->post_delay),
                 std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
    if (!sleep(static_cast<unsigned>(initial_delay.count()))) {
        return false;
    }

    TaskList detection_tasks = wait_task->next;
    std::erase_if(detection_tasks, [](const std::string& name) {
        const auto task = Task.get(name);
        return !task || task->algorithm == AlgorithmType::JustReturn;
    });

    while (!need_exit()) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            Log.info(__FUNCTION__, "Fight battle start wait timed out after", timeout_seconds, "seconds");
            break;
        }

        PipelineAnalyzer analyzer(ctrler()->get_image(), Rect(), inst());
        analyzer.set_tasks(detection_tasks);
        if (const auto result = analyzer.analyze()) {
            Log.info(__FUNCTION__, "Fight battle start wait matched", result->task_ptr->name);
            break;
        }

        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (remaining <= std::chrono::milliseconds::zero()) {
            continue;
        }
        const auto delay = std::min(remaining, std::chrono::duration_cast<std::chrono::milliseconds>(PollInterval));
        if (!sleep(static_cast<unsigned>(delay.count()))) {
            return false;
        }
    }

    return true;
}
