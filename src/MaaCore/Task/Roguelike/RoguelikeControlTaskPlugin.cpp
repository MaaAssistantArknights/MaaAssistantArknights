#include "RoguelikeControlTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeControlTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "RoguelikeControlTaskPlugin-Stop") {
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
    ProcessTask(*this, { m_roguelike_theme + "@Roguelike@ExitThenAbandon" })
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
