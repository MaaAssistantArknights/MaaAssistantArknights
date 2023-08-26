#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Status.h"
#include "Task/Interface/RoguelikeTask.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/RoguelikeLastRewardTaskPlugin.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeDifficultySelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = status()->get_properties(Status::RoguelikeTheme).value();
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    if (theme != "Phantom" && mode == "4") {
        if (m_last_reward == true) {
            // 到第三层退出，选最高难度开水壶
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty_Hardest" }).run();
        }
        // 其他情况下开始，选最低难度
        else if (m_last_reward == false) {
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty_Easiest" }).run();
        }
        m_last_reward = false;
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficulty" }).run();
    }

    return true;
}
