#pragma once

#ifndef _WIN32
#include <netinet/in.h>

#include "NativeController.h"

#include "InstHelper.h"


namespace asst
{
    class ControllerPosix: public NativeController, private InstHelper
    {
    public:
        ControllerPosix(Assistant* inst);
        ControllerPosix(const ControllerPosix&) = delete;
        ControllerPosix(ControllerPosix&&) = delete;
        ~ControllerPosix();

        std::optional<int> call_command(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                        std::string& sock_data, const int64_t timeout,
                                        const std::chrono::steady_clock::time_point start_time) override;

        std::optional<unsigned short> init_socket(const std::string& local_address) override;
        void close_socket() noexcept override;

        bool call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str) override;
        bool input_to_minitouch(const std::string& cmd) override;
        void release_minitouch(bool force = false) override;

        void release_adb(const std::string &adb_release, int64_t timeout = 20000) override;

        int m_server_sock = -1;
        sockaddr_in m_server_sock_addr {};
        static constexpr int PIPE_READ = 0;
        static constexpr int PIPE_WRITE = 1;
        int m_pipe_in[2] = { 0 };
        int m_pipe_out[2] = { 0 };
        int m_child = 0;

        ::pid_t m_minitouch_process = -1;
        int m_write_to_minitouch_fd = -1;
    };
}
#endif
