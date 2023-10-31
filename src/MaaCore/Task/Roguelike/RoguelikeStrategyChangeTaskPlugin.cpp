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

    if (m_roguelike_config->get_theme().empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_roguelike_config->get_theme() + "@";
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

bool asst::RoguelikeStrategyChangeTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_config->get_theme();
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    // TODO: 这段识别有点冗余，要是 plugin 能获取识别结果就好了
    std::string task_name = theme + "@Roguelike@StrategyChange";
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info(task_name);
    if (!analyzer.analyze()) {
        return false;
    }

    std::string stages_task_name = theme + "@Roguelike@Stages";
    std::string current_strategy = analyzer.get_result().front().text;
    std::string strategy_task_name = stages_task_name + current_strategy;

    if (Task.get(strategy_task_name) == nullptr) {
        Log.error("Strategy task", strategy_task_name, "doesn't exist!");
        return false;
    }

    Task.set_task_base(stages_task_name, strategy_task_name);

    return true;
}
