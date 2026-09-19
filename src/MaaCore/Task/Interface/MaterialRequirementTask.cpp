#include "MaterialRequirementTask.h"

#include "Task/Miscellaneous/MaterialRequirementRecognitionTask.h"

asst::MaterialRequirementTask::MaterialRequirementTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_recognition_task_ptr(std::make_shared<MaterialRequirementRecognitionTask>(callback, inst, TaskType))
{
    m_recognition_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_recognition_task_ptr);
}
