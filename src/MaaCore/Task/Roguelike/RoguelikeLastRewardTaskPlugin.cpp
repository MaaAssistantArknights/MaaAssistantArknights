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
        m_is_next_hardest = false;
        return false;
    }
    if (task_view == "Roguelike@ExitThenAbandon") {
        // 开始行动过且没有打完三层，重开低难度
        return !m_is_next_hardest;
    }
    if (task_view == "Roguelike@ExitThenAbandon_ToHardest") {
        // 打完低难度的三层，重开高难度烧水壶
        m_is_next_hardest = true;
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

    // 需要开局凹直升
    bool start_with_elite_two =
        status()->get_properties(Status::RoguelikeStartWithEliteTwo).value() == std::to_string(true);
    if (m_roguelike_theme != "Phantom" && mode == "4") {
        if (m_is_next_hardest) {
            status()->set_properties(Status::RoguelikeDifficulty, "max");
            // 获得热水壶和演讲时停止肉鸽（凹直升则继续），获得其他奖励时重开
            std::string last_reward_stop_or_continue =
                start_with_elite_two ? "Roguelike@LastReward_default" : "Roguelike@LastReward_stop";
            Task.set_task_base("Roguelike@LastReward", last_reward_stop_or_continue);
            Task.set_task_base("Roguelike@LastReward2", "Roguelike@LastReward_restart");
            Task.set_task_base("Roguelike@LastReward3", "Roguelike@LastReward_restart");
            Task.set_task_base("Roguelike@LastReward4", last_reward_stop_or_continue);
            Task.set_task_base("Roguelike@LastRewardRand", "Roguelike@LastReward_restart");
        }
        else {
            status()->set_properties(Status::RoguelikeDifficulty, "0");
            // 重置开局奖励 next，获得任意奖励均继续
            Task.set_task_base("Roguelike@LastReward", "Roguelike@LastReward_default");
            Task.set_task_base("Roguelike@LastReward2", "Roguelike@LastReward_default");
            Task.set_task_base("Roguelike@LastReward3", "Roguelike@LastReward_default");
            Task.set_task_base("Roguelike@LastReward4", "Roguelike@LastReward_default");
            Task.set_task_base("Roguelike@LastRewardRand", "Roguelike@LastReward_default");
        }
    }
    return true;
}
