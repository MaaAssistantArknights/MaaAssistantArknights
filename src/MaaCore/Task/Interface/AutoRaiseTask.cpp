#include "AutoRaiseTask.h"

#include "Task/AutoRaise/AutoRaisePlanParser.h"
#include "Task/AutoRaise/AutoRaiseProcessTask.h"
#include "Utils/Logger.hpp"

asst::AutoRaiseTask::AutoRaiseTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_process_task_ptr(std::make_shared<AutoRaiseProcessTask>(callback, inst, TaskType))
{
    m_process_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_process_task_ptr);
}

bool asst::AutoRaiseTask::set_params(const json::value& params)
{
    LogTraceFunction;
    auto plan = parse_auto_raise_plan(params);
    if (!plan) {
        Log.error("AutoRaise params: invalid plans");
        return false;
    }
    m_process_task_ptr->set_plan(std::move(*plan));
    return true;
}
