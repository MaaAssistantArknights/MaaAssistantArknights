#pragma once

#ifdef _WIN32
#include "NativeController.h"

#include "Utils/Platform/SafeWindows.h"
#include <mswsock.h>

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
    class ControllerWin32 : public NativeController, private InstHelper
    {
    public:
        ControllerWin32(Assistant* inst);
        ControllerWin32(const ControllerWin32&) = delete;
        ControllerWin32(ControllerWin32&&) = delete;
        ~ControllerWin32();

        std::optional<int> call_command(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                        std::string& sock_data, const int64_t timeout,
                                        const std::chrono::steady_clock::time_point start_time) override;

        std::optional<unsigned short> init_socket(const std::string& local_address) override;
        void close_socket() noexcept override;

        bool call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str) override;
        bool input_to_minitouch(const std::string& cmd) override;
        void release_minitouch(bool force = false) override;

        void release_adb(const std::string& adb_release, int64_t timeout = 20000);

        ASST_AUTO_DEDUCED_ZERO_INIT_START
        WSADATA m_wsa_data {};
        SOCKET m_server_sock = INVALID_SOCKET;
        sockaddr_in m_server_sock_addr {};
        LPFN_ACCEPTEX m_server_accept_ex = nullptr;
        ASST_AUTO_DEDUCED_ZERO_INIT_END

        HANDLE m_minitouch_parent_write = INVALID_HANDLE_VALUE;
        ASST_AUTO_DEDUCED_ZERO_INIT_START
        PROCESS_INFORMATION m_minitouch_process_info = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
        ASST_AUTO_DEDUCED_ZERO_INIT_END

    private:
        // for Windows socket
        class WsaHelper : public SingletonHolder<WsaHelper>
        {
            friend class SingletonHolder<WsaHelper>;

        public:
            virtual ~WsaHelper() override { WSACleanup(); }
            bool operator()() const noexcept { return m_supports; }

        private:
            WsaHelper()
            {
                m_supports = WSAStartup(MAKEWORD(2, 2), &m_wsa_data) == 0 && m_wsa_data.wVersion == MAKEWORD(2, 2);
            }
            WSADATA m_wsa_data = { 0 };
            bool m_supports = false;
        };
    };
}
#endif
