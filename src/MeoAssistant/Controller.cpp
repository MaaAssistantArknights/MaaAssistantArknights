#include "Controller.h"
#include "Utils/AsstConf.h"
#include "Utils/Platform/AsstPlatform.h"

#ifdef _WIN32
#include "Utils/Platform/AsstPlatformWin32.h"
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "Utils/NoWarningCV.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4068)
#endif
#include <zlib/decompress.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Resource/GeneralConfiger.h"
#include "Utils/AsstTypes.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

struct MinitouchCmd
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
    inline static std::string reset() { return "r\n"; }
    inline static std::string commit() { return "c\n"; }
    inline static std::string down(int x, int y, bool with_commit = true, int wait_ms = 0, int contact = 0,
                                   int pressure = 1)
    {
        // mac 编不过
        // std::string str = std::format("d {} {} {} {}\n", contact, x, y, pressure);

        char buff[64] = { 0 };
        sprintf(buff, "d %d %d %d %d\n", contact, x, y, pressure);
        std::string str = buff;

        if (with_commit) str += commit();
        if (wait_ms) str += wait(wait_ms);
        return str;
    }
    inline static std::string move(int x, int y, bool with_commit = true, int wait_ms = 0, int contact = 0,
                                   int pressure = 1)
    {
        char buff[64] = { 0 };
        sprintf(buff, "m %d %d %d %d\n", contact, x, y, pressure);
        std::string str = buff;

        if (with_commit) str += commit();
        if (wait_ms) str += wait(wait_ms);
        return str;
    }
    inline static std::string up(bool with_commit = true, int contact = 0)
    {
        char buff[16] = { 0 };
        sprintf(buff, "u %d\n", contact);
        std::string str = buff;

        if (with_commit) str += commit();
        return str;
    }
    inline static std::string wait(int ms)
    {
        char buff[16] = { 0 };
        sprintf(buff, "w %d\n", ms);
        std::string str = buff;

        return str;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

asst::Controller::Controller(AsstCallback callback, void* callback_arg)
    : m_callback(std::move(callback)), m_callback_arg(callback_arg), m_rand_engine(std::random_device {}())
{
    LogTraceFunction;

#ifdef _WIN32
    m_support_socket = WsaHelper::get_instance()();
#else
    int pipe_in_ret = ::pipe(m_pipe_in);
    int pipe_out_ret = ::pipe(m_pipe_out);
    ::fcntl(m_pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);

    if (pipe_in_ret < 0 || pipe_out_ret < 0) {
        Log.error(__FUNCTION__, "controller pipe created failed", pipe_in_ret, pipe_out_ret);
    }
    m_support_socket = true;
#endif

    if (!m_support_socket) {
        Log.error("sokcet not supports");
    }
}

asst::Controller::~Controller()
{
    LogTraceFunction;

    release_minitouch();
    make_instance_inited(false);
    kill_adb_daemon();

#ifndef _WIN32
    ::close(m_pipe_in[PIPE_READ]);
    ::close(m_pipe_in[PIPE_WRITE]);
    ::close(m_pipe_out[PIPE_READ]);
    ::close(m_pipe_out[PIPE_WRITE]);
#endif
}

std::pair<int, int> asst::Controller::get_scale_size() const noexcept
{
    return m_scale_size;
}

bool asst::Controller::need_exit() const
{
    return m_exit_flag != nullptr && *m_exit_flag;
}

std::optional<std::string> asst::Controller::call_command(const std::string& cmd, int64_t timeout, bool allow_reconnect,
                                                          bool recv_by_socket)
{
    using namespace std::chrono_literals;
    using namespace std::chrono;
    // LogTraceScope(std::string(__FUNCTION__) + " | `" + cmd + "`");

    std::string pipe_data;
    std::string sock_data;
    asst::platform::single_page_buffer<char> pipe_buffer;
    std::optional<asst::platform::single_page_buffer<char>> sock_buffer;

    auto start_time = steady_clock::now();
    std::unique_lock<std::mutex> callcmd_lock(m_callcmd_mutex);

#ifdef _WIN32

    DWORD err = 0;
    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa_inherit { .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_inherit,
                                             (DWORD)pipe_buffer.size(), true, false)) {
        err = GetLastError();
        Log.error("CreateOverlappablePipe failed, err", err);
        return std::nullopt;
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;
    ASST_AUTO_DEDUCED_ZERO_INIT_START
    PROCESS_INFORMATION process_info = { nullptr }; // 进程信息结构体
    ASST_AUTO_DEDUCED_ZERO_INIT_END

    auto cmdline_osstr = asst::utils::to_osstring(cmd);
    BOOL create_ret =
        CreateProcessW(nullptr, cmdline_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &process_info);
    if (!create_ret) {
        Log.error("Call `", cmd, "` create process failed, ret", create_ret);
        return std::nullopt;
    }

    CloseHandle(pipe_child_write);
    pipe_child_write = INVALID_HANDLE_VALUE;

    std::vector<HANDLE> wait_handles;
    wait_handles.reserve(3);
    bool process_running = true;
    bool pipe_eof = false;
    bool accept_pending = false;
    bool socket_eof = false;

    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    (void)ReadFile(pipe_parent_read, pipe_buffer.get(), (DWORD)pipe_buffer.size(), nullptr, &pipeov);

    OVERLAPPED sockov {};
    SOCKET client_socket = INVALID_SOCKET;

    if (recv_by_socket) {
        sock_buffer = asst::platform::single_page_buffer<char>();
        sockov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        DWORD dummy;
        if (!m_server_accept_ex(m_server_sock, client_socket, sock_buffer.value().get(),
                                (DWORD)sock_buffer.value().size() - ((sizeof(sockaddr_in) + 16) * 2),
                                sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dummy, &sockov)) {
            err = WSAGetLastError();
            if (err == ERROR_IO_PENDING) {
                accept_pending = true;
            }
            else {
                Log.trace("AcceptEx failed, err:", err);
                accept_pending = false;
                socket_eof = true;
                ::closesocket(client_socket);
            }
        }
    }

    while (!need_exit()) {
        wait_handles.clear();
        if (process_running) wait_handles.push_back(process_info.hProcess);
        if (!pipe_eof) wait_handles.push_back(pipeov.hEvent);
        if (recv_by_socket && ((accept_pending && process_running) || !socket_eof)) {
            wait_handles.push_back(sockov.hEvent);
        }
        if (wait_handles.empty()) break;
        auto elapsed = steady_clock::now() - start_time;
        // TODO: 这里目前是隔 5000ms 判断一次，应该可以加一个 wait_handle 来判断外部中断（need_exit）
        auto wait_time =
            (std::min)(timeout - duration_cast<milliseconds>(elapsed).count(), process_running ? 5LL * 1000 : 0LL);
        if (wait_time < 0) break;
        auto wait_result =
            WaitForMultipleObjectsEx((DWORD)wait_handles.size(), &wait_handles[0], FALSE, (DWORD)wait_time, TRUE);
        HANDLE signaled_object = INVALID_HANDLE_VALUE;
        if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + wait_handles.size()) {
            signaled_object = wait_handles[(size_t)wait_result - WAIT_OBJECT_0];
        }
        else if (wait_result == WAIT_TIMEOUT) {
            if (wait_time == 0) {
                std::vector<std::string> handle_string {};
                for (auto handle : wait_handles) {
                    if (handle == process_info.hProcess) {
                        handle_string.emplace_back("process_info.hProcess");
                    }
                    else if (handle == pipeov.hEvent) {
                        handle_string.emplace_back("pipeov.hEvent");
                    }
                    else if (recv_by_socket && handle == sockov.hEvent) {
                        handle_string.emplace_back("sockov.hEvent");
                    }
                    else {
                        handle_string.emplace_back("UnknownHandle");
                    }
                }
                Log.warn("Wait handles:", handle_string, "timeout.");
                if (process_running) {
                    TerminateProcess(process_info.hProcess, 0);
                }
                break;
            }
            continue;
        }
        else {
            // something bad happened
            err = GetLastError();
            // throw std::system_error(std::error_code(err, std::system_category()));
            Log.error(__FUNCTION__, "A fatal error occurred", err);
            break;
        }

        if (signaled_object == process_info.hProcess) {
            process_running = false;
        }
        else if (signaled_object == pipeov.hEvent) {
            // pipe read
            DWORD len = 0;
            if (GetOverlappedResult(pipe_parent_read, &pipeov, &len, FALSE)) {
                pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + len);
                (void)ReadFile(pipe_parent_read, pipe_buffer.get(), (DWORD)pipe_buffer.size(), nullptr, &pipeov);
            }
            else {
                err = GetLastError();
                if (err == ERROR_HANDLE_EOF || err == ERROR_BROKEN_PIPE) {
                    pipe_eof = true;
                }
            }
        }
        else if (signaled_object == sockov.hEvent) {
            if (accept_pending) {
                // AcceptEx, client_socker is connected and first chunk of data is received
                DWORD len = 0;
                if (GetOverlappedResult(reinterpret_cast<HANDLE>(m_server_sock), &sockov, &len, FALSE)) {
                    accept_pending = false;
                    if (recv_by_socket)
                        sock_data.insert(sock_data.end(), sock_buffer.value().get(), sock_buffer.value().get() + len);

                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        // reset the overlapped since we reuse it for different handle
                        auto event = sockov.hEvent;
                        sockov = {};
                        sockov.hEvent = event;

                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.value().get(),
                                       (DWORD)sock_buffer.value().size(), nullptr, &sockov);
                    }
                }
            }
            else {
                // ReadFile
                DWORD len = 0;
                if (GetOverlappedResult(reinterpret_cast<HANDLE>(client_socket), &sockov, &len, FALSE)) {
                    if (recv_by_socket)
                        sock_data.insert(sock_data.end(), sock_buffer.value().get(), sock_buffer.value().get() + len);
                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.value().get(),
                                       (DWORD)sock_buffer.value().size(), nullptr, &sockov);
                    }
                }
                else {
                    // err = GetLastError();
                    socket_eof = true;
                    ::closesocket(client_socket);
                }
            }
        }
    }

    DWORD exit_ret = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_ret);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    CloseHandle(pipe_parent_read);
    CloseHandle(pipeov.hEvent);
    if (recv_by_socket) {
        if (!socket_eof) closesocket(client_socket);
        CloseHandle(sockov.hEvent);
    }

