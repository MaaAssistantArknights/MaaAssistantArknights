#include "CopilotTask.h"

#include "AsstUtils.hpp"
#include "BattleFormationTask.h"
#include "BattleProcessTask.h"
#include "CopilotConfiger.h"
#include "Logger.hpp"
#include "ProcessTask.h"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
      m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, callback_arg, TaskType)),
      m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, callback_arg, TaskType))
{
    auto start_1_tp = std::make_shared<ProcessTask>(callback, callback_arg, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(0).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_subtasks.emplace_back(m_formation_task_ptr)->set_ignore_error(false).set_retry_times(0);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, callback_arg, TaskType);
    start_2_tp->set_tasks({ "BattleStartNormal", "BattleStartRaid", "BattleStartExercise", "BattleStartSimulation" })
        .set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    if (m_running) {
        return false;
    }

    auto stage_name_opt = params.find<std::string>("stage_name");
    auto filename_opt = params.find<std::string>("filename");
    if (!stage_name_opt || !filename_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    const std::string& stage_name = *stage_name_opt;
    m_battle_task_ptr->set_stage_name(stage_name);
    m_formation_task_ptr->set_stage_name(stage_name);

    bool with_formation = params.get("formation", false);
    m_formation_task_ptr->set_enable(with_formation);

    std::string filename = std::move(filename_opt.value());
#ifdef _WIN32
    filename = utils::utf8_to_ansi(filename);
#endif //  _WIN32

    return Copilot.load(filename);
}
