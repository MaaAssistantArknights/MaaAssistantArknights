#include "CopilotTask.h"

#include "Resource.h"
#include "ProcessTask.h"
#include "BattleProcessTask.h"
#include "BattleFormationTask.h"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, callback_arg, TaskType)),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, callback_arg, TaskType))
{
    auto start_1_tp = std::make_shared<ProcessTask>(callback, callback_arg, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" })
        .set_retry_times(0)
        .set_ignore_error(true);
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
    if (m_finished) {
        return false;
    }

    auto stage_name_opt = params.find<std::string>("stage_name");
    if (!stage_name_opt) {
        return false;
    }

    const std::string& stage_name = *stage_name_opt;
    m_battle_task_ptr->set_stage_name(stage_name);
    m_formation_task_ptr->set_stage_name(stage_name);

    bool with_formation = params.get("formation", false);
    m_formation_task_ptr->set_enable(with_formation);

    std::string filename = params.get("filename", std::string());
    // 文件名为空时，不加载资源，直接返回 true
    return filename.empty() || Resrc.copilot().load(filename);
}