#else
    auto check_timeout = [&]() -> bool {
        return timeout && timeout < duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    };

    int exit_ret = 0;
    m_child = ::fork();
    if (m_child == 0) {
        // child process

        ::dup2(m_pipe_in[PIPE_READ], STDIN_FILENO);
        ::dup2(m_pipe_out[PIPE_WRITE], STDOUT_FILENO);
        ::dup2(m_pipe_out[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        // close(m_pipe_in[PIPE_READ]);
        // close(m_pipe_in[PIPE_WRITE]);
        // close(m_pipe_out[PIPE_READ]);
        // close(m_pipe_out[PIPE_WRITE]);

        exit_ret = execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        ::exit(exit_ret);
    }
    else if (m_child > 0) {
        // parent process
        do {
            if (recv_by_socket) {
                sockaddr addr {};
                socklen_t len = sizeof(addr);
                sock_buffer = asst::platform::single_page_buffer<char>();

                int client_socket = ::accept(m_server_sock, &addr, &len);
                if (client_socket < 0) {
                    Log.error("accept failed:", strerror(errno));
                    return std::nullopt;
                }

                ssize_t read_num = ::read(client_socket, sock_buffer.value().get(), sock_buffer.value().size());

                while (read_num > 0) {
                    sock_data.insert(sock_data.end(), sock_buffer.value().get(), sock_buffer.value().get() + read_num);
                    read_num = ::read(client_socket, sock_buffer.value().get(), sock_buffer.value().size());
                }

                ::close(client_socket);
                break;
            }

            ssize_t read_num = ::read(m_pipe_out[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());

            while (read_num > 0) {
                pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                read_num = ::read(m_pipe_out[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());
            }
        } while (::waitpid(m_child, &exit_ret, WNOHANG) == 0 && !check_timeout());
    }
    else {
        // failed to create child process
        Log.error("Call `", cmd, "` create process failed, child:", m_child);
        return std::nullopt;
    }
#endif

    callcmd_lock.unlock();

    auto duration = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    Log.info("Call `", cmd, "` ret", exit_ret, ", cost", duration, "ms , stdout size:", pipe_data.size(),
             ", socket size:", sock_data.size());
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        Log.trace("stdout output:", Logger::separator::newline, pipe_data);
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        Log.trace("socket output:", Logger::separator::newline, sock_data);
    }
    // 直接 return，避免走到下面的 else if 里的 make_instance_inited(false) 关闭 adb 连接，
    // 导致停止后再开始任务还需要重连一次
    if (need_exit()) {
        return std::nullopt;
    }

    if (!exit_ret) {
        return recv_by_socket ? sock_data : pipe_data;
    }
    else if (inited() && allow_reconnect) {
        // 之前可以运行，突然运行不了了，这种情况多半是 adb 炸了。所以重新连接一下
        json::value reconnect_info = json::object {
            { "uuid", m_uuid },
            { "what", "Reconnecting" },
            { "why", "" },
            { "details",
              json::object {
                  { "reconnect", m_adb.connect },
                  { "cmd", cmd },
              } },
        };
        static constexpr int ReconnectTimes = 20;
        for (int i = 0; i < ReconnectTimes; ++i) {
            if (need_exit()) {
                break;
            }
            reconnect_info["details"]["times"] = i;
            callback(AsstMsg::ConnectionInfo, reconnect_info);

            // TODO: 也许 WIN32 可以用 WaitForSingleObjectEx 做一个允许外部打断的 sleep
            std::this_thread::sleep_for(10s);
            if (need_exit()) {
                break;
            }
            auto reconnect_ret = call_command(m_adb.connect, 60LL * 1000, false /* 禁止重连避免无限递归 */);
            if (need_exit()) {
                break;
            }
            bool is_reconnect_success = false;
            if (reconnect_ret) {
                auto& reconnect_str = reconnect_ret.value();
                is_reconnect_success = reconnect_str.find("error") == std::string::npos;
            }
            if (is_reconnect_success) {
                auto recall_ret = call_command(cmd, timeout, false /* 禁止重连避免无限递归 */, recv_by_socket);
                if (recall_ret) {
                    // 重连并成功执行了
                    reconnect_info["what"] = "Reconnected";
                    callback(AsstMsg::ConnectionInfo, reconnect_info);
                    return recall_ret;
                }
            }
        }
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "Disconnect" },
            { "why", "Reconnect failed" },
            { "details",
              json::object {
                  { "cmd", m_adb.connect },
              } },
        };
        make_instance_inited(false); // 重连失败，释放
        callback(AsstMsg::ConnectionInfo, info);
    }

    return std::nullopt;
}

