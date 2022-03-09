#include "AwardTask.h"

#include "ProcessTask.h"

asst::AwardTask::AwardTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_award_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType))
{
    m_award_task_ptr->set_tasks({ "AwardBegin" });
    m_subtasks.emplace_back(m_award_task_ptr);
}
