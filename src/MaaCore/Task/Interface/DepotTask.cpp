#include "DepotTask.h"

#include "Task/Miscellaneous/DepotRecognitionTask.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"

asst::DepotTask::DepotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    enter_task->set_tasks({ "DepotBegin" }).set_ignore_error(true);
    enter_task->register_plugin<ScreenshotTaskPlugin>();
    m_subtasks.emplace_back(enter_task);

    m_recognition_task_ptr = std::make_shared<DepotRecognitionTask>(m_callback, m_inst, TaskType);
    m_recognition_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_recognition_task_ptr);
}

void asst::DepotTask::set_strict_scan(bool value)
{
    m_recognition_task_ptr->set_strict_scan(value);
}
