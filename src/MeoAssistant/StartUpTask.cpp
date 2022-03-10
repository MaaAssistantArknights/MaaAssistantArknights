#include "StartUpTask.h"

#include "ProcessTask.h"

asst::StartUpTask::StartUpTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType))
{
    m_start_up_task_ptr->set_tasks({ "StartUp" })
        .set_times_limit("ReturnToTerminal", 0)
        .set_times_limit("Terminal", 0);
    m_subtasks.emplace_back(m_start_up_task_ptr);
}
