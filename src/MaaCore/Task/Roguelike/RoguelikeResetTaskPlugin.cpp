#include "RoguelikeResetTaskPlugin.h"

#include "Status.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeResetTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeResetTaskPlugin::_run()
{
    // 简单粗暴，后面如果多任务间有联动可能要改改
    status()->clear_number();
    status()->clear_str();
    return true;
}
