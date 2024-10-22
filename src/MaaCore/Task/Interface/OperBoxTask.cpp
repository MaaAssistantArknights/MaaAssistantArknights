#include "OperBoxTask.h"

#include "Task/Miscellaneous/OperBoxRecognitionTask.h"
#include "Task/ProcessTask.h"

asst::OperBoxTask::OperBoxTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    enter_task->set_tasks({ "OperBoxBegin" }).set_ignore_error(true);
    m_subtasks.emplace_back(enter_task);

    auto recognition_task = std::make_shared<OperBoxRecognitionTask>(m_callback, m_inst, TaskType);
    recognition_task->set_retry_times(0);
    m_subtasks.emplace_back(recognition_task);
}
