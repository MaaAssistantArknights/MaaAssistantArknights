#include "AwardTask.h"

#include <utility>

#include "Sub/ProcessTask.h"

asst::AwardTask::AwardTask(const AsstCallback& callback, void* callback_arg)
    : InterfaceTask(callback, callback_arg, TaskType)
{
    m_subtasks.emplace_back(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType));
}
