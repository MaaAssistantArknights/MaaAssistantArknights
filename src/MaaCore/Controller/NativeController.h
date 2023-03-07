#pragma once

#include <optional>
#include <string>
#include <chrono>

namespace asst {
    
    class NativeController
    {
    public:
        virtual std::optional<int> call_command(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                              std::string& sock_data, const int64_t timeout,
                                              const std::chrono::steady_clock::time_point start_time) = 0;

        virtual std::optional<unsigned short> init_socket(const std::string& local_address) = 0;
        virtual void close_socket() noexcept = 0;

        virtual bool call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str) = 0;
        virtual bool input_to_minitouch(const std::string& cmd) = 0;
        virtual void release_minitouch(bool force = false) = 0;

        virtual void release_adb(const std::string &adb_release, int64_t timeout = 20000) = 0;

        bool m_support_socket = false;
    };
}