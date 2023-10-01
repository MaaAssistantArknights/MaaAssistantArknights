#include "RoguelikeControlTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
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
    if (task_view == "Roguelike@StartExplore") {
        m_need_exit_and_abandon = false;
        return true;
    }
    if (task_view == "Roguelike@StageTraderInvestConfirm" || task_view == "Roguelike@StageTraderInvestSystemFull") {
        m_need_exit_and_abandon = true;
        return true;
    }

    return false;
}

void asst::RoguelikeControlTaskPlugin::exit_and_abandon()
{
    std::string theme = status()->get_properties(Status::RoguelikeTheme).value();
    ProcessTask(*this, { theme + "@Roguelike@ExitThenAbandon" })
        .set_times_limit("Roguelike@StartExplore", 0)
        .run();
}

bool asst::RoguelikeControlTaskPlugin::_run()
{
    if (m_need_exit_and_abandon) {
        exit_and_abandon();
        m_need_exit_and_abandon = false;
    }
    m_task_ptr->set_enable(false);
    return true;
}
