#include "Task/Interface/ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

#include "Task/Reclamation/ReclamationBattlePlugin.h"


asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_reclamation_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    m_reclamation_task_ptr->set_tasks({ "Reclamation@Begin" });
    m_reclamation_task_ptr->set_ignore_error(true);
    m_reclamation_task_ptr->register_plugin<ReclamationBattlePlugin>();
    for (int i = 0; i != 100; ++i) {
        m_subtasks.emplace_back(m_reclamation_task_ptr);
    }
}
