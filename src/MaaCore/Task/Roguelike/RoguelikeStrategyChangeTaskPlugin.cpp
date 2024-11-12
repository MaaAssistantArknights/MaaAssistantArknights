#include "RoguelikeStrategyChangeTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeStrategyChangeTaskPlugin::verify(AsstMsg msg, const json::value& details) const
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
    if (task_view == "Roguelike@StrategyChange") {
        m_result = details.get("details", "result", json::object());
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeStrategyChangeTaskPlugin::_run()
{
    LogTraceFunction;

    const std::string theme = m_config->get_theme();
    const std::string stages_task_name = theme + "@Roguelike@Stages";
    const std::string current_strategy = m_result.get("text", "");
    if (current_strategy.empty() || current_strategy.find("_SKIP_") != std::string::npos) {
        Log.info("Skip strategy change, current strategy is", current_strategy);
        return true;
    }
    const std::string strategy_task_name = stages_task_name + current_strategy;

    if (Task.get(strategy_task_name) == nullptr) [[unlikely]] {
        Log.error("Strategy task", strategy_task_name, "doesn't exist!");
        return false;
    }

    Task.set_task_base(stages_task_name, strategy_task_name);

    return true;
}
