#include "StartGameTaskPlugin.h"

#include "Controller/Controller.h"

using namespace asst;

bool StartGameTaskPlugin::start_game_with_retries([[maybe_unused]] size_t pipe_data_size_limit,
                                                  [[maybe_unused]] bool newer_android) const
{
#ifdef __ANDROID__
    // On Android, start_game returns true only after the game is actually started
    return !need_exit() && ctrler()->start_game(m_client_type);
#else
    int extra_runs = 0;
    for (int i = 0; i < 30; ++i) {
        if (need_exit() || !ctrler()->start_game(m_client_type)) {
            return false;
        }

        if (ctrler()->get_pipe_data_size() > pipe_data_size_limit) {
            if (newer_android || ++extra_runs > 3) {
                return true;
            }
        }

        sleep(1500);
    }

    return false;
#endif
}

bool StartGameTaskPlugin::_run()
{
    if (m_client_type.empty()) {
        return false;
    }

    // check for MAC / iOS
    if (ctrler()->get_controller_type() == ControllerType::MacPlayTools) {
        return ctrler()->start_game(m_client_type);
    }

    // check for android version, because it leads to different magic values
    // >8:  167: magic value needs to be >164 but <172 (as that's max)
    // <=8: 145: needs to be >85 but <153 (as that's max)
    // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/8966#issuecomment-2094369694
    // https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/8961#issue-2277568882
    int pipe_data_size_limit = (ctrler()->get_version() > 8) ? 167 : 145;

    // In Android 9 and above, there's a way to check if the game started correctly
    // so no need for limited tries
    bool newer_android = (ctrler()->get_version() > 8);

    return start_game_with_retries(pipe_data_size_limit, newer_android);
}

StartGameTaskPlugin& StartGameTaskPlugin::set_client_type(std::string client_type) noexcept
{
    m_client_type = std::move(client_type);
    return *this;
}
