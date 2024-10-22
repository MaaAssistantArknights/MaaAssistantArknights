#include "CloseDownTask.h"

#include <utility>

#include "Config/GeneralConfig.h"
#include "Task/Miscellaneous/StopGameTaskPlugin.h"
#include "Task/ProcessTask.h"

asst::CloseDownTask::CloseDownTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_stop_game_task_ptr(std::make_shared<StopGameTaskPlugin>(m_callback, m_inst, TaskType))
{
    m_subtasks.emplace_back(m_stop_game_task_ptr);
}

bool asst::CloseDownTask::set_params(const json::value& params)
{
    std::string client_type = params.get("client_type", std::string());

    m_stop_game_task_ptr->set_client_type(client_type);

    return true;
}
