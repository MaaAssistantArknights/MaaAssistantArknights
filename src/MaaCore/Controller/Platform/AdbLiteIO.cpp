#include "AdbLiteIO.h"

#include <boost/regex.hpp>

#include "../adb-lite/stage.hpp"
#include "Utils/Logger.hpp"

std::optional<int> asst::AdbLiteIO::call_command(
    const std::string& cmd,
    bool recv_by_socket,
    std::string& pipe_data,
    std::string& sock_data,
    int64_t timeout,
    std::chrono::steady_clock::time_point start_time)
{
    // Do not let a timed-out probe continue with another screenshot method.
    if (m_screencap_cancelled.load(std::memory_order_acquire) && cmd.find("screencap") != std::string::npos) {
        adb::stage::set("screencap-cancelled");
        return -1;
    }

    // TODO: 从上面的 call_command_win32/posix 里抽取出 socket 接收的部分
    if (recv_by_socket) {
        Log.error("adb-lite does not support receiving data from socket");
        sock_data.clear();
        return std::nullopt;
    }

    // TODO: 实现 timeout，目前暂时忽略
    std::ignore = timeout;
    std::ignore = start_time;
    boost::smatch match;
    std::optional<int> ret;

    static const boost::regex devices_regex(R"(^.+ devices$)");
    static const boost::regex release_regex(R"(^.+ kill-server$)");
    static const boost::regex connect_regex(R"(^.+ connect (\S+)$)");
    static const boost::regex shell_regex(R"(^.+ -s (\S+) shell (.+)$)");
    static const boost::regex exec_regex(R"(^.+ -s (\S+) exec-out (.+)$)");
    static const boost::regex push_regex(R"#(^.+ -s (\S+) push "(.+)" "(.+)"$)#");

    const auto set_active_screencap_client = [&] {
        std::lock_guard lock(m_active_screencap_client_mutex);
        m_active_screencap_client = m_adb_client;
    };
    const auto clear_active_screencap_client = [&] {
        std::lock_guard lock(m_active_screencap_client_mutex);
        m_active_screencap_client.reset();
    };

    // adb devices
    if (boost::regex_match(cmd, devices_regex)) {
        try {
            pipe_data = adb::devices();
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            Log.error("adb devices failed:", e.what());
            ret = std::nullopt;
            goto ret_exit;
        }
    }

    // adb kill-server
    if (boost::regex_match(cmd, release_regex)) {
        try {
            adb::kill_server();
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            Log.error("adb kill-server failed:", e.what());
            ret = std::nullopt;
            goto ret_exit;
        }
    }

    // adb connect
    if (boost::regex_match(cmd, match, connect_regex)) {
        const std::string serial = match[1].str();
        auto lock = lock_adb_client(serial);
        if (!lock) {
            ret = std::nullopt;
            goto ret_exit;
        }

        try {
            pipe_data = m_adb_client->connect();
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            adb::stage::set("adb-connect-error");
            Log.error("adb connect failed:", e.what());
            // fallback 到 fork adb 进程的方式
            ret = std::nullopt;
            goto ret_exit;
        }
    }

    // adb shell
    if (boost::regex_match(cmd, match, shell_regex)) {
        const std::string serial = match[1].str();
        std::string command = match[2].str();
        remove_quotes(command);

        auto lock = lock_adb_client(serial);
        if (!lock) {
            ret = std::nullopt;
            goto ret_exit;
        }

        const bool is_screencap = command.find("screencap") != std::string::npos;
        if (is_screencap) {
            set_active_screencap_client();
        }
        try {
            pipe_data = m_adb_client->shell(command);
            if (is_screencap) {
                clear_active_screencap_client();
            }
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            if (is_screencap) {
                clear_active_screencap_client();
            }
            Log.error("adb shell failed:", e.what());
            ret = -1;
            goto ret_exit;
        }
    }

    // adb exec-out
    if (boost::regex_match(cmd, match, exec_regex)) {
        const std::string serial = match[1].str();
        std::string command = match[2].str();
        remove_quotes(command);

        auto lock = lock_adb_client(serial);
        if (!lock) {
            ret = std::nullopt;
            goto ret_exit;
        }

        const bool is_screencap = command.find("screencap") != std::string::npos;
        if (is_screencap) {
            set_active_screencap_client();
        }
        try {
            pipe_data = m_adb_client->exec(command);
            if (is_screencap) {
                clear_active_screencap_client();
            }
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            if (is_screencap) {
                clear_active_screencap_client();
            }
            Log.error("adb exec-out failed:", e.what());
            ret = -1;
            goto ret_exit;
        }
    }

    // adb push
    if (boost::regex_match(cmd, match, push_regex)) {
        const std::string serial = match[1].str();
        auto lock = lock_adb_client(serial);
        if (!lock) {
            ret = std::nullopt;
            goto ret_exit;
        }

        try {
            m_adb_client->push(match[2].str(), match[3].str(), 0644);
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
            Log.error("adb push failed:", e.what());
            ret = -1;
            goto ret_exit;
        }
    }

    Log.info("adb-lite does not support command:", cmd);
    ret = std::nullopt;

ret_exit:
    if (!ret) {
        Log.warn("adb-lite command: \"", cmd, "\"run failed");
        Log.warn("fallback to NativeIO");
        adb::stage::set("native-fallback");
        ret = NativeIO::call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout, start_time);
    }
    return ret;
}

