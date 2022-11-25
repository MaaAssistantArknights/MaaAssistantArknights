#include "AwardTask.h"

#include <utility>

#include "Task/ProcessTask.h"

asst::AwardTask::AwardTask(const AsstCallback& callback, void* callback_arg)
    : InterfaceTask(callback, callback_arg, TaskType)
{
    auto award_task_ptr = std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType);
    award_task_ptr->set_tasks({ "AwardBegin" });
    m_subtasks.emplace_back(award_task_ptr);
}
