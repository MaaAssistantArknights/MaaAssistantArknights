#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <boost/asio.hpp>

#include "client.hpp"
#include "protocol.hpp"
#include "stage.hpp"

#include "Utils/Platform.hpp"

using boost::asio::ip::tcp;
using tcp_endpoints = tcp::resolver::results_type;

using adb::protocol::send_host_request;
using adb::protocol::send_sync_request;

namespace adb
{
static inline std::string version(boost::asio::io_context& context, const tcp_endpoints& endpoints)
{
    tcp::socket socket(context);
    stage::set("adb-server-tcp-connect");
    boost::asio::connect(socket, endpoints);

    const auto request = "host:version";
    stage::set("send-host-version-request");
    send_host_request(socket, request);

    return protocol::host_message(socket);
}

static inline std::string devices(boost::asio::io_context& context, const tcp_endpoints& endpoints)
{
    tcp::socket socket(context);
    stage::set("adb-server-tcp-connect");
    boost::asio::connect(socket, endpoints);

    const auto request = "host:devices";
    stage::set("send-host-devices-request");
    send_host_request(socket, request);

    return protocol::host_message(socket);
}

std::string version()
{
    boost::asio::io_context context;
    tcp::resolver resolver(context);
    const auto endpoints = resolver.resolve("127.0.0.1", "5037");
    return version(context, endpoints);
}

std::string devices()
{
    boost::asio::io_context context;
    tcp::resolver resolver(context);
    const auto endpoints = resolver.resolve("127.0.0.1", "5037");
    return devices(context, endpoints);
}

void kill_server()
{
    boost::asio::io_context context;
    tcp::resolver resolver(context);
    const auto endpoints = resolver.resolve("127.0.0.1", "5037");

    tcp::socket socket(context);
    boost::asio::connect(socket, endpoints);

    const auto request = "host:kill";
    send_host_request(socket, request);
}

class io_handle_impl : public io_handle
{
public:
    io_handle_impl(std::unique_ptr<boost::asio::io_context> context, tcp::socket socket);
    void write(const std::string_view data) override;
    std::string read(unsigned timeout = 0) override;

private:
    friend class client_impl;

    const std::unique_ptr<boost::asio::io_context> m_context;
    boost::asio::ip::tcp::socket m_socket;
};

io_handle_impl::io_handle_impl(std::unique_ptr<boost::asio::io_context> context, tcp::socket socket) :
    m_context(std::move(context)),
    m_socket(std::move(socket))
{
    boost::asio::socket_base::keep_alive option(true);
    m_socket.set_option(option);
}

std::string io_handle_impl::read(unsigned timeout)
{
    std::array<char, 1024> buffer;

    if (timeout == 0) {
        const auto bytes_read = m_socket.read_some(boost::asio::buffer(buffer));
        return std::string(buffer.data(), bytes_read);
    }

    boost::system::error_code ec;
    size_t bytes_read = 0;
    boost::asio::steady_timer timer(*m_context, std::chrono::seconds(timeout));

    m_socket.async_read_some(
        boost::asio::buffer(buffer),
        [&](const boost::system::error_code& error, size_t bytes_transferred) {
            if (error) {
                ec = error;
                return;
            }
            bytes_read = bytes_transferred;
            timer.cancel();
        });

    timer.async_wait([&](const boost::system::error_code& error) {
        if (error) {
            ec = error;
            return;
        }
        try {
            m_socket.cancel();
        }
        catch (const boost::system::system_error& e) {
            ec = e.code();
        }
    });

    m_context->restart();
    while (m_context->run_one()) {
        if (ec == boost::asio::error::eof) {
            break;
        }
        else if (ec == boost::asio::error::operation_aborted) {
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
    m_socket.write_some(boost::asio::buffer(data));
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
    void cancel_active_io() noexcept override;

private:
    friend class client;

    struct active_socket_scope
    {
        active_socket_scope(client_impl& owner, std::shared_ptr<tcp::socket> socket) :
            owner(owner),
            socket(std::move(socket))
        {
            std::lock_guard lock(owner.m_active_socket_mutex);
            owner.m_active_socket = this->socket;
        }

        void publish_native_handle()
        {
            std::lock_guard lock(owner.m_active_socket_mutex);
            if (socket->is_open()) {
                owner.m_active_socket_handle = socket->native_handle();
            }
        }

        ~active_socket_scope()
        {
            std::lock_guard lock(owner.m_active_socket_mutex);
            owner.m_active_socket_handle.reset();
            if (owner.m_active_socket == socket) {
                owner.m_active_socket.reset();
            }
        }

        client_impl& owner;
        std::shared_ptr<tcp::socket> socket;
    };

    std::string m_serial;
    boost::asio::io_context m_context;
    tcp_endpoints m_endpoints;
    std::mutex m_active_socket_mutex;
    std::shared_ptr<tcp::socket> m_active_socket;
    std::optional<tcp::socket::native_handle_type> m_active_socket_handle;
    std::atomic_bool m_cancel_requested = false;

    void connect_active_socket(tcp::socket& socket);

    /// Switch the connection to the device.
    /**
     * @param socket Opened adb connection.
     * @note Should be used only by the client class.
     * @note Local services (e.g. shell, push) can be requested after this.
     */
    void switch_to_device(boost::asio::ip::tcp::socket& socket);
};

std::shared_ptr<client> client::create(const std::string_view serial)
{
    return std::make_shared<client_impl>(serial);
}

client_impl::client_impl(const std::string_view serial)
{
    m_serial = serial;

    tcp::resolver resolver(m_context);
    m_endpoints = resolver.resolve("127.0.0.1", "5037");
}

void client_impl::connect_active_socket(tcp::socket& socket)
{
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        m_context.restart();
        m_context.poll();
        throw boost::system::system_error(boost::asio::error::operation_aborted);
    }

    boost::system::error_code result;
    bool completed = false;
    boost::asio::async_connect(socket, m_endpoints, [&](const boost::system::error_code& error, const tcp::endpoint&) {
        result = error;
        completed = true;
    });

    m_context.restart();
    while (!completed) {
        m_context.run_one();
    }
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        m_context.restart();
        m_context.poll();
        throw boost::system::system_error(boost::asio::error::operation_aborted);
    }
    if (result) {
        throw boost::system::system_error(result);
    }
}

std::string client_impl::connect()
{
    tcp::socket socket(m_context);
    stage::set("adb-server-tcp-connect");
    boost::asio::connect(socket, m_endpoints);

    const auto request = "host:connect:" + m_serial;
    stage::set("send-host-connect-request");
    send_host_request(socket, request);

    stage::set("wait-host-connect-message");
    return protocol::host_message(socket);
}

std::string client_impl::disconnect()
{
    tcp::socket socket(m_context);
    boost::asio::connect(socket, m_endpoints);

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
    auto socket = std::make_shared<tcp::socket>(m_context);
    active_socket_scope active_socket(*this, socket);
    stage::set("adb-server-tcp-connect");
    connect_active_socket(*socket);
    active_socket.publish_native_handle();
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        m_context.restart();
        m_context.poll();
        throw boost::system::system_error(boost::asio::error::operation_aborted);
    }

