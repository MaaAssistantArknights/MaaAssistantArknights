#include "AwardTask.h"

#include <utility>

#include "Task/ProcessTask.h"

asst::AwardTask::AwardTask(const AsstCallback& callback, Assistant* inst) : InterfaceTask(callback, inst, TaskType)
{
    auto award_task_ptr = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    award_task_ptr->set_tasks({ "AwardBegin" });
    m_subtasks.emplace_back(award_task_ptr);
}
