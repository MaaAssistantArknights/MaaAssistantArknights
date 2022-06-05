#include "StartGameTask.h"

#include "Controller.h"

using namespace asst;

bool StartGameTask::_run()
{
    m_ctrler->start_game(m_server_type);
    return true;
}

StartGameTask& StartGameTask::set_param(ServerType server_type) noexcept
{
    m_server_type = server_type;
    return *this;
}

StartGameTask& StartGameTask::set_enable(bool enable) noexcept
{
    AbstractTask::set_enable(enable);
    return *this;
}
