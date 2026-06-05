#include "DepotTask.h"

#include "Task/Miscellaneous/DepotRecognitionTask.h"
#include "Task/ProcessTask.h"

asst::DepotTask::DepotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    auto enter_task = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    // DepotRecognitionTask assumes we have reached the depot screen. If DepotBegin fails,
    // scanning may happen on the previous screen and clear a valid depot result.
    // Keep DepotBegin required, and leave this delay here for the OperBox return path.
    enter_task->set_tasks({ "DepotBegin" }).set_post_delay("ReturnButton", 1000).set_ignore_error(false);
    m_subtasks.emplace_back(enter_task);

    auto recognition_task = std::make_shared<DepotRecognitionTask>(m_callback, m_inst, TaskType);
    recognition_task->set_retry_times(0);
    m_subtasks.emplace_back(recognition_task);
}