void asst::Controller::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_callback_arg);
    }
}

bool asst::Controller::call_and_hup_minitouch(const std::string& cmd)
{
    LogTraceFunction;
    Log.info(cmd);
    m_minitouch_avaiable = false;

    constexpr int PipeReadBuffSize = 4096ULL;
    constexpr int PipeWriteBuffSize = 64 * 1024ULL;
    std::string pipe_str;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa_attr_inherit {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };
    HANDLE pipe_child_write = INVALID_HANDLE_VALUE, pipe_parent_read = INVALID_HANDLE_VALUE;
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_attr_inherit,
                                             PipeReadBuffSize, true, false) ||
        !asst::win32::CreateOverlappablePipe(&m_minitouch_child_read, &m_minitouch_parent_write, &sa_attr_inherit,
                                             nullptr, PipeWriteBuffSize, false, false)) {
        DWORD err = GetLastError();
        Log.error("Failed to create pipe for minitouch, err", err);
        return false;
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = m_minitouch_child_read;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;

    auto cmd_osstr = utils::to_osstring(cmd);
    if (!CreateProcessW(NULL, cmd_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si,
                        &m_minitouch_process_info)) {
        DWORD err = GetLastError();
        Log.error("Failed to create process for minitouch, err", err);
        release_minitouch(true);
        return false;
    }

    CloseHandle(pipe_child_write);
    pipe_child_write = INVALID_HANDLE_VALUE;

    auto pipe_buffer = std::make_unique<char[]>(PipeReadBuffSize);

    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&]() -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < 3s;
    };
    std::vector<HANDLE> wait_handles;

    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    std::ignore = ReadFile(pipe_parent_read, pipe_buffer.get(), PipeReadBuffSize, nullptr, &pipeov);

    while (!need_exit() && check_timeout()) {
        if (pipe_str.find('$') != std::string::npos) {
            break;
        }
        DWORD len = 0;
        if (!GetOverlappedResult(pipe_parent_read, &pipeov, &len, FALSE)) {
            continue;
        }
        pipe_str.insert(pipe_str.end(), pipe_buffer.get(), pipe_buffer.get() + len);
        std::ignore = ReadFile(pipe_parent_read, pipe_buffer.get(), PipeReadBuffSize, nullptr, &pipeov);
    }

    CloseHandle(pipe_parent_read);
    pipe_parent_read = INVALID_HANDLE_VALUE;

#else  // !_WIN32
    // TODO
    std::ignore = pipe_str;
    std::ignore = PipeBuffSize;
    return false;
#endif // _WIN32

    Log.info("pipe str", Logger::separator::newline, pipe_str);

    convert_lf(pipe_str);
    size_t s_pos = pipe_str.find('^');
    size_t e_pos = pipe_str.find('\n', s_pos);
    if (s_pos == std::string::npos || e_pos == std::string::npos) {
        Log.error("Failed to find ^ in minitouch pipe");
        release_minitouch(true);
        return false;
    }
    std::string key_info = pipe_str.substr(s_pos + 1, e_pos - s_pos - 1);
    Log.info("minitouch key props", key_info);
    std::stringstream ss;
    ss << key_info;
    ss >> m_minitouch_props.max_contacts;
    ss >> m_minitouch_props.max_x;
    ss >> m_minitouch_props.max_y;
    ss >> m_minitouch_props.max_pressure;

    m_minitouch_props.x_scaling = static_cast<double>(m_minitouch_props.max_x) / m_width;
    m_minitouch_props.y_scaling = static_cast<double>(m_minitouch_props.max_y) / m_height;

    m_minitouch_avaiable = true;
    return true;
}

