#include "StrongholdProtocolTask.h"

#include "Task/ProcessTask.h"
#include "Task/Stronghold/StrongholdProtocolTaskPlugin.h"
#include "Utils/Logger.hpp"

asst::StrongholdProtocolTask::StrongholdProtocolTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_stronghold_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_plugin_ptr = m_stronghold_task_ptr->register_plugin<StrongholdProtocolTaskPlugin>();
    m_subtasks.emplace_back(m_stronghold_task_ptr);
}

bool asst::StrongholdProtocolTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (params.exists("mode") && !params.at("mode").is_string()) {
        LogError << __FUNCTION__ << "expected a stronghold protocol mode string";
        return false;
    }
    const auto mode = params.get("mode", std::string("badge"));
    if (mode == "score") {
        m_stronghold_task_ptr->set_tasks({ "MiniGame@SPA@Begin" });
        return true;
    }
    if (mode != "badge") {
        LogError << __FUNCTION__ << "invalid stronghold protocol mode" << mode;
        return false;
    }

    const std::string theme = params.get("theme", std::string(StrongholdProtocolTheme::Regulation));
    if (theme == StrongholdProtocolTheme::Alliance) {
        // 盟约（第 2 期）当前未开放，空跑结束（同生息演算沙中之火）
        LogInfo << __FUNCTION__ << "theme" << theme << "is not available";
        m_stronghold_task_ptr->set_tasks({ "Stop" });
        return true;
    }
    if (theme != StrongholdProtocolTheme::Regulation) {
        Log.error("set_params failed, invalid stronghold protocol theme:", theme);
        return false;
    }

    const int target = params.get("target", 3);
    if (target != 3 && target != 6) {
        Log.error("set_params failed, invalid stronghold protocol target:", target);
        return false;
    }
    const int runs = params.get("runs", 60);
    if (runs <= 0) {
        Log.error("set_params failed, invalid stronghold protocol runs:", runs);
        return false;
    }
    const int attempts = params.get("attempts", 0);
    if (attempts < 0) {
        Log.error("set_params failed, invalid stronghold protocol attempts:", attempts);
        return false;
    }

    m_stronghold_task_ptr->set_tasks({ "StrongholdProtocol@Farm@Begin" });
    m_plugin_ptr->set_target(target).set_runs(runs).set_attempts(attempts);
    return true;
}
