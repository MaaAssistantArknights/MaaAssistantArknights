#include "RoguelikeLevelTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/RoguelikeControlTaskPlugin.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeLevelTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    const std::string& pre_task = details.get("pre_task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore" && pre_task != task) {
        return true;
    }
    return false;
}

bool asst::RoguelikeLevelTaskPlugin::load_params(const json::value& params)
{
    m_stop_at_max = params.get("stop_at_max_level", false);
    return m_stop_at_max;
}

bool asst::RoguelikeLevelTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_stop_at_max) {
        if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevel" }).run()) {
            if (ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevelMax" }).set_retry_times(2).run()) {
                stop_roguelike();
            }
            else {
                ProcessTask(*this, { "Return" }).run();
            }
        }
    }
    return true;
}

void asst::RoguelikeLevelTaskPlugin::stop_roguelike() const
{
    ProcessTask(*this, { "Return" }).run();
    m_task_ptr->set_enable(false);
}
