#include <fstream>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <processenv.h>
#include <string>
#else
#include <cstdlib>
#endif //  _WIN32

#include <asio.hpp>

#include "client.hpp"
#include "protocol.hpp"

using asio::ip::tcp;
using tcp_endpoints = tcp::resolver::results_type;

using adb::protocol::send_host_request;
using adb::protocol::send_sync_request;

namespace adb
{
    static inline const tcp_endpoints resolve_adb_server(asio::io_context& context)
    {
#ifdef _WIN32
        const auto size = GetEnvironmentVariableA("ANDROID_ADB_SERVER_PORT", nullptr, 0);
        std::string port(size, '\0');
        GetEnvironmentVariableA("ANDROID_ADB_SERVER_PORT", port.data(), size);
        if (len == 0) {
            port = nullptr;
        }
#else
        const char* port = std::getenv("ANDROID_ADB_SERVER_PORT");
#endif //  _WIN32

        tcp::resolver resolver(context);

        if (port) {
            return resolver.resolve("127.0.0.1", port);
        } else {
            return resolver.resolve("127.0.0.1", "5037");
        }
    }

    static inline std::string version(asio::io_context& context, const tcp_endpoints& endpoints)
    {
        tcp::socket socket(context);
        asio::connect(socket, endpoints);

        const auto request = "host:version";
        send_host_request(socket, request);

        return protocol::host_message(socket);
    }

    static inline std::string devices(asio::io_context& context, const tcp_endpoints& endpoints)
    {
        tcp::socket socket(context);
        asio::connect(socket, endpoints);

        const auto request = "host:devices";
        send_host_request(socket, request);

        return protocol::host_message(socket);
    }

    std::string version()
    {
        asio::io_context context;
        const auto endpoints = resolve_adb_server(context);
        return version(context, endpoints);
    }

    std::string devices()
    {
        asio::io_context context;
        const auto endpoints = resolve_adb_server(context);
        return devices(context, endpoints);
    }

    void kill_server()
    {
        asio::io_context context;
        const auto endpoints = resolve_adb_server(context);

        tcp::socket socket(context);
        asio::connect(socket, endpoints);

        const auto request = "host:kill";
        send_host_request(socket, request);
    }

    class io_handle_impl : public io_handle
    {
    public:
        io_handle_impl(std::unique_ptr<asio::io_context> context, tcp::socket socket);
        void write(const std::string_view data) override;
        std::string read(unsigned timeout = 0) override;

    private:
        friend class client_impl;

        const std::unique_ptr<asio::io_context> m_context;
        asio::ip::tcp::socket m_socket;
    };

    io_handle_impl::io_handle_impl(std::unique_ptr<asio::io_context> context, tcp::socket socket)
        : m_context(std::move(context)), m_socket(std::move(socket))
    {
        asio::socket_base::keep_alive option(true);
        m_socket.set_option(option);
    }

    std::string io_handle_impl::read(unsigned timeout)
    {
        std::array<char, 1024> buffer;

        if (timeout == 0) {
            const auto bytes_read = m_socket.read_some(asio::buffer(buffer));
            return std::string(buffer.data(), bytes_read);
        }

        std::error_code ec;
        size_t bytes_read = 0;
        asio::steady_timer timer(*m_context, std::chrono::seconds(timeout));

        m_socket.async_read_some(asio::buffer(buffer), [&](const asio::error_code& error, size_t bytes_transferred) {
            if (error) {
                ec = error;
                return;
            }
            bytes_read = bytes_transferred;
            timer.cancel();
        });

        timer.async_wait([&](const asio::error_code& error) {
            if (error) {
                ec = error;
                return;
            }
            m_socket.cancel(ec);
        });

        m_context->restart();
        while (m_context->run_one()) {
            if (ec == asio::error::eof) {
                break;
            }
            else if (ec == asio::error::operation_aborted) {
                break;
            }
            else if (ec) {
                throw std::system_error(ec);
            }
        }

        return std::string(buffer.data(), bytes_read);
    }

    void io_handle_impl::write(const std::string_view data)
    {
        m_socket.write_some(asio::buffer(data));
    }

