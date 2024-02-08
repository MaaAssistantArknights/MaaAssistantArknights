#include "Task/Interface/ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

#include "Task/Reclamation/ReclamationBattlePlugin.h"
#include "Task/Reclamation/ReclamationConclusionReportPlugin.h"
#include "Task/Reclamation/ReclamationControlTask.h"

auto asst::ReclamationTask::init_reclamation_fire_within_the_sand()
{
    auto ptr = std::make_shared<fire_within_the_sand_task>(m_callback, m_inst, TaskType);
    ptr->register_plugin<ReclamationConclusionReportPlugin>();
    m_subtasks = { ptr };
    m_reclamation_task_ptr = ptr;
    return ptr;
}

auto asst::ReclamationTask::init_reclamation_tales_within_the_sand()
{
    auto ptr = std::make_shared<tales_within_the_sand_task>(m_callback, m_inst, TaskType);
    ptr->set_tasks({ "Reclamation2" });
    m_subtasks = { ptr };
    m_reclamation_task_ptr = ptr;
    return ptr;
}

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
    init_reclamation_tales_within_the_sand();
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    int theme = params.get("theme", 1);

    switch (theme) {
    case 0: // 沙中之火
    {
        auto ptr = std::dynamic_pointer_cast<fire_within_the_sand_task>(m_reclamation_task_ptr);
        if (!ptr) ptr = init_reclamation_fire_within_the_sand();
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
        break;
    }
    case 1: // 沙洲遗闻
    {
        auto ptr = std::dynamic_pointer_cast<tales_within_the_sand_task>(m_reclamation_task_ptr);
        if (!ptr) ptr = init_reclamation_tales_within_the_sand();
        break;
    }
    default:
        Log.error(__FUNCTION__, "Unknown theme", theme);
        return false;
    }
    return true;
}
