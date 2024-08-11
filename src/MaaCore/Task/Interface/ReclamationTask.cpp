#include "Task/Interface/ReclamationTask.h"

#include "Common/AsstBattleDef.h"
#include "Status.h"
#include "Config/TaskData.h"
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

auto asst::ReclamationTask::init_reclamation_tales_within_the_sand(const bool enable_ex)
{
    auto ptr = std::make_shared<tales_within_the_sand_task>(m_callback, m_inst, TaskType);
    if (enable_ex) {
        ptr->set_tasks({ "Reclamation2Ex" });
    } else {
        ptr->set_tasks({ "Reclamation2" });
    }
    m_subtasks = { ptr };
    m_reclamation_task_ptr = ptr;
    return ptr;
}

asst::ReclamationTask::ReclamationTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
    init_reclamation_tales_within_the_sand(false);
}

bool asst::ReclamationTask::set_params(const json::value& params)
{
    LogTraceFunction;

    switch (int theme = params.get("theme", 1)) {
    case 0: // 沙中之火
    {
        if (m_current_theme != TaskTheme::FireWithinTheSand) {
            m_reclamation_task_ptr = init_reclamation_fire_within_the_sand();
            m_current_theme = TaskTheme::FireWithinTheSand;
        }
        const auto ptr = std::static_pointer_cast<fire_within_the_sand_task>(m_reclamation_task_ptr);
        // 0 - 刷分与建造点，进入战斗直接退出
        // 1 - 刷赤金，联络员买水后基地锻造
        switch (int mode = params.get("mode", 0)) {
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
        const int enable_ex = params.get("mode", 0);
        if (m_current_theme != TaskTheme::TalesWithinTheSand) {
            m_current_theme = TaskTheme::TalesWithinTheSand;
        }
        m_reclamation_task_ptr = init_reclamation_tales_within_the_sand(enable_ex);
        auto ptr = std::static_pointer_cast<tales_within_the_sand_task>(m_reclamation_task_ptr);
        if (const std::string product = params.get("product", "荧光棒"); !product.empty()) {
            Task.get<OcrTaskInfo>("Reclamation2ExClickProduct")->text = { product };
        }
        else {
            Task.get<OcrTaskInfo>("Reclamation2ExClickProduct")->text = { "荧光棒" };
        }
        // 刷满点数后停止运行
        const auto& task_name = params.get("details", "task", "");
        if (task_name.ends_with("Reclamation2SkipDaysAwardEndTask")) {
            ProcessTask(*this, {"Reclamation2SkipDaysAwardEndTask" })
                .set_times_limit("Reclamation2", 0)
                .run();
        }
        break;
    }
    default:
        Log.error(__FUNCTION__, "Unknown theme", theme);
        return false;
    }
    return true;
}
