#include "CreditFightTask.h"

#include <utility>

#include "TaskData.h"

#include "CopilotTask.h"
#include "Plugin/GameCrashRestartTaskPlugin.h"
#include "Sub/ProcessTask.h"
#include "Sub/StageNavigationTask.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/WorkingDir.hpp"

asst::CreditFightTask::CreditFightTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType)
{
    auto start_up_task_ptr = std::make_shared<ProcessTask>(m_callback, m_callback_arg, m_task_chain);
    auto stage_navigation_task_ptr = std::make_shared<StageNavigationTask>(m_callback, m_callback_arg, m_task_chain);
    auto fight_task_ptr = std::make_shared<ProcessTask>(m_callback, m_callback_arg, m_task_chain);
    auto copilot_task_ptr = std::make_shared<CopilotTask>(m_callback, m_callback_arg);

    // 读取关卡名
    std::string copilot_path = utils::path_to_utf8_string(ResDir.get() / "resource" / "credit_fight_copilot.json");

    std::string stage_name = copilot_task_ptr->get_stage_name(copilot_path); // OF-1

    // 开始
    start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);

    // 关卡导航
    stage_navigation_task_ptr->set_stage_name(stage_name);
    stage_navigation_task_ptr->set_enable(true).set_ignore_error(false);

    // 自动战斗
    json::value copilotparams;
    copilotparams["stage_name"] = stage_name;

    copilotparams["filename"] = copilot_path;
    copilotparams["formation"] = true;
    copilotparams["support_unit_name"] = "_RANDOM_";
    copilot_task_ptr->set_params(copilotparams);

    // 战斗结束后
    fight_task_ptr->set_tasks({ "EndOfActionAndStop" }).set_ignore_error(false);

    auto game_restart_plugin_ptr = fight_task_ptr->register_plugin<GameCrashRestartTaskPlugin>();
    game_restart_plugin_ptr->set_retry_times(0);

    m_subtasks.emplace_back(start_up_task_ptr);
    m_subtasks.emplace_back(stage_navigation_task_ptr);
    m_subtasks.emplace_back(copilot_task_ptr);
    m_subtasks.emplace_back(fight_task_ptr);
}