bool asst::Controller::input_to_minitouch(const std::string& cmd, int delay_ms)
{
    LogTraceFunction;
    Log.info("Input to minitouch", Logger::separator::newline, cmd);

#ifdef _WIN32
    DWORD written = 0;
    if (!WriteFile(m_minitouch_parent_write, cmd.c_str(),
                   static_cast<DWORD>(cmd.size() * sizeof(std::string::value_type)), &written, NULL)) {
        auto err = GetLastError();
        Log.error("Failed to write to minitouch, err", err);
        return false;
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(delay_ms * 1ms);

    return cmd.size() == written;
#else
    return false;
#endif
}

void asst::Controller::release_minitouch(bool force)
{
    LogTraceFunction;

    if (!m_minitouch_avaiable && !force) {
        return;
    }
    m_minitouch_avaiable = false;

#ifdef _WIN32

    if (m_minitouch_process_info.hProcess) {
        CloseHandle(m_minitouch_process_info.hProcess);
        m_minitouch_process_info.hProcess = nullptr;
    }
    if (m_minitouch_process_info.hThread) {
        CloseHandle(m_minitouch_process_info.hThread);
        m_minitouch_process_info.hThread = nullptr;
    }
    if (m_minitouch_child_read) {
        CloseHandle(m_minitouch_child_read);
        m_minitouch_child_read = nullptr;
    }
    if (m_minitouch_parent_write) {
        CloseHandle(m_minitouch_parent_write);
        m_minitouch_parent_write = nullptr;
    }

#endif //  _WIN32
}

// 返回值代表是否找到 "\r\n"，函数本身会将所有 "\r\n" 替换为 "\n"
bool asst::Controller::convert_lf(std::string& data)
{
    LogTraceFunction;

    if (data.empty() || data.size() < 2) {
        return false;
    }
    auto pred = [](const std::string::iterator& cur) -> bool { return *cur == '\r' && *(cur + 1) == '\n'; };
    // find the first of "\r\n"
    auto first_iter = data.end();
    for (auto iter = data.begin(); iter != data.end() - 1; ++iter) {
        if (pred(iter)) {
            first_iter = iter;
            break;
        }
    }
    if (first_iter == data.end()) {
        return false;
    }
    // move forward all non-crlf elements
    auto end_r1_iter = data.end() - 1;
    auto next_iter = first_iter;
    while (++first_iter != end_r1_iter) {
        if (!pred(first_iter)) {
            *next_iter = *first_iter;
            ++next_iter;
        }
    }
    *next_iter = *end_r1_iter;
    ++next_iter;
    data.erase(next_iter, data.end());
    return true;
}

asst::Point asst::Controller::rand_point_in_rect(const Rect& rect)
{
    int x = 0, y = 0;
    if (rect.width == 0) {
        x = rect.x;
    }
    else {
        int x_rand = std::poisson_distribution<int>(rect.width / 2.)(m_rand_engine);

        x = x_rand + rect.x;
    }

    if (rect.height == 0) {
        y = rect.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(rect.height / 2.)(m_rand_engine);
        y = y_rand + rect.y;
    }

    return { x, y };
}

void asst::Controller::random_delay() const
{
    auto& opt = Configer.get_options();
    if (opt.control_delay_upper != 0) {
        LogTraceFunction;
        static std::default_random_engine rand_engine(std::random_device {}());
        static std::uniform_int_distribution<unsigned> rand_uni(opt.control_delay_lower, opt.control_delay_upper);

        unsigned rand_delay = rand_uni(rand_engine);

        Log.trace("random_delay |", rand_delay, "ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(rand_delay));
    }
}

void asst::Controller::clear_info() noexcept
{
    make_instance_inited(false);
    m_adb = decltype(m_adb)();
    m_uuid.clear();
    m_width = 0;
    m_height = 0;
    m_control_scale = 1.0;
    m_minitouch_avaiable = false;
    m_scale_size = { WindowWidthDefault, WindowHeightDefault };
}

void asst::Controller::close_socket() noexcept
{
#ifdef _WIN32
    if (m_server_sock != INVALID_SOCKET) {
        ::closesocket(m_server_sock);
        m_server_sock = INVALID_SOCKET;
    }
#else
    if (m_server_sock >= 0) {
        ::close(m_server_sock);
        m_server_sock = -1;
    }
#endif
    m_server_started = false;
}

std::optional<unsigned short> asst::Controller::init_socket(const std::string& local_address)
{
    LogTraceFunction;

#ifdef _WIN32
    if (m_server_sock == INVALID_SOCKET) {
        m_server_sock = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_server_sock == INVALID_SOCKET) {
            return std::nullopt;
        }
    }

    DWORD dummy = 0;
    GUID guid_accept_ex = WSAID_ACCEPTEX;
    int err = WSAIoctl(m_server_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_accept_ex, sizeof(guid_accept_ex),
                       &m_server_accept_ex, sizeof(m_server_accept_ex), &dummy, NULL, NULL);
    if (err == SOCKET_ERROR) {
        err = WSAGetLastError();
        Log.error("failed to resolve AcceptEx, err:", err);
        ::closesocket(m_server_sock);
        return std::nullopt;
    }
    m_server_sock_addr.sin_family = PF_INET;
    ::inet_pton(AF_INET, local_address.c_str(), &m_server_sock_addr.sin_addr);
#else
    m_server_sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_sock < 0) {
        return std::nullopt;
    }

    m_server_sock_addr.sin_family = AF_INET;
    m_server_sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif

    bool server_start = false;
    uint16_t port_result = 0;

#ifdef _WIN32
    m_server_sock_addr.sin_port = ::htons(0);
    int bind_ret = ::bind(m_server_sock, reinterpret_cast<SOCKADDR*>(&m_server_sock_addr), sizeof(SOCKADDR));
    int addrlen = sizeof(m_server_sock_addr);
    int getname_ret = ::getsockname(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), &addrlen);
    int listen_ret = ::listen(m_server_sock, 3);
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0;
#else
    m_server_sock_addr.sin_port = htons(0);
    int bind_ret = ::bind(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), sizeof(::sockaddr_in));
    socklen_t addrlen = sizeof(m_server_sock_addr);
    int getname_ret = ::getsockname(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), &addrlen);
    int listen_ret = ::listen(m_server_sock, 3);
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0;
#endif

    if (!server_start) {
        Log.info("not supports socket");
        return std::nullopt;
    }

