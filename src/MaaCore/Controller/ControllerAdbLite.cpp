#include "ControllerAdbLite.h"

#include <regex>

#include "Utils/Logger.hpp"

asst::ControllerAdbLite::~ControllerAdbLite()
{
    close_socket();
    release_minitouch(true);
}

std::optional<int> asst::ControllerAdbLite::call_command(const std::string& cmd, const bool recv_by_socket,
                                                         std::string& pipe_data, std::string& sock_data,
                                                         const int64_t timeout,
                                                         const std::chrono::steady_clock::time_point start_time)
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
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb devices failed:", e.what());
            return std::nullopt;
        }
    }

    // adb kill-server
    if (std::regex_match(cmd, release_regex)) {
        try {
            adb::kill_server();
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb kill-server failed:", e.what());
            return std::nullopt;
        }
    }

    std::smatch match;

    // adb connect
    // TODO: adb server 尚未实现，第一次连接需要执行一次 adb.exe 启动 daemon
    if (std::regex_match(cmd, match, connect_regex)) {

        m_adb_client = adb::client::create(match[1].str()); // TODO: compare address with existing (if any)

        try {
            pipe_data = m_adb_client->connect();
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb connect failed:", e.what());
            // fallback 到 fork adb 进程的方式
            return std::nullopt;
        }
    }

    // adb shell
    if (std::regex_match(cmd, match, shell_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            return std::nullopt;
        }

        std::string command = match[1].str();
        remove_quotes(command);

        try {
            pipe_data = m_adb_client->shell(command);
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb shell failed:", e.what());
            return -1;
        }
    }

    // adb exec-out
    if (std::regex_match(cmd, match, exec_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            return std::nullopt;
        }

        std::string command = match[1].str();
        remove_quotes(command);

        try {
            pipe_data = m_adb_client->exec(command);
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb exec-out failed:", e.what());
            return -1;
        }
    }

    // adb push
    if (std::regex_match(cmd, match, push_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            return std::nullopt;
        }

        try {
            m_adb_client->push(match[1].str(), match[2].str(), 0644);
            return 0;
        }
        catch (const std::exception& e) {
            Log.error("adb push failed:", e.what());
            return -1;
        }
    }

    Log.info("adb-lite does not support command:", cmd);
    return std::nullopt;
}

bool asst::ControllerAdbLite::call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str)
{
    static const std::regex shell_regex(R"(^.+ -s \S+ shell (.+)$)");
    std::smatch match;

    if (std::regex_match(cmd, match, shell_regex)) {
        if (!m_adb_client) {
            Log.error("adb client not initialized");
            return false;
        }

        std::string command = match[1].str();
        remove_quotes(command);

        try {
            m_minitouch_handle = m_adb_client->interactive_shell(command);
        }
        catch (const std::exception& e) {
            Log.error("adb shell failed:", e.what());
            release_minitouch(true);
            return false;
        }
    }
    else {
        Log.error("unknown command to call minitouch:", cmd);
        return false;
    }

    while (true) {
        std::string buffer;
        try {
            buffer += m_minitouch_handle->read(timeout);
        }
        catch (const std::exception& e) {
            Log.error("read minitouch handle failed", e.what());
            release_minitouch(true);
            return false;
        }
        // Timeout
        if (buffer.empty()) {
            Log.info("unable to find $ from pipe_str:", Logger::separator::newline, pipe_str);
            release_minitouch(true);
            return false;
        }
        // Read new data
        pipe_str += buffer;
        if (pipe_str.find('$') != std::string::npos) {
            break;
        }
    }

    return true;
}

bool asst::ControllerAdbLite::input_to_minitouch(const std::string& cmd)
{
    if (m_minitouch_handle == nullptr) {
        Log.error("Minitouch connect not active");
        return false;
    }

    try {
        m_minitouch_handle->write(cmd);
    }
    catch (const std::exception& e) {
        Log.error("Failed to write to minitouch", e.what());
        return false;
    }

    return true;
}

void asst::ControllerAdbLite::release_minitouch([[maybe_unused]] bool force)
{
    m_minitouch_handle = nullptr;
}

void asst::ControllerAdbLite::release_adb(const std::string& adb_release, int64_t timeout)
{
    if (m_adb_client) {
        std::string pipe_data;
        std::string sock_data;
        auto start_time = std::chrono::steady_clock::now();

        call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
    }
}

bool asst::ControllerAdbLite::remove_quotes(std::string& data)
{
    if (data.size() < 2) return false;

    if (data.front() == '"' && data.back() == '"') {
        data.erase(data.begin());
        data.pop_back();
        return true;
    }

    return false;
}
