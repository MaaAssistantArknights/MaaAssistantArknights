#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeDifficultySelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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

bool asst::RoguelikeDifficultySelectionTaskPlugin::_run()
{
    LogTraceFunction;

    std::string mode = status()->get_properties(Status::RoguelikeMode).value();
    // todo:以后可以根据传入的难度值选择难度?

    // 当前难度
    std::string difficulty = status()->get_properties(Status::RoguelikeDifficulty).value();
    if (m_roguelike_theme != "Phantom" && mode == "4") {
        if (difficulty == "max") {
            ProcessTask(*this, { m_roguelike_theme + "@Roguelike@ChooseDifficulty_Hardest" }).run();
        }
        else {
            ProcessTask(*this, { m_roguelike_theme + "@Roguelike@ChooseDifficulty_Easiest" }).run();
        }
        ProcessTask(*this, { m_roguelike_theme + "@Roguelike@ChooseDifficultyConfirm" }).run();
    }

    return true;
}
