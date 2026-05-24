#include "AdbLiteIO.h"

#include <boost/regex.hpp>

#include "Utils/Logger.hpp"

std::optional<int> asst::AdbLiteIO::call_command(
    const std::string& cmd,
    bool recv_by_socket,
    std::string& pipe_data,
    std::string& sock_data,
    int64_t timeout,
    std::chrono::steady_clock::time_point start_time)
{
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

        try {
            pipe_data = m_adb_client->shell(command);
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
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

        try {
            pipe_data = m_adb_client->exec(command);
            ret = 0;
            goto ret_exit;
        }
        catch (const std::exception& e) {
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
        ret = NativeIO::call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout, start_time);
    }
    return ret;
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
