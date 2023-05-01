#include "Task/Interface/ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

#include "Task/Reclamation/ReclamationBattlePlugin.h"
#include "Task/Reclamation/ReclamationConclusionReportPlugin.h"
#include "Task/Reclamation/ReclamationControlTask.h"

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_reclamation_task_ptr(std::make_shared<ReclamationControlTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_reclamation_task_ptr->register_plugin<ReclamationConclusionReportPlugin>();
    m_subtasks.emplace_back(m_reclamation_task_ptr);
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    // 0 - 刷分与建造点，进入战斗直接退出
    // 1 - 刷赤金，联络员买水后基地锻造
    int mode = params.get("mode", 0);
    switch (mode) {
    case 0:
        m_reclamation_task_ptr->set_task_mode(ReclamationTaskMode::GiveupUponFight);
        break;
    case 1:
        m_reclamation_task_ptr->set_task_mode(ReclamationTaskMode::SmeltGold);
        break;
    default:
        Log.error(__FUNCTION__, "| Unknown mode", mode);
        return false;
    }

    return true;
}
