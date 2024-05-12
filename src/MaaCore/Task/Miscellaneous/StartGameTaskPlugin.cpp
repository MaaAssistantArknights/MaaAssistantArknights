#include "StartGameTaskPlugin.h"

#include "Controller/Controller.h"

using namespace asst;

bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }
    // check for MAC / iOS
    if (ctrler()->get_controller_type() == ControllerType::MacPlayTools) {
        return ctrler()->start_game(m_client_type);
    }
    else {
        // check for android version, because it leads to different magic values
        // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/8966#issuecomment-2094369694
        if (ctrler()->get_version() > 8) {
            do {
                if (!ctrler()->start_game(m_client_type)) {
                    return false;
                }

                // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/8961#issue-2277568882
                // 167: magic value needs to be >164 but <172 (as that's max)
                if (ctrler()->get_pipe_data_size() > 167) {
                    return true;
                }

                sleep(1500);

            } while (true);
        }
        else {
            do {
                if (!ctrler()->start_game(m_client_type)) {
                    return false;
                }

                // 145: needs to be >85 but <153 (as that's max)
                // magic value lowered because of different android version
                if (ctrler()->get_pipe_data_size() > 145) {
                    // As there's no way to know (in Android < 8) if Ark has correctly started
                    // we run the command a few more times to be sure
                    for (auto _ = 3; _--;) {
                        sleep(1500);
                        if (!ctrler()->start_game(m_client_type)) {
                            return false;
                        }
                    }
                    return true;
                }

                sleep(1500);

            } while (true);
        }
    }
}

StartGameTaskPlugin& StartGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}
