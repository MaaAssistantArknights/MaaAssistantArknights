#include "RoguelikeDebugTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeDebugTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (msg == AsstMsg::SubTaskError) {
        return true;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (msg == AsstMsg::SubTaskStart && details.get("subtask", std::string()) == "ProcessTask") {
        if (task_view == "Roguelike@ExitThenAbandon" || task_view == "Roguelike@GamePass") {
            return true;
        }
    }

    return false;
}

bool asst::RoguelikeDebugTaskPlugin::_run()
{
    save_img(utils::path("debug") / utils::path("roguelike"));
    return true;
}