#ifdef _WIN32
    port_result = ::ntohs(m_server_sock_addr.sin_port);
#else
    port_result = ntohs(m_server_sock_addr.sin_port);
#endif

    Log.info("command server start", local_address, port_result);
    return port_result;
}

bool asst::Controller::screencap(bool allow_reconnect)
{
    DecodeFunc decode_raw = [&](const std::string& data) -> bool {
        if (data.empty()) {
            return false;
        }
        size_t std_size = 4ULL * m_width * m_height;
        if (data.size() < std_size) {
            return false;
        }
        size_t header_size = data.size() - std_size;
        auto img_data_beg = data.cbegin() + header_size;
        if (std::all_of(data.cbegin(), img_data_beg, std::logical_not<bool> {})) {
            return false;
        }
        cv::Mat temp(m_height, m_width, CV_8UC4, const_cast<char*>(&*img_data_beg));
        if (temp.empty()) {
            return false;
        }
        cv::cvtColor(temp, temp, cv::COLOR_RGB2BGR);
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    DecodeFunc decode_raw_with_gzip = [&](const std::string& data) -> bool {
        const std::string raw_data = gzip::decompress(data.data(), data.size());
        return decode_raw(raw_data);
    };

    DecodeFunc decode_encode = [&](const std::string& data) -> bool {
        cv::Mat temp = cv::imdecode({ data.data(), int(data.size()) }, cv::IMREAD_COLOR);
        if (temp.empty()) {
            return false;
        }
        std::unique_lock<std::shared_mutex> image_lock(m_image_mutex);
        m_cache_image = temp;
        return true;
    };

    switch (m_adb.screencap_method) {
    case AdbProperty::ScreencapMethod::UnknownYet: {
        using namespace std::chrono;
        Log.info("Try to find the fastest way to screencap");
        auto min_cost = milliseconds(LLONG_MAX);
        clear_lf_info();

        auto start_time = high_resolution_clock::now();
        if (m_support_socket && m_server_started &&
            screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true)) {
            // sock 第一次截图比较长（不知道是不是初始化了什么东西耽误时间，减个额外的的时间）
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time) + 1000ms;
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawByNc;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("RawByNc cost", duration.count(), "ms");
        }
        else {
            Log.info("RawByNc is not supported");
        }
        clear_lf_info();

        start_time = high_resolution_clock::now();
        if (screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect)) {
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::RawWithGzip;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("RawWithGzip cost", duration.count(), "ms");
        }
        else {
            Log.info("RawWithGzip is not supported");
        }
        clear_lf_info();

        start_time = high_resolution_clock::now();
        if (screencap(m_adb.screencap_encode, decode_encode, allow_reconnect)) {
            auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start_time);
            if (duration < min_cost) {
                m_adb.screencap_method = AdbProperty::ScreencapMethod::Encode;
                make_instance_inited(true);
                min_cost = duration;
            }
            Log.info("Encode cost", duration.count(), "ms");
        }
        else {
            Log.info("Encode is not supported");
        }
        static const std::unordered_map<AdbProperty::ScreencapMethod, std::string> MethodName = {
            { AdbProperty::ScreencapMethod::UnknownYet, "UnknownYet" },
            { AdbProperty::ScreencapMethod::RawByNc, "RawByNc" },
            { AdbProperty::ScreencapMethod::RawWithGzip, "RawWithGzip" },
            { AdbProperty::ScreencapMethod::Encode, "Encode" },
        };
        Log.info("The fastest way is", MethodName.at(m_adb.screencap_method), ", cost:", min_cost.count(), "ms");
        clear_lf_info();
        return m_adb.screencap_method != AdbProperty::ScreencapMethod::UnknownYet;
    } break;
    case AdbProperty::ScreencapMethod::RawByNc: {
        return screencap(m_adb.screencap_raw_by_nc, decode_raw, allow_reconnect, true);
    } break;
    case AdbProperty::ScreencapMethod::RawWithGzip: {
        return screencap(m_adb.screencap_raw_with_gzip, decode_raw_with_gzip, allow_reconnect);
    } break;
    case AdbProperty::ScreencapMethod::Encode: {
        return screencap(m_adb.screencap_encode, decode_encode, allow_reconnect);
    } break;
    }

    return false;
}

bool asst::Controller::screencap(const std::string& cmd, const DecodeFunc& decode_func, bool allow_reconnect,
                                 bool by_socket)
{
    if ((!m_support_socket || !m_server_started) && by_socket) [[unlikely]] {
        return false;
    }

    auto ret = call_command(cmd, 20000, allow_reconnect, by_socket);

    if (!ret || ret.value().empty()) [[unlikely]] {
        Log.error("data is empty!");
        return false;
    }
    auto& data = ret.value();

    bool tried_conversion = false;
    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::CRLF) {
        tried_conversion = true;
        if (!convert_lf(data)) [[unlikely]] { // 没找到 "\r\n"
            Log.info("screencap_end_of_line is set to CRLF but no `\\r\\n` found, set it to LF");
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
    }

    if (decode_func(data)) [[likely]] {
        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) [[unlikely]] {
            Log.info("screencap_end_of_line is LF");
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
        return true;
    }
    else {
        Log.info("data is not empty, but image is empty");

        if (tried_conversion) { // 已经转换过行尾，再次转换 data 不会变化，不必重试
            Log.error("skip retry decoding and decode failed!");
            return false;
        }

        Log.info("try to cvt lf");
        if (!convert_lf(data)) { // 没找到 "\r\n"，data 没有变化，不必重试
            Log.error("no `\\r\\n` found, skip retry decode");
            return false;
        }
        if (!decode_func(data)) {
            Log.error("convert lf and retry decode failed!");
            return false;
        }

        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
            Log.info("screencap_end_of_line is CRLF");
        }
        else {
            Log.info("screencap_end_of_line is changed to CRLF");
        }
        m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;
        return true;
    }
}

