#include "RoguelikeLastRewardTaskPlugin.h"

#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeLastRewardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_roguelike_theme + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore") {
        // 标记开始行动
        m_need_change_difficulty_higher = false;
        return false;
    }
    if (task_view == "Roguelike@ExitThenAbandon") {
        // 开始行动过且没有打完三层，重开低难度
        return !m_need_change_difficulty_higher;
    }
    if (task_view == "Roguelike@ExitThenAbandon_ToHardest") {
        // 打完低难度的三层，重开高难度烧水壶
        m_need_change_difficulty_higher = true;
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeLastRewardTaskPlugin::_run()
{
    LogTraceFunction;

    std::string mode = status()->get_properties(Status::RoguelikeMode).value();
    std::string stages_task_name = m_roguelike_theme + "@Roguelike@Stages";
    std::string strategy_task_name = stages_task_name + "_default";

    if (Task.get(strategy_task_name) == nullptr) {
        Log.error("Strategy task", strategy_task_name, "doesn't exist!");
    }
    else {
        // 重置选点策略
        Task.set_task_base(stages_task_name, strategy_task_name);
    }

    if (m_roguelike_theme != "Phantom" && mode == "4") {
        if (m_need_change_difficulty_higher) {
            status()->set_properties(Status::RoguelikeDifficulty, "max");
        }
        else {
            status()->set_properties(Status::RoguelikeDifficulty, "0");
        }
    }
    return true;
}
