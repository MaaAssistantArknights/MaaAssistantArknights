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
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StartExplore") {
        return true;
    }
    return false;
}

bool asst::RoguelikeLevelTaskPlugin::load_params(const json::value& params)
{
    return params.get("stop_at_max_level", false);
}

bool asst::RoguelikeLevelTaskPlugin::_run()
{
    LogTraceFunction;

    bool ret = ProcessTask(*this, { m_config->get_theme() + "@Roguelike@CheckLevel" }).run();
    if (ret) {
        ret = ProcessTask(*this, { "Roguelike@CheckLevelMax" }).set_retry_times(2).run();
        if (ret) {
            ProcessTask(*this, { "Return" }).run();
            m_task_ptr->set_enable(false);
        }
        else {
            ProcessTask(*this, { "Return" }).run();
        }
    }
    return true;
}
