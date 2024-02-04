#include "Task/Interface/ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

#include "Task/Reclamation/ReclamationBattlePlugin.h"
#include "Task/Reclamation/ReclamationConclusionReportPlugin.h"
#include "Task/Reclamation/ReclamationControlTask.h"

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
    init_reclamation_2_task();
}

void asst::ReclamationTask::init_reclamation_task()
{
    auto ptr = std::make_shared<ReclamationControlTask>(m_callback, m_inst, TaskType);
    ptr->register_plugin<ReclamationConclusionReportPlugin>();
    m_subtasks = { ptr };
    m_reclamation_task_ptr = ptr;
}

void asst::ReclamationTask::init_reclamation_2_task()
{
    auto ptr = std::make_shared<ProcessTask>(m_callback, m_inst, TaskType);
    ptr->set_tasks({ "Reclamation2" });
    m_subtasks = { ptr };
    m_reclamation_task_ptr = ptr;
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    // TODO: add option to switch between 沙中之火 and 沙洲遗闻

    // TODO: dynamic_cast is not reliable, add a enum member variable
    if (auto ptr = std::dynamic_pointer_cast<ReclamationControlTask>(m_reclamation_task_ptr)) {
        // 0 - 刷分与建造点，进入战斗直接退出
        // 1 - 刷赤金，联络员买水后基地锻造
        int mode = params.get("mode", 0);
        switch (mode) {
        case 0:
            ptr->set_task_mode(ReclamationTaskMode::GiveupUponFight);
            break;
        case 1:
            ptr->set_task_mode(ReclamationTaskMode::SmeltGold);
            break;
        default:
            Log.error(__FUNCTION__, "| Unknown mode", mode);
            return false;
        }
    }
    return true;
}
