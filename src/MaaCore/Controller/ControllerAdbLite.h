#pragma once

#include "NativeController.h"

#include "adb-lite/client.hpp"

namespace asst
{
    class ControllerAdbLite : public NativeController
    {
    public:
        ControllerAdbLite(std::shared_ptr<NativeController> controller) { m_controller = controller; };
        ControllerAdbLite(const ControllerAdbLite&) = delete;
        ControllerAdbLite(ControllerAdbLite&&) = delete;
        ~ControllerAdbLite();

        std::optional<int> call_command(const std::string& cmd, const bool recv_by_socket, std::string& pipe_data,
                                        std::string& sock_data, const int64_t timeout,
                                        const std::chrono::steady_clock::time_point start_time) override;

        std::optional<unsigned short> init_socket(const std::string& local_address) override
        {
            return m_controller->init_socket(local_address);
        }
        void close_socket() noexcept override { m_controller->close_socket(); }

        bool call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str) override;
        bool input_to_minitouch(const std::string& cmd) override;
        void release_minitouch(bool force = false) override;

        void release_adb(const std::string& adb_release, int64_t timeout = 20000) override;

        std::shared_ptr<NativeController> m_controller = nullptr;

        std::shared_ptr<adb::client> m_adb_client = nullptr;
        std::shared_ptr<adb::io_handle> m_minitouch_handle = nullptr;

    private:
        static bool remove_quotes(std::string& data);
    };
}
