#include "RoguelikeLastRewardTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeLastRewardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (auto pos = task_view.find(roguelike_name); pos != std::string_view::npos) {
        task_view.remove_prefix(pos + roguelike_name.length());
    }
    if (task_view == "Roguelike@ExitThenAbandon") {
        m_need_change_difficulty_higher = false;
        return true;
    }
    if (task_view == "Roguelike@ExitThenAbandon_mode4") {
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

    std::string theme = status()->get_properties(Status::RoguelikeTheme).value();
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    if (theme != "Phantom" && mode == "4") {
        if (m_need_change_difficulty_higher) {
            status()->set_properties(Status::RoguelikeNeedChangeDifficulty, "max");
        }
        else {
            status()->set_properties(Status::RoguelikeNeedChangeDifficulty, "0");
        }
    }
    return true;
}
