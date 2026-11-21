#include "SeasonalTask.h"

#include "Task/ProcessTask.h"
#include "Task/Stronghold/StrongholdProtocolTaskPlugin.h"
#include "Utils/Logger.hpp"

asst::SeasonalTask::SeasonalTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_plugin_ptr = m_task_ptr->register_plugin<StrongholdProtocolTaskPlugin>();
    m_plugin_ptr->enable_terminal_entry();
    m_task_ptr->set_tasks({ "StrongholdProtocol@Farm@Begin" });
    m_subtasks.emplace_back(m_task_ptr);
}

bool asst::SeasonalTask::set_params(const json::value& params)
{
    LogTraceFunction;

    const int rounds = params.get("rounds", 3);
    if (rounds <= 0) {
        // 0 = 跳过任务（GUI 侧已记日志），空跑结束
        LogInfo << __FUNCTION__ << "rounds" << rounds << "is not positive, skip task";
        m_task_ptr->set_tasks({ "Stop" });
        return true;
    }

    if (!m_plugin_ptr->set_farm_params(params, 2)) {
        return false;
    }

    m_task_ptr->set_tasks({ "StrongholdProtocol@Farm@Begin" });
    m_plugin_ptr->set_runs(rounds);
    return true;
}
