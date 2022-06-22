#include "CloseDownTask.h"

#include <utility>

#include "ProcessTask.h"

asst::CloseDownTask::CloseDownTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType),
    m_stop_game_task_ptr(std::make_shared<StopGameTaskPlugin>(m_callback, m_callback_arg, TaskType)),
    m_close_down_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType))
{
    m_close_down_task_ptr->set_tasks({ "CloseDown" })
        .set_times_limit("ReturnToTerminal", 0)
        .set_times_limit("Terminal", 0)
        .set_times_limit("EndOfAction", 0);
    m_subtasks.emplace_back(m_stop_game_task_ptr);
    m_subtasks.emplace_back(m_close_down_task_ptr);
}

bool asst::CloseDownTask::set_params(const json::value& params)
{
    m_stop_game_task_ptr->set_client_type(params.get("client_type", ""))
        .set_enable(params.get("stop_game_enable", false));
    return true;
}
