#include "CopilotTask.h"

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
      m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType))
{
    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(0).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_subtasks.emplace_back(m_formation_task_ptr);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp
        ->set_tasks({ "BattleStartNormal", "BattleStartRaid", "BattleStartExercise", "BattleStartSimulation",
                      "BattleStartTrials" })
        .set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    if (m_running) {
        return false;
    }

    auto filename_opt = params.find<std::string>("filename");
    if (!filename_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (!Copilot.load(utils::path(*filename_opt))) {
        Log.error("CopilotConfig parse failed");
        return false;
    }

    m_stage_name = Copilot.get_stage_name();
    if (!m_battle_task_ptr->set_stage_name(m_stage_name)) {
        Log.error("Not support stage");
        return false;
    }

    bool with_formation = params.get("formation", false);
    m_formation_task_ptr->set_enable(with_formation);
    std::string support_unit_name = params.get("support_unit_name", std::string());
    m_formation_task_ptr->set_support_unit_name(std::move(support_unit_name));

    return true;
}
