#include "OperTask.h"

#include "Task/Miscellaneous/OperRecognitionTask.h"
#include "Task/ProcessTask.h"

asst::OperTask::OperTask(const AsstCallback& callback, Assistant* inst) : InterfaceTask(callback, inst, TaskType)
{
    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    enter_task->set_tasks({ "OperatorBegin" }).set_ignore_error(true);
    m_subtasks.emplace_back(enter_task);

    auto recognition_task = std::make_shared<OperRecognitionTask>(m_callback, m_inst, TaskType);
    recognition_task->set_retry_times(0);
    m_subtasks.emplace_back(recognition_task);
}
