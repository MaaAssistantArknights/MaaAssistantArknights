#include "CraftTask.h"

#include "Utils/Logger.hpp"

#include "Task/ProcessTask.h"

asst::CraftTask::CraftTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_infrast_begin_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    m_infrast_begin_task_ptr->set_tasks({ "InfrastBegin" }).set_ignore_error(true);

    m_subtasks.emplace_back(m_infrast_begin_task_ptr);
}

bool asst::CraftTask::set_params(const json::value& params)
{
    LogTraceFunction;

    if (!m_running) {
        m_subtasks.clear();
        m_subtasks.emplace_back(m_infrast_begin_task_ptr);
    }
}
