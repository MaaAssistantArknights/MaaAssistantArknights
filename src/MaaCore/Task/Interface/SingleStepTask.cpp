#include "SingleStepTask.h"

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Task/Experiment/SingleStepBattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::SingleStepTask::SingleStepTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
}

bool asst::SingleStepTask::set_params(const json::value& params)
{
    LogTraceFunction;

    std::string type = params.get("type", "");
    std::string subtype = params.get("subtype", "");
    auto details_opt = params.find("details");

    if (type == "copilot") {
        if (subtype == "stage" && details_opt) {
            return set_copilot_stage(*details_opt);
        }
        else if (subtype == "start") {
            return append_copllot_start();
        }
        else if (subtype == "action" && details_opt) {
            return append_copilot_action(*details_opt);
        }
    }
    return false;
}

bool asst::SingleStepTask::set_copilot_stage(const json::value& details)
{
    return SingleStepBattleProcessTask::set_stage_name_cache(details.get("stage_name", ""));
}

bool asst::SingleStepTask::append_copllot_start()
{
    LogTraceFunction;

    auto start_2_tp = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_ignore_error(false);
    m_subtasks.emplace_back(std::move(start_2_tp));

    return true;
}

bool asst::SingleStepTask::append_copilot_action(const json::value& details)
{
    LogTraceFunction;

    auto task = std::make_shared<SingleStepBattleProcessTask>(m_callback, m_inst, TaskType);

    try {
        // 请参考自动战斗协议
        auto actions = CopilotConfig::parse_actions(details);
        task->set_actions(std::move(actions));
    }
    catch (const json::exception& e) {
        Log.error(__FUNCTION__, e.what());
        return false;
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, e.what());
        return false;
    }

    m_subtasks.emplace_back(std::move(task));

    return true;
}
