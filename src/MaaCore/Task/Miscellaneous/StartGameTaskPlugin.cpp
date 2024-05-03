#include "StartGameTaskPlugin.h"

#include "Controller/Controller.h"

using namespace asst;

bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }

    do {
        if (!ctrler()->start_game(m_client_type)) {
            return false;
        }

        if (ctrler()->get_pipe_data_size() > 167) {
            return true;
        }
        sleep(1500);

    } while (true);
}

StartGameTaskPlugin& StartGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}
