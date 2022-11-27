#include "CloseDownTask.h"

#include <utility>

#include "Task/Miscellaneous/StopGameTaskPlugin.h"
#include "Task/ProcessTask.h"

asst::CloseDownTask::CloseDownTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{
    m_subtasks.emplace_back(std::make_shared<StopGameTaskPlugin>(m_callback, m_inst, TaskType));
}
