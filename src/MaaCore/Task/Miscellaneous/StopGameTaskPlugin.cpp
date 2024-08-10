#include "StopGameTaskPlugin.h"

#include "Controller/Controller.h"

using namespace asst;

bool StopGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }
    return ctrler()->stop_game(m_client_type);
}

StopGameTaskPlugin& StopGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}