void asst::Controller::clear_lf_info()
{
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::UnknownYet;
}

cv::Mat asst::Controller::get_resized_image_cache() const
{
    const static cv::Size d_size(m_scale_size.first, m_scale_size.second);

    std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
    if (m_cache_image.empty()) {
        Log.error("image is empty");
        return { d_size, CV_8UC3 };
    }
    cv::Mat resized_mat;
    cv::resize(m_cache_image, resized_mat, d_size, 0.0, 0.0, cv::INTER_AREA);
    return resized_mat;
}

bool asst::Controller::start_game(const std::string& client_type)
{
    if (client_type.empty()) {
        return false;
    }
    auto intent_name = Configer.get_intent_name(client_type);
    if (!intent_name) {
        return false;
    }
    std::string cur_cmd = utils::string_replace_all(m_adb.start, "[Intent]", intent_name.value());
    return call_command(cur_cmd).has_value();
}

bool asst::Controller::stop_game()
{
    return call_command(m_adb.stop).has_value();
}

bool asst::Controller::click(const Point& p)
{
    int x = static_cast<int>(p.x * m_control_scale);
    int y = static_cast<int>(p.y * m_control_scale);
    // log.trace("Click, raw:", p.x, p.y, "corr:", x, y);

    return click_without_scale(Point(x, y));
}

bool asst::Controller::click(const Rect& rect)
{
    return click(rand_point_in_rect(rect));
}

bool asst::Controller::click_without_scale(const Point& p)
{
    if (p.x < 0 || p.x >= m_width || p.y < 0 || p.y >= m_height) {
        Log.error("click point out of range");
    }

    if (m_minitouch_enabled && m_minitouch_avaiable) {
        int x = static_cast<int>(p.x * m_minitouch_props.x_scaling);
        int y = static_cast<int>(p.y * m_minitouch_props.y_scaling);
        constexpr int WaitMs = 50;
        std::string minitouch_cmd = MinitouchCmd::down(x, y, true, WaitMs) + MinitouchCmd::up();
        return input_to_minitouch(minitouch_cmd, WaitMs);
    }
    else {
        std::string cur_cmd =
            utils::string_replace_all(m_adb.click, { { "[x]", std::to_string(p.x) }, { "[y]", std::to_string(p.y) } });
        return call_command(cur_cmd).has_value();
    }
}

bool asst::Controller::click_without_scale(const Rect& rect)
{
    return click_without_scale(rand_point_in_rect(rect));
}

bool asst::Controller::swipe(const Point& p1, const Point& p2, int duration, bool extra_swipe)
{
    int x1 = static_cast<int>(p1.x * m_control_scale);
    int y1 = static_cast<int>(p1.y * m_control_scale);
    int x2 = static_cast<int>(p2.x * m_control_scale);
    int y2 = static_cast<int>(p2.y * m_control_scale);
    // log.trace("Swipe, raw:", p1.x, p1.y, p2.x, p2.y, "corr:", x1, y1, x2, y2);

    return swipe_without_scale(Point(x1, y1), Point(x2, y2), duration, extra_swipe);
}

bool asst::Controller::swipe(const Rect& r1, const Rect& r2, int duration, bool extra_swipe)
{
    return swipe(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, extra_swipe);
}

bool asst::Controller::swipe_without_scale(const Point& p1, const Point& p2, int duration, bool extra_swipe)
{
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    // 起点不能在屏幕外，但是终点可以
    if (x1 < 0 || x1 >= m_width || y1 < 0 || y1 >= m_height) {
        Log.warn("swipe point1 is out of range", x1, y1);
        x1 = std::clamp(x1, 0, m_width - 1);
        y1 = std::clamp(y1, 0, m_height - 1);
    }

    const auto& opt = Configer.get_options();
    if (m_minitouch_enabled && m_minitouch_avaiable) {
        constexpr int MoveInterval = 1;
        std::string minitouch_cmd =
            MinitouchCmd::down(static_cast<int>(x1 * m_minitouch_props.x_scaling),
                               static_cast<int>(m_minitouch_props.y_scaling * y1), true, MoveInterval);
        if (duration == 0) {
            duration = 100;
        }
        auto move_cmd = [&](int _x1, int _y1, int _x2, int _y2, int _duration) -> std::string {
            _x1 = static_cast<int>(_x1 * m_minitouch_props.x_scaling);
            _y1 = static_cast<int>(_y1 * m_minitouch_props.y_scaling);
            _x2 = static_cast<int>(_x2 * m_minitouch_props.x_scaling);
            _y2 = static_cast<int>(_y2 * m_minitouch_props.y_scaling);

            int move_times = _duration / MoveInterval;
            int x_step = (_x2 - _x1) / move_times;
            int y_step = (_y2 - _y1) / move_times;
            // TODO: 加点随机因子，或者改成中间快两头慢
            std::string minitouch_cmd;
            for (int times = 1; times < move_times; ++times) {
                int cur_x = _x1 + x_step * times;
                int cur_y = _y1 + y_step * times;
                if (cur_x < 0 || cur_x > m_minitouch_props.max_x || cur_y < 0 || cur_y > m_minitouch_props.max_y) {
                    continue;
                }
                minitouch_cmd += MinitouchCmd::move(cur_x, cur_y, true, MoveInterval);
            }
            if (_x2 >= 0 && _x2 <= m_minitouch_props.max_x && _y2 >= 0 && _y2 <= m_minitouch_props.max_y) {
                minitouch_cmd += MinitouchCmd::move(_x2, _y2);
            }
            return minitouch_cmd;
        };
        minitouch_cmd += move_cmd(x1, y1, x2, y2, duration);

        if (extra_swipe && opt.minitouch_extra_swipe_duration > 0) {
            minitouch_cmd +=
                move_cmd(x2, y2, x2, y2 - opt.minitouch_extra_swipe_dist, opt.minitouch_extra_swipe_duration);
            duration += opt.minitouch_extra_swipe_duration;
        }
        minitouch_cmd += MinitouchCmd::up();

        return input_to_minitouch(minitouch_cmd, duration);
    }
    else {
        std::string cur_cmd =
            utils::string_replace_all(m_adb.swipe, {
                                                       { "[x1]", std::to_string(x1) },
                                                       { "[y1]", std::to_string(y1) },
                                                       { "[x2]", std::to_string(x2) },
                                                       { "[y2]", std::to_string(y2) },
                                                       { "[duration]", duration <= 0 ? "" : std::to_string(duration) },
                                                   });
        bool ret = call_command(cur_cmd).has_value();

        // 额外的滑动：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
        if (!m_minitouch_avaiable && extra_swipe && opt.adb_extra_swipe_duration > 0) {
            std::string extra_cmd = utils::string_replace_all(
                m_adb.swipe, {
                                 { "[x1]", std::to_string(x2) },
                                 { "[y1]", std::to_string(y2) },
                                 { "[x2]", std::to_string(x2) },
                                 { "[y2]", std::to_string(y2 - opt.adb_extra_swipe_dist /* * m_control_scale*/) },
                                 { "[duration]", std::to_string(opt.adb_extra_swipe_duration) },
                             });
            ret &= call_command(extra_cmd).has_value();
        }
        return ret;
    }
}

