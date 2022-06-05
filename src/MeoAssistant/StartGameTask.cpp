#include "StartGameTask.h"

#include "Controller.h"

using namespace asst;

bool StartGameTask::_run()
{
    if (m_client_type.empty()) {
        return false;
    }
    return m_ctrler->start_game(m_client_type).has_value();
}

StartGameTask& StartGameTask::set_param(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}

StartGameTask& StartGameTask::set_enable(bool enable) noexcept
{
    AbstractTask::set_enable(enable);
    return *this;
}
