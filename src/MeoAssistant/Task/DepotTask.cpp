#include "DepotTask.h"

#include "DepotRecognitionTask.h"
#include "ProcessTask.h"

asst::DepotTask::DepotTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType)
{
    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType);
    enter_task->set_tasks({ "DepotBegin" });
    m_subtasks.emplace_back(enter_task);

    auto recognition_task = std::make_shared<DepotRecognitionTask>(m_callback, m_callback_arg, TaskType);
    recognition_task->set_retry_times(0);
    m_subtasks.emplace_back(recognition_task);
}
