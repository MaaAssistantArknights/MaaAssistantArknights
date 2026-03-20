#include "StartUpTask.h"

#include <utility>

#include "Config/GeneralConfig.h"
#include "Task/Miscellaneous/AccountSwitchTask.h"
#include "Task/Miscellaneous/StartGameTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::StartUpTask::StartUpTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_start_game_task_ptr(std::make_shared<StartGameTaskPlugin>(m_callback, m_inst, TaskType)),
    m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_inst, TaskType)),
    m_account_switch_task_ptr(std::make_shared<AccountSwitchTask>(m_callback, m_inst, TaskType))
{
    LogTraceFunction;

    // 前两项认为用户已手动启动至首页，如果识别到了切换但不认为在首页说明主题不支持
    m_start_up_task_ptr
        ->set_tasks({ "StartAtHome", "StartWithSanity", "SwitchTheme@ToggleSettingsMenu", "StartUpBegin" })
        .set_times_limit("ReturnButton", 0)
        .set_times_limit("StartButton1", 0)
        .set_task_delay(Config.get_options().task_delay * 2)
        .set_retry_times(50);
    m_account_switch_task_ptr->set_retry_times(0);
    m_subtasks.emplace_back(m_start_game_task_ptr)->set_ignore_error(true).set_retry_times(0);
    m_subtasks.emplace_back(m_account_switch_task_ptr);
    m_subtasks.emplace_back(m_start_up_task_ptr);
}

bool asst::StartUpTask::run()
{
    LogTraceFunction;

    if (PackageTask::run()) {
        return true;
    }

    if (!m_restart_on_login_failed || !m_start_game_task_ptr->get_enable()) {
        save_img(utils::path("debug") / utils::path("login"));
        return false;
    }

    Log.warn(__FUNCTION__, "| login failed, entering game-restart loop");
    for (int attempts = 0; attempts < MaxRestartAttempts && !need_exit(); ++attempts) {
        Log.info(__FUNCTION__, "| restarting game client (attempt", attempts + 1, "/", MaxRestartAttempts, ")...");
        if (!m_start_game_task_ptr->restart_game()) {
            Log.warn(__FUNCTION__, "| restart_game failed, retrying...");
            sleep(3000);
            continue;
        }

        Log.info(__FUNCTION__, "| game restarted, retrying login navigation...");
        if (m_start_up_task_ptr->run()) {
            return true;
        }
        Log.warn(__FUNCTION__, "| login navigation failed again, restarting game...");
    }

    save_img(utils::path("debug") / utils::path("login"));
    return false;
}

bool asst::StartUpTask::set_params(const json::value& params)
{
    LogTraceFunction;

    std::string account_name = params.get("account_name", std::string());
    std::string client_type = params.get("client_type", std::string());

    if (!Config.get_package_name(client_type)) {
        return false;
    }
    m_start_game_task_ptr->set_client_type(client_type).set_enable(params.get("start_game_enabled", false));
    m_restart_on_login_failed = params.get("restart_on_login_failed", false);
    m_account_switch_task_ptr->set_enable(!account_name.empty());
    m_account_switch_task_ptr->set_account(std::move(account_name));
    m_account_switch_task_ptr->set_client_type(std::move(client_type));
    return true;
}