bool asst::Controller::swipe_without_scale(const Rect& r1, const Rect& r2, int duration, bool extra_swipe)
{
    return swipe_without_scale(rand_point_in_rect(r1), rand_point_in_rect(r2), duration, extra_swipe);
}

bool asst::Controller::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    release_minitouch();
    clear_info();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        make_instance_inited(true);
        return true;
    }
#endif

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    auto adb_ret = Configer.get_adb_cfg(config);
    if (!adb_ret) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigNotFound" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    const auto& adb_cfg = adb_ret.value();
    std::string display_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    // 里面的值每次执行命令后可能更新，所以要用 lambda 拿最新的
    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(cfg_cmd, {
                                                      { "[Adb]", adb_path },
                                                      { "[AdbSerial]", address },
                                                      { "[DisplayId]", display_id },
                                                      { "[NcPort]", std::to_string(nc_port) },
                                                      { "[NcAddress]", nc_address },
                                                  });
    };

    if (need_exit()) {
        return false;
    }

    /* connect */
    {
        m_adb.connect = cmd_replace(adb_cfg.connect);
        auto connect_ret = call_command(m_adb.connect, 60LL * 1000, false /* adb 连接时不允许重试 */);
        // 端口即使错误，命令仍然会返回0，TODO 对connect_result进行判断
        bool is_connect_success = false;
        if (connect_ret) {
            auto& connect_str = connect_ret.value();
            is_connect_success = connect_str.find("error") == std::string::npos;
            if (connect_str.find("daemon started successfully") != std::string::npos &&
                connect_str.find("daemon still not running") == std::string::npos) {
                m_adb_release = cmd_replace(adb_cfg.release);
            }
        }
        if (!is_connect_success) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Connection command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }

    /* get uuid (imei) */
    {
        auto uuid_ret = call_command(cmd_replace(adb_cfg.uuid), 20000, false /* adb 连接时不允许重试 */);
        if (!uuid_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Uuid command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        auto& uuid_str = uuid_ret.value();
        std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });
        m_uuid = std::move(uuid_str);

        json::value info = get_info_json() | json::object {
            { "what", "UuidGot" },
            { "why", "" },
        };
        info["details"]["uuid"] = m_uuid;
        callback(AsstMsg::ConnectionInfo, info);
    }

    if (need_exit()) {
        return false;
    }

    // 按需获取display ID 信息
    if (!adb_cfg.display_id.empty()) {
        auto display_id_ret = call_command(cmd_replace(adb_cfg.display_id));
        if (!display_id_ret) {
            return false;
        }

        auto& display_id_pipe_str = display_id_ret.value();
        convert_lf(display_id_pipe_str);
        auto last = display_id_pipe_str.rfind(':');
        if (last == std::string::npos) {
            return false;
        }

        display_id = display_id_pipe_str.substr(last + 1);
        // 去掉换行
        display_id.pop_back();
    }

    if (need_exit()) {
        return false;
    }

    /* display */
    {
        auto display_ret = call_command(cmd_replace(adb_cfg.display));
        if (!display_ret) {
            json::value info = get_info_json() | json::object {
                { "what", "ConnectFailed" },
                { "why", "Display command failed to exec" },
            };
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }

        auto& display_pipe_str = display_ret.value();
        int size_value1 = 0;
        int size_value2 = 0;
#ifdef _MSC_VER
        sscanf_s(display_pipe_str.c_str(), adb_cfg.display_format.c_str(), &size_value1, &size_value2);
#else
        sscanf(display_pipe_str.c_str(), adb_cfg.display_format.c_str(), &size_value1, &size_value2);
#endif
        // 为了防止抓取句柄的时候手机是竖屏的（还没进游戏），这里取大的值为宽，小的为高
        // 总不能有人竖屏玩明日方舟吧（？
        m_width = (std::max)(size_value1, size_value2);
        m_height = (std::min)(size_value1, size_value2);

        json::value info = get_info_json() | json::object {
            { "what", "ResolutionGot" },
            { "why", "" },
        };

        info["details"] |= json::object {
            { "width", m_width },
            { "height", m_height },
        };

        callback(AsstMsg::ConnectionInfo, info);

        if (m_width == 0 || m_height == 0) {
            info["what"] = "ResolutionError";
            info["why"] = "Get resolution failed";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        else if (m_width < WindowWidthDefault || m_height < WindowHeightDefault) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Low screen resolution";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
        else if (std::fabs(static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault) -
                           static_cast<double>(m_width) / static_cast<double>(m_height)) > 1e-7) {
            info["what"] = "UnsupportedResolution";
            info["why"] = "Not 16:9";
            callback(AsstMsg::ConnectionInfo, info);
            return false;
        }
    }

    if (need_exit()) {
        return false;
    }

    /* calc ratio */
    {
        constexpr double DefaultRatio =
            static_cast<double>(WindowWidthDefault) / static_cast<double>(WindowHeightDefault);
        double cur_ratio = static_cast<double>(m_width) / static_cast<double>(m_height);

        if (cur_ratio >= DefaultRatio // 说明是宽屏或默认16:9，按照高度计算缩放
            || std::fabs(cur_ratio - DefaultRatio) < DoubleDiff) {
            int scale_width = static_cast<int>(cur_ratio * WindowHeightDefault);
            m_scale_size = std::make_pair(scale_width, WindowHeightDefault);
            m_control_scale = static_cast<double>(m_height) / static_cast<double>(WindowHeightDefault);
        }
        else { // 否则可能是偏正方形的屏幕，按宽度计算
            int scale_height = static_cast<int>(WindowWidthDefault / cur_ratio);
            m_scale_size = std::make_pair(WindowWidthDefault, scale_height);
            m_control_scale = static_cast<double>(m_width) / static_cast<double>(WindowWidthDefault);
        }
    }

    {
        json::value info = get_info_json() | json::object {
            { "what", "Connected" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
    }

    m_adb.click = cmd_replace(adb_cfg.click);
    m_adb.swipe = cmd_replace(adb_cfg.swipe);
    m_adb.screencap_raw_with_gzip = cmd_replace(adb_cfg.screencap_raw_with_gzip);
    m_adb.screencap_encode = cmd_replace(adb_cfg.screencap_encode);
    m_adb_release = m_adb.release = cmd_replace(adb_cfg.release);
    m_adb.start = cmd_replace(adb_cfg.start);
    m_adb.stop = cmd_replace(adb_cfg.stop);

    if (m_support_socket && !m_server_started) {
        std::string bind_address;
        if (size_t pos = address.rfind(':'); pos != std::string::npos) {
            bind_address = address.substr(0, pos);
        }
        else {
            bind_address = "127.0.0.1";
        }

        // reference from
        // https://github.com/ArknightsAutoHelper/ArknightsAutoHelper/blob/master/automator/connector/ADBConnector.py#L436
        auto nc_address_ret = call_command(cmd_replace(adb_cfg.nc_address));
        if (nc_address_ret && !m_server_started) {
            auto& nc_result_str = nc_address_ret.value();
            if (auto pos = nc_result_str.find(' '); pos != std::string::npos) {
                nc_address = nc_result_str.substr(0, pos);
            }
        }

        auto socket_opt = init_socket(bind_address);
        if (socket_opt) {
            nc_port = socket_opt.value();
            m_adb.screencap_raw_by_nc = cmd_replace(adb_cfg.screencap_raw_by_nc);
            m_server_started = true;
        }
        else {
            m_server_started = false;
        }
    }

    if (need_exit()) {
        return false;
    }

    auto minitouch_cmd_rep = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(
            cmd_replace(cfg_cmd),
            {
                // TODO: 用 adb shell getprop ro.product.cpu.abilist 来判断该用哪个文件夹里的
                { "[minitouchLocalPath]", (m_resource_path / "minitouch" / "armeabi-v7a" / "minitouch").string() },
                { "[minitouchWorkingFile]", m_uuid },
            });
    };

    call_command(minitouch_cmd_rep(adb_cfg.push_minitouch));
    call_command(minitouch_cmd_rep(adb_cfg.chmod_minitouch));

    call_and_hup_minitouch(minitouch_cmd_rep(adb_cfg.call_minitouch));

    // try to find the fastest way
    if (!screencap()) {
        Log.error("Cannot find a proper way to screencap!");
        return false;
    }

    return true;
}

