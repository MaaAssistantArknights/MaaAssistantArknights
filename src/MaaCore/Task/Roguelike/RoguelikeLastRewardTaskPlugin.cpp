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
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@ExitThenAbandon_mode4") {
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
    
    if (theme != "Phantom" && mode == "4" ) {
        set_last_reward(true);
    }
    return true;
}

void asst::RoguelikeLastRewardTaskPlugin::set_last_reward(bool last_reward)
{
    AbstractTaskPlugin::set_last_reward(last_reward);
}
