#include "OperatorDevelopmentTask.h"

#include "Task/OperatorDevelopment/OperatorDevelopmentPlanParser.h"
#include "Task/OperatorDevelopment/OperatorDevelopmentProcessTask.h"
#include "Utils/Logger.hpp"

asst::OperatorDevelopmentTask::OperatorDevelopmentTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_process_task_ptr(std::make_shared<OperatorDevelopmentProcessTask>(callback, inst, TaskType))
{
    m_process_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_process_task_ptr);
}

bool asst::OperatorDevelopmentTask::set_params(const json::value& params)
{
    LogTraceFunction;
    auto plan = parse_operator_development_plan(params);
    if (!plan) {
        Log.error("OperatorDevelopment params: invalid plans");
        return false;
    }
    m_process_task_ptr->set_plan(std::move(*plan));
    return true;
}