bool asst::Controller::make_instance_inited(bool inited)
{
    Log.trace(__FUNCTION__, "|", inited, ", pre m_inited =", m_inited, ", pre m_instance_count =", m_instance_count);

    if (inited == m_inited) {
        return true;
    }
    m_inited = inited;

    if (inited) {
        ++m_instance_count;
    }
    else {
        // 所有实例全部释放，执行最终的 release 函数
        if (!--m_instance_count) {
            return release();
        }
    }
    return true;
}

void asst::Controller::kill_adb_daemon()
{
    if (m_instance_count) return;
#ifndef _WIN32
    if (m_child)
#endif
    {
        if (!m_adb_release.empty()) {
            call_command(m_adb_release, 20000, false);
            m_adb_release.clear();
        }
    }
}

bool asst::Controller::release()
{
    close_socket();

#ifndef _WIN32
    if (m_child)
#endif
    {
        if (m_adb.release.empty()) {
            return true;
        }
        else {
            m_adb_release.clear();
            return call_command(m_adb.release, 20000, false).has_value();
        }
    }
}

bool asst::Controller::inited() const noexcept
{
    return m_inited;
}

void asst::Controller::set_exit_flag(bool* flag)
{
    m_exit_flag = flag;
}

const std::string& asst::Controller::get_uuid() const
{
    return m_uuid;
}

cv::Mat asst::Controller::get_image(bool raw)
{
    if (m_scale_size.first == 0 || m_scale_size.second == 0) {
        Log.error("Unknown image size");
        return {};
    }

    // 有些模拟器adb偶尔会莫名其妙截图失败，多试几次
    static constexpr int MaxTryCount = 20;
    bool success = false;
    for (int i = 0; i < MaxTryCount && inited(); ++i) {
        if (need_exit()) {
            break;
        }
        if (screencap()) {
            success = true;
            break;
        }
    }
    while (!success && !need_exit()) {
        if (screencap(true)) {
            break;
        }
        Log.error(__FUNCTION__, "screencap failed!");
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapFailed" },
            { "why", "ScreencapFailed" },
            { "details", json::object {} },
        };
        callback(AsstMsg::ConnectionInfo, info);

        const static cv::Size d_size(m_scale_size.first, m_scale_size.second);
        m_cache_image = cv::Mat(d_size, CV_8UC3);

        break;
    }

    if (raw) {
        std::shared_lock<std::shared_mutex> image_lock(m_image_mutex);
        cv::Mat copy = m_cache_image.clone();
        return copy;
    }

    return get_resized_image_cache();
}

std::vector<uchar> asst::Controller::get_encoded_image_cache() const
{
    cv::Mat img = get_resized_image_cache();
    std::vector<uchar> buf;
    cv::imencode(".png", img, buf);

    return buf;
}
