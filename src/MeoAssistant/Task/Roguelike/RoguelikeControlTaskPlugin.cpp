#include "RoguelikeControlTaskPlugin.h"

#include "Status.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeControlTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string what = details.get("what", std::string());
    if (what != "ExceededLimit") {
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
    if (task_view == "Roguelike@StartExplore" || task_view == "Roguelike@StageTraderInvestConfirm" ||
        task_view == "Roguelike@StageTraderInvestSystemFull") {
        return true;
    }

    return false;
}

bool asst::RoguelikeControlTaskPlugin::_run()
{
    m_task_ptr->set_enable(false);
    return true;
}
