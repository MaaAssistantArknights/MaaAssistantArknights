#include "RoguelikeInputSeedTaskPlugin.h"

#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeInputSeedTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    LogTraceFunction;

    // 萨卡兹种子刷钱
    if (m_config->get_theme() == RoguelikeTheme::Sarkaz && params.get("start_with_seed", false)) {
        RoguelikeStageEncounter.set_event(m_config->get_theme(), m_config->get_mode(), "相遇", 3, 4);
        return true;
    }
    return false;
}

bool asst::RoguelikeInputSeedTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore") {
        return true;
    }
    return false;
}

bool asst::RoguelikeInputSeedTaskPlugin::_run()
{
    return ProcessTask(*this, { m_config->get_theme() + "@Roguelike@StartExploreWithSeed" }).run();
}
