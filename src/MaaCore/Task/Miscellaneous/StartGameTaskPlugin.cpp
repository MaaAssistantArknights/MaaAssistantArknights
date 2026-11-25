#include "StartGameTaskPlugin.h"

#include "Controller/Controller.h"
#include "Utils/Logger.hpp"

using namespace asst;

bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }

    // StartUpTask verifies login readiness and owns bounded client restarts.
    return !need_exit() && ctrler()->start_game(m_client_type);
}

StartGameTaskPlugin& StartGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}

bool StartGameTaskPlugin::restart_game()
{
    if (m_client_type.empty()) {
        Log.error(__FUNCTION__, "| client_type is empty, cannot restart game");
        return false;
    }
    if (!ctrler()) {
        Log.error(__FUNCTION__, "| controller is not initialized, cannot restart game");
        return false;
    }
    Log.info(__FUNCTION__, "| stopping game client");
    ctrler()->stop_game(m_client_type);
    sleep(3000);
    Log.info(__FUNCTION__, "| starting game client");
    return _run();
}
