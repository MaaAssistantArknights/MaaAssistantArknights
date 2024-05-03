#include "StartGameTaskPlugin.h"

#include "Controller/Controller.h"

using namespace asst;

bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }

    for (int i = 0; i < 4; i++) {
        if (!ctrler()->start_game(m_client_type)) {
            return false;
        }
        sleep(2000);
    }

    return true;
}

StartGameTaskPlugin& StartGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}
