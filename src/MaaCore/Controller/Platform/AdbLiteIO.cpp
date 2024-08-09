#include "AdbLiteIO.h"

#include <regex>

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
    std::smatch match;
    std::optional<int> ret;

    static const std::regex devices_regex(R"(^.+ devices$)");
    static const std::regex release_regex(R"(^.+ kill-server$)");
    static const std::regex connect_regex(R"(^.+ connect (\S+)$)");
    static const std::regex shell_regex(R"(^.+ -s \S+ shell (.+)$)");
    static const std::regex exec_regex(R"(^.+ -s \S+ exec-out (.+)$)");
    static const std::regex push_regex(R"#(^.+ -s \S+ push "(.+)" "(.+)"$)#");

    // adb devices
    if (std::regex_match(cmd, devices_regex)) {
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
    if (std::regex_match(cmd, release_regex)) {
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
    // TODO: adb server 尚未实现，第一次连接需要执行一次 adb.exe 启动 daemon
    if (std::regex_match(cmd, match, connect_regex)) {
        m_adb_client = adb::client::create(match[1].str()); // TODO: compare address with existing (if any)

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
    if (std::regex_match(cmd, match, shell_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            ret = std::nullopt;
            goto ret_exit;
        }

        std::string command = match[1].str();
        remove_quotes(command);

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
    if (std::regex_match(cmd, match, exec_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            ret = std::nullopt;
            goto ret_exit;
        }

        std::string command = match[1].str();
        remove_quotes(command);

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
    if (std::regex_match(cmd, match, push_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            ret = std::nullopt;
            goto ret_exit;
        }

        try {
            m_adb_client->push(match[1].str(), match[2].str(), 0644);
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

std::shared_ptr<asst::IOHandler> asst::AdbLiteIO::interactive_shell(const std::string& cmd)
{
    static const std::regex shell_regex(R"(^.+ -s \S+ shell (.+)$)");
    std::smatch match;

    if (std::regex_match(cmd, match, shell_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            return nullptr;
        }

        std::string command = match[1].str();
        remove_quotes(command);

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
    if (m_adb_client) {
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