    class client_impl : public client
    {
    public:
        client_impl(const std::string_view serial);
        std::string connect() override;
        std::string disconnect() override;
        std::string version() override;
        std::string devices() override;
        std::string shell(const std::string_view command) override;
        std::string exec(const std::string_view command) override;
        bool push(const std::string_view src, const std::string_view dst, int perm) override;
        std::shared_ptr<io_handle> interactive_shell(const std::string_view command) override;
        std::string root() override;
        std::string unroot() override;
        void wait_for_device() override;

    private:
        friend class client;

        std::string m_serial;
        asio::io_context m_context;
        tcp_endpoints m_endpoints;

        /// Switch the connection to the device.
        /**
         * @param socket Opened adb connection.
         * @note Should be used only by the client class.
         * @note Local services (e.g. shell, push) can be requested after this.
         */
        void switch_to_device(asio::ip::tcp::socket& socket);
    };

    std::shared_ptr<client> client::create(const std::string_view serial)
    {
        return std::make_shared<client_impl>(serial);
    }

    client_impl::client_impl(const std::string_view serial)
    {
        m_serial = serial;

        m_endpoints = resolve_adb_server(m_context);
    }

    std::string client_impl::connect()
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        const auto request = "host:connect:" + m_serial;
        send_host_request(socket, request);

        return protocol::host_message(socket);
    }

    std::string client_impl::disconnect()
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        const auto request = "host:disconnect:" + m_serial;
        send_host_request(socket, request);

        return protocol::host_message(socket);
    }

    std::string client_impl::version()
    {
        return adb::version(m_context, m_endpoints);
    }

    std::string client_impl::devices()
    {
        return adb::devices(m_context, m_endpoints);
    }

    std::string client_impl::shell(const std::string_view command)
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        const auto request = std::string("shell:") + command.data();
        send_host_request(socket, request);

        return protocol::host_data(socket);
    }

    std::string client_impl::exec(const std::string_view command)
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        const auto request = std::string("exec:") + command.data();
        send_host_request(socket, request);

        return protocol::host_data(socket);
    }

    bool client_impl::push(const std::string_view src, const std::string_view dst, int perm)
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        // Switch to sync mode
        const auto sync = "sync:";
        send_host_request(socket, sync);

        // SEND request: destination, permissions
        const auto send_request = std::string(dst) + "," + std::to_string(perm);
        const auto request_size = static_cast<uint32_t>(send_request.size());
        send_sync_request(socket, "SEND", request_size, send_request.data());

        // DATA request: file data trunk, trunk size
        std::ifstream file(src.data(), std::ios::binary);
        const auto buf_size = 64000;
        std::array<char, buf_size> buffer;
        while (!file.eof()) {
            file.read(buffer.data(), buf_size);
            const auto bytes_read = static_cast<uint32_t>(file.gcount());
            send_sync_request(socket, "DATA", bytes_read, buffer.data());
        }
        file.close();

        // DONE request: timestamp
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        const auto done_request = protocol::sync_request("DONE", static_cast<uint32_t>(timestamp));
        socket.write_some(asio::buffer(done_request));

        std::string result;
        uint32_t length;
        protocol::sync_response(socket, result, length);
        if (result != "OKAY") {
            return false;
        }

        return true;
    }

    std::string client_impl::root()
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        const auto request = "root:";
        send_host_request(socket, request);

        return protocol::host_data(socket);
    }

    std::string client_impl::unroot()
    {
        tcp::socket socket(m_context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        const auto request = "unroot:";
        send_host_request(socket, request);

        return protocol::host_data(socket);
    }

    std::shared_ptr<io_handle> client_impl::interactive_shell(const std::string_view command)
    {
        auto context = std::make_unique<asio::io_context>();
        tcp::socket socket(*context);
        asio::connect(socket, m_endpoints);

        switch_to_device(socket);

        const auto request = std::string("shell:") + command.data();
        send_host_request(socket, request);

        return std::make_shared<io_handle_impl>(std::move(context), std::move(socket));
    }

    void client_impl::wait_for_device()
    {
        // If adbd restarts, we should wait the device to get offline first.
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const auto pattern = m_serial + "\tdevice";
        while (devices().find(pattern) == std::string::npos) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }

    void client_impl::switch_to_device(tcp::socket& socket)
    {
        const auto request = "host:transport:" + m_serial;
        send_host_request(socket, request);
    }
} // namespace adb
