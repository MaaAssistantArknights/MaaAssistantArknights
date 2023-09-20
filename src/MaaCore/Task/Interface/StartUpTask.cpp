#include "StartUpTask.h"

#include <utility>

#include "Config/GeneralConfig.h"
#include "Task/Miscellaneous/StartGameTaskPlugin.h"
#include "Task/Miscellaneous/AccountSwitchTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::StartUpTask::StartUpTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_start_game_task_ptr(std::make_shared<StartGameTaskPlugin>(m_callback, m_inst, TaskType)),
      m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
      m_account_switch_task_ptr(std::make_shared<AccountSwitchTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    m_start_up_task_ptr->set_tasks({ "StartUpBegin" })
        .set_times_limit("ReturnTo", 0)
        .set_task_delay(Config.get_options().task_delay * 2)
        .set_retry_times(30);
    m_account_switch_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_start_game_task_ptr)->set_ignore_error(true);
    m_subtasks.emplace_back(m_account_switch_task_ptr);
    m_subtasks.emplace_back(m_start_up_task_ptr);
}

bool asst::StartUpTask::set_params(const json::value& params)
{
    LogTraceFunction;

    std::string accountName = params.get("account_name", std::string());
    m_start_game_task_ptr->set_client_type(params.get("client_type", std::string()))
        .set_enable(params.get("start_game_enabled", false));
    m_account_switch_task_ptr->set_enable(!accountName.empty());
    m_account_switch_task_ptr->set_account(std::move(accountName));
    return true;
}
