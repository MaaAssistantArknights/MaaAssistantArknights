#include "CreditFightTask.h"

#include <utility>

#include "TaskData.h"

#include "Plugin/GameCrashRestartTaskPlugin.h"
#include "Sub/ProcessTask.h"
#include "CopilotTask.h"
#include "Sub/StageNavigationTask.h"
#include "Utils/AsstRanges.hpp"


asst::CreditFightTask::CreditFightTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType),
      m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_stage_navigation_task_ptr(std::make_shared<StageNavigationTask>(m_callback, m_callback_arg, TaskType)),
      m_fight_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_copilot_task_ptr(std::make_shared<CopilotTask>(m_callback, m_callback_arg))
{

    // 进入选关界面
    // 对于指定关卡，就是主界面的“终端”点进去
    // 对于当前/上次，就是点到 蓝色开始行动 为止。
    m_start_up_task_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StageSNReturnFlag", 0)
        .set_times_limit("PRTS3", 0)
        .set_times_limit("PRTS", 0)
        .set_times_limit("PRTS2", 0)
        .set_times_limit("EndOfAction", 0)
        .set_ignore_error(false)
        .set_retry_times(5);

    m_stage_navigation_task_ptr->set_enable(false).set_ignore_error(false);

    // 战斗结束后
    m_fight_task_ptr->set_tasks({ "EndOfActionAndStop" }).set_ignore_error(false);

    m_game_restart_plugin_ptr = m_fight_task_ptr->register_plugin<GameCrashRestartTaskPlugin>();
    m_game_restart_plugin_ptr->set_retry_times(0);

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(m_copilot_task_ptr);
    m_subtasks.emplace_back(m_fight_task_ptr);

    const std::string stage = "OF-1";
    
    if (!m_running) {
        m_start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);
        m_stage_navigation_task_ptr->set_stage_name(stage);
        m_stage_navigation_task_ptr->set_enable(true);
    }

    json::value copilotparams;
    copilotparams["stage_name"] = "activities/act3d0/level_act3d0_01";// OF-1
    copilotparams["filename"] = "resource/credit_fight_colilot.json";
    copilotparams["formation"] = true;
    m_copilot_task_ptr->set_params(copilotparams);
    
}