    stage::set("switch-adb-transport");
    switch_to_device(*socket);

    const auto request = std::string("shell:") + command.data();
    stage::set("send-shell-request");
    send_host_request(*socket, request);

    stage::set("wait-shell-output-eof");
    return protocol::host_data(*socket);
}

std::string client_impl::exec(const std::string_view command)
{
    auto socket = std::make_shared<tcp::socket>(m_context);
    active_socket_scope active_socket(*this, socket);
    stage::set("adb-server-tcp-connect");
    connect_active_socket(*socket);
    active_socket.publish_native_handle();
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        m_context.restart();
        m_context.poll();
        throw boost::system::system_error(boost::asio::error::operation_aborted);
    }

    stage::set("switch-adb-transport");
    switch_to_device(*socket);

    const auto request = std::string("exec:") + command.data();
    stage::set("send-exec-request");
    send_host_request(*socket, request);

    stage::set("wait-exec-output-eof");
    return protocol::host_data(*socket);
}

bool client_impl::push(const std::string_view src, const std::string_view dst, int perm)
{
    // bail out if source file cannot be opened
    std::ifstream file(std::filesystem::path(asst::platform::to_osstring(std::string(src))), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    tcp::socket socket(m_context);
    boost::asio::connect(socket, m_endpoints);

    switch_to_device(socket);

    // Switch to sync mode
    const auto sync = "sync:";
    send_host_request(socket, sync);

    // SEND request: destination, permissions
    const auto send_request = std::string(dst) + "," + std::to_string(perm);
    const auto request_size = static_cast<uint32_t>(send_request.size());
    send_sync_request(socket, "SEND", request_size, send_request.data());

    // DATA request: file data trunk, trunk size
    const auto buf_size = 64'000;
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
    socket.write_some(boost::asio::buffer(done_request));

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
    boost::asio::connect(socket, m_endpoints);

    switch_to_device(socket);

    const auto request = "root:";
    send_host_request(socket, request);

    return protocol::host_data(socket);
}

std::string client_impl::unroot()
{
    tcp::socket socket(m_context);
    boost::asio::connect(socket, m_endpoints);

    switch_to_device(socket);

    const auto request = "unroot:";
    send_host_request(socket, request);

    return protocol::host_data(socket);
}

std::shared_ptr<io_handle> client_impl::interactive_shell(const std::string_view command)
{
    auto context = std::make_unique<boost::asio::io_context>();
    tcp::socket socket(*context);
    boost::asio::connect(socket, m_endpoints);

    switch_to_device(socket);

    const auto request = std::string("shell:") + command.data();
    send_host_request(socket, request);

    return std::make_shared<io_handle_impl>(std::move(context), std::move(socket));
}

void client_impl::cancel_active_io() noexcept
{
    std::optional<tcp::socket::native_handle_type> socket_handle;
    {
        std::lock_guard lock(m_active_socket_mutex);
        m_cancel_requested.store(true, std::memory_order_release);
        if (m_active_socket && !m_active_socket_handle) {
            const auto socket = m_active_socket;
            boost::asio::post(m_context, [socket] {
                boost::system::error_code ignored;
                socket->cancel(ignored);
            });
        }
        socket_handle = m_active_socket_handle;
    }

    if (!socket_handle) {
        return;
    }
#ifdef _WIN32
    ::shutdown(*socket_handle, SD_BOTH);
#else
    ::shutdown(*socket_handle, SHUT_RDWR);
#endif
}

void client_impl::wait_for_device()
{
    // If adbd restarts, we should wait the device to get offline first.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const auto pattern = m_serial + "\tdevice";
    while (devices().find(pattern) == std::string::npos) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void client_impl::switch_to_device(tcp::socket& socket)
{
    const auto request = "host:transport:" + m_serial;
    send_host_request(socket, request);
}
} // namespace adb
