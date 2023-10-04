#include "RoguelikeCiphertextBoardUseTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "Roguelike@StrategyChange") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeCiphertextBoardUseTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    

    return true;
}
