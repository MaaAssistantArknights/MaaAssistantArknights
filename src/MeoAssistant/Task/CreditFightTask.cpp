#include "CreditFightTask.h"

#include <utility>

#include "TaskData.h"

#include "CopilotTask.h"
#include "Plugin/GameCrashRestartTaskPlugin.h"
#include "Sub/AbstractTask.h"
#include "Sub/ProcessTask.h"
#include "Sub/StageNavigationTask.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/WorkingDir.hpp"

bool asst::CreditFightTask::_run()
{
    return true;
}

void asst::CreditFightTask::set_subtasks_enable(bool enable) {
    m_start_up_task_ptr->set_enable(enable);
    m_stage_navigation_task_ptr->set_enable(enable);
    m_fight_task_ptr->set_enable(enable);
    m_copilot_task_ptr->set_enable(enable);
    m_game_restart_plugin_ptr->set_enable(enable);
}

void asst::CreditFightTask::set_subtasks(std::vector<std::shared_ptr<AbstractTask>>& visit_subtasks)
{
    m_start_up_task_ptr = std::make_shared<ProcessTask>(m_callback, m_callback_arg, m_task_chain);
    m_stage_navigation_task_ptr = std::make_shared<StageNavigationTask>(m_callback, m_callback_arg, m_task_chain);
    m_fight_task_ptr = std::make_shared<ProcessTask>(m_callback, m_callback_arg, m_task_chain);
    m_copilot_task_ptr = std::make_shared<CopilotTask>(m_callback, m_callback_arg);

    // 读取关卡名
    std::string copilot_path = utils::path_to_utf8_string(ResDir.get() / "resource" / "credit_fight_copilot.json");

    std::string stage_name = m_copilot_task_ptr->get_stage_name(copilot_path); // OF-1

    // 开始
    m_start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);

    // 关卡导航
    m_stage_navigation_task_ptr->set_stage_name(stage_name);

    // 自动战斗
    json::value copilotparams;
    copilotparams["stage_name"] = stage_name;
    copilotparams["filename"] = copilot_path;
    copilotparams["formation"] = true;
    copilotparams["support_unit_name"] = "_RANDOM_";
    m_copilot_task_ptr->set_params(copilotparams);

    // 战斗结束后
    m_fight_task_ptr->set_tasks({ "EndOfActionAndStop" }).set_ignore_error(false);

    m_game_restart_plugin_ptr = m_fight_task_ptr->register_plugin<GameCrashRestartTaskPlugin>();
    m_game_restart_plugin_ptr->set_retry_times(0);

    m_start_up_task_ptr->set_enable(true).set_ignore_error(false);
    m_stage_navigation_task_ptr->set_enable(true).set_ignore_error(false);
    m_fight_task_ptr->set_enable(true).set_ignore_error(false);
    m_copilot_task_ptr->set_enable(true).set_ignore_error(false);
    m_game_restart_plugin_ptr->set_enable(true).set_ignore_error(false);

    visit_subtasks.emplace_back(m_start_up_task_ptr);
    visit_subtasks.emplace_back(m_stage_navigation_task_ptr);
    visit_subtasks.emplace_back(m_copilot_task_ptr);
    visit_subtasks.emplace_back(m_fight_task_ptr);
}
