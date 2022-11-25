#include "CloseDownTask.h"

#include <utility>

#include "Task/Miscellaneous/StopGameTaskPlugin.h"
#include "Task/ProcessTask.h"

asst::CloseDownTask::CloseDownTask(const AsstCallback& callback, void* callback_arg)
    : InterfaceTask(callback, callback_arg, TaskType)
{
    m_subtasks.emplace_back(std::make_shared<StopGameTaskPlugin>(m_callback, m_callback_arg, TaskType));
}
