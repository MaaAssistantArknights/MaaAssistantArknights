#include "SideStoryTask.h"

#include <ranges>
#include <utility>

#include "Config/TaskData.h"
#include "Task/Fight/SideStoryReopenTask.h"
#include "Task/Fight/StageDropsTaskPlugin.h"
#include "Task/Fight/StageNavigationTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::SideStoryTask::SideStoryTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
    m_stage_navigation_task_ptr(std::make_shared<StageNavigationTask>(m_callback, m_inst, TaskType)),
    m_sidestory_reopen_task_ptr(std::make_shared<SideStoryReopenTask>(m_callback, m_inst, TaskType)),
    m_claim_task_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    // 进入选关界面
    // 对于指定关卡，就是主界面的“终端”点进去
    // 对于当前/上次，就是点到 蓝色开始行动 为止。
    m_start_up_task_ptr->set_tasks({ "StageBegin" })
        .set_times_limit("GoLastBattle", 0)
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StageSNReturnFlag", 0)
        .set_times_limit("PRTS1", 0)
        .set_times_limit("PRTS2", 0)
        .set_times_limit("PRTS3", 0)
        .set_times_limit("EndOfAction", 0)
        .set_retry_times(5);
    m_start_up_task2_ptr->set_tasks({ "StageBegin" })
        .set_times_limit("GoLastBattle", 0)
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StageSNReturnFlag", 0)
        .set_times_limit("PRTS1", 0)
        .set_times_limit("PRTS2", 0)
        .set_times_limit("PRTS3", 0)
        .set_times_limit("EndOfAction", 0)
        .set_retry_times(5);

    m_stage_navigation_task_ptr->set_retry_times(0);
    m_stage_navigation_task2_ptr->set_retry_times(0);
    m_sidestory_reopen_task_ptr->set_enable(false).set_retry_times(0);

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(m_sidestory_reopen_task_ptr);
    m_subtasks.emplace_back(m_start_up_task2_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task2_ptr);
    m_subtasks.emplace_back(m_claim_task_task_ptr);
}

bool asst::SideStoryTask::set_params(const json::value& params)
{
    LogTraceFunction;

    const std::string code = params.get("code", "");
    if (!Task.get(code + "-OpenOpt")) {
        return false;
    }

    const bool stage_replay = params.get("stage_replay", false);
    m_start_up_task_ptr->set_enable(stage_replay);
    m_stage_navigation_task_ptr->set_enable(stage_replay);
    m_sidestory_reopen_task_ptr->set_enable(stage_replay);
    if (stage_replay) {
        m_stage_navigation_task_ptr->set_stage_name(code + "-OpenOpt");
    }

    const bool award = params.get("award", false);
    if (award) {
        if (!Task.get(code + "-OpenTask")) {
            LogError << "claim task not exists:" << code + "-OpenTask";
            return false;
        }
        m_claim_task_task_ptr->set_tasks({ code + "-OpenTask" }).set_retry_times(20);
    }
    m_start_up_task2_ptr->set_enable(award);
    m_stage_navigation_task2_ptr->set_enable(award);
    m_claim_task_task_ptr->set_enable(award);

    return true;
}
