#include "RoguelikeControlTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeControlTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "Roguelik@ControlTaskPlugin-Stop") {
        m_need_exit_then_stop = false;
        return true;
    }
    if (task_view == "RoguelikeControlTaskPlugin-ExitThenStop") {
        m_need_exit_then_stop = true;
        return true;
    }

    return false;
}

void asst::RoguelikeControlTaskPlugin::exit_then_stop()
{
    std::string theme = status()->get_properties(Status::RoguelikeTheme).value();
    ProcessTask(*this, { theme + "@Roguelike@ExitThenAbandon" })
        .set_times_limit("Roguelike@StartExplore", 0)
        .set_times_limit("Roguelike@Abandon", 0)
        .run();
}

bool asst::RoguelikeControlTaskPlugin::_run()
{
    if (m_need_exit_then_stop) {
        exit_then_stop();
        m_need_exit_then_stop = false;
    }
    m_task_ptr->set_enable(false);
    return true;
}