void asst::AdbLiteIO::cancel_screencap() noexcept
{
    m_screencap_cancelled.store(true, std::memory_order_release);
    std::lock_guard lock(m_active_screencap_client_mutex);
    if (m_active_screencap_client) {
        m_active_screencap_client->cancel_active_io();
    }
}

bool asst::AdbLiteIO::reset_screencap()
{
    std::lock_guard lock(m_adb_client_mutex);
    if (m_adb_serial.empty()) {
        m_screencap_cancelled.store(false, std::memory_order_release);
        return false;
    }
    try {
        m_adb_client = adb::client::create(m_adb_serial);
        m_screencap_cancelled.store(false, std::memory_order_release);
        return true;
    }
    catch (const std::exception& e) {
        Log.error("failed to recreate adb-lite client after screencap timeout:", e.what());
        m_adb_client.reset();
        m_screencap_cancelled.store(false, std::memory_order_release);
        return false;
    }
}

std::string asst::AdbLiteIO::screencap_stage() const
{
    return adb::stage::last();
}

std::optional<std::unique_lock<std::mutex>> asst::AdbLiteIO::lock_adb_client(std::string_view serial)
{
    std::unique_lock lock(m_adb_client_mutex);
    if (!m_adb_client || m_adb_serial != serial) {
        Log.error("adb client not initialized for serial:", std::string(serial), "current:", m_adb_serial);
        return std::nullopt;
    }

    return std::move(lock);
}

void asst::AdbLiteIO::set_adb_serial(std::string_view serial)
{
    std::lock_guard lock(m_adb_client_mutex);
    const std::string serial_str(serial);
    if (m_adb_client && m_adb_serial == serial_str) {
        return;
    }

    try {
        auto adb_client = adb::client::create(serial_str);
        m_adb_serial = serial_str;
        m_adb_client = std::move(adb_client);
        m_screencap_cancelled.store(false, std::memory_order_release);
    }
    catch (const std::exception& e) {
        Log.error("failed to create adb-lite client for serial:", serial_str, e.what());
        m_adb_serial.clear();
        m_adb_client.reset();
    }
}

std::shared_ptr<asst::IOHandler> asst::AdbLiteIO::interactive_shell(const std::string& cmd)
{
    static const boost::regex shell_regex(R"(^.+ -s (\S+) shell (.+)$)");
    boost::smatch match;

    if (boost::regex_match(cmd, match, shell_regex)) {
        const std::string serial = match[1].str();
        std::string command = match[2].str();
        remove_quotes(command);

        auto lock = lock_adb_client(serial);
        if (!lock) {
            return nullptr;
        }

        try {
            return std::make_shared<IOHandlerAdbLite>(m_adb_client->interactive_shell(command));
        }
        catch (const std::exception& e) {
            Log.error("adb shell failed:", e.what());
            return nullptr;
        }
    }
    else {
        Log.error("unknown command to call interactive shell:", cmd);
        return nullptr;
    }
}

void asst::AdbLiteIO::release_adb(const std::string& adb_release, int64_t timeout)
{
    bool has_adb_client = false;
    {
        std::lock_guard lock(m_adb_client_mutex);
        has_adb_client = static_cast<bool>(m_adb_client);
    }

    if (has_adb_client) {
        std::string pipe_data;
        std::string sock_data;
        auto start_time = std::chrono::steady_clock::now();

        call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
    }
}

bool asst::AdbLiteIO::remove_quotes(std::string& data)
{
    if (data.size() < 2) {
        return false;
    }

    if (data.front() == '"' && data.back() == '"') {
        data.erase(data.begin());
        data.pop_back();
        return true;
    }

    return false;
}

bool asst::IOHandlerAdbLite::write(std::string_view data)
{
    try {
        m_handle->write(data);
        return true;
    }
    catch (const std::exception& e) {
        Log.error("IOHandler write failed:", e.what());
        return false;
    }
}

std::string asst::IOHandlerAdbLite::read(unsigned timeout_sec)
{
    try {
        return m_handle->read(timeout_sec);
    }
    catch (const std::exception& e) {
        Log.error("IOHandler read failed:", e.what());
        return {};
    }
}
