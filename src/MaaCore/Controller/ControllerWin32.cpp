#ifdef _WIN32
#include "ControllerWin32.h"

#include <ws2tcpip.h>

#include "Utils/Logger.hpp"

asst::ControllerWin32::ControllerWin32(Assistant* inst) : InstHelper(inst)
{
    LogTraceFunction;

    m_support_socket = WsaHelper::get_instance()();
}

asst::ControllerWin32::~ControllerWin32()
{
    release_minitouch();
    close_socket();
}

std::optional<int> asst::ControllerWin32::call_command(const std::string& cmd, const bool recv_by_socket,
                                                       std::string& pipe_data, std::string& sock_data,
                                                       const int64_t timeout,
                                                       const std::chrono::steady_clock::time_point start_time)
{
    using namespace std::chrono;

    asst::platform::single_page_buffer<char> pipe_buffer;
    asst::platform::single_page_buffer<char> sock_buffer;

    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa_inherit { .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_inherit,
                                             (DWORD)pipe_buffer.size(), true, false)) {
        DWORD err = GetLastError();
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
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` create process failed, ret", create_ret, "error code:", err);
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
        if (!m_server_accept_ex(m_server_sock, client_socket, sock_buffer.get(),
                                (DWORD)sock_buffer.size() - ((sizeof(sockaddr_in) + 16) * 2), sizeof(sockaddr_in) + 16,
                                sizeof(sockaddr_in) + 16, &dummy, &sockov)) {
            DWORD err = WSAGetLastError();
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
            WaitForMultipleObjectsEx((DWORD)wait_handles.size(), wait_handles.data(), FALSE, (DWORD)wait_time, TRUE);
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
            DWORD err = GetLastError();
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
                DWORD err = GetLastError();
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
                    if (recv_by_socket) sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + len);

                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        // reset the overlapped since we reuse it for different handle
                        auto event = sockov.hEvent;
                        sockov = {};
                        sockov.hEvent = event;

                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.get(),
                                       (DWORD)sock_buffer.size(), nullptr, &sockov);
                    }
                }
            }
            else {
                // ReadFile
                DWORD len = 0;
                if (GetOverlappedResult(reinterpret_cast<HANDLE>(client_socket), &sockov, &len, FALSE)) {
                    if (recv_by_socket) sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + len);
                    if (len == 0) {
                        socket_eof = true;
                        ::closesocket(client_socket);
                    }
                    else {
                        (void)ReadFile(reinterpret_cast<HANDLE>(client_socket), sock_buffer.get(),
                                       (DWORD)sock_buffer.size(), nullptr, &sockov);
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

    return static_cast<int>(exit_ret);
}

std::optional<unsigned short> asst::ControllerWin32::init_socket(const std::string& local_address)
{
    LogTraceFunction;

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

    bool server_start = false;
    uint16_t port_result = 0;

    m_server_sock_addr.sin_port = ::htons(0);
    int bind_ret = ::bind(m_server_sock, reinterpret_cast<SOCKADDR*>(&m_server_sock_addr), sizeof(SOCKADDR));
    int addrlen = sizeof(m_server_sock_addr);
    int getname_ret = ::getsockname(m_server_sock, reinterpret_cast<sockaddr*>(&m_server_sock_addr), &addrlen);
    int listen_ret = ::listen(m_server_sock, 3);
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0;

    if (!server_start) {
        Log.info("not supports socket");
        return std::nullopt;
    }

    port_result = ::ntohs(m_server_sock_addr.sin_port);

    Log.info("command server start", local_address, port_result);
    return port_result;
}

void asst::ControllerWin32::close_socket() noexcept
{
    if (m_server_sock != INVALID_SOCKET) {
        ::closesocket(m_server_sock);
        m_server_sock = INVALID_SOCKET;
    }
}

bool asst::ControllerWin32::call_and_hup_minitouch(const std::string& cmd, const int timeout, std::string& pipe_str)
{
    constexpr int PipeReadBuffSize = 4096ULL;
    constexpr int PipeWriteBuffSize = 64 * 1024ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < std::chrono::seconds(timeout);
    };

    SECURITY_ATTRIBUTES sa_attr_inherit {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };
    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    HANDLE pipe_child_read = INVALID_HANDLE_VALUE, pipe_parent_write = INVALID_HANDLE_VALUE;
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_attr_inherit,
                                             PipeReadBuffSize, true, false) ||
        !asst::win32::CreateOverlappablePipe(&pipe_child_read, &pipe_parent_write, &sa_attr_inherit, nullptr,
                                             PipeWriteBuffSize, false, false)) {
        DWORD err = GetLastError();
        Log.error("Failed to create pipe for minitouch, err", err);
        return false;
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = pipe_child_read;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;

    auto cmd_osstr = utils::to_osstring(cmd);
    BOOL create_ret = CreateProcessW(NULL, cmd_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si,
                                     &m_minitouch_process_info);
    CloseHandle(pipe_child_write);
    CloseHandle(pipe_child_read);
    pipe_child_write = INVALID_HANDLE_VALUE;
    pipe_child_read = INVALID_HANDLE_VALUE;

    if (!create_ret) {
        DWORD err = GetLastError();
        Log.error("Failed to create process for minitouch, err", err);
        release_minitouch(true);
        return false;
    }

    auto start_time = std::chrono::steady_clock::now();

    auto pipe_buffer = std::make_unique<char[]>(PipeReadBuffSize);
    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    std::ignore = ReadFile(pipe_parent_read, pipe_buffer.get(), PipeReadBuffSize, nullptr, &pipeov);

    while (!need_exit() && check_timeout(start_time)) {
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

    m_minitouch_parent_write = pipe_parent_write;
    return true;
}

bool asst::ControllerWin32::input_to_minitouch(const std::string& cmd)
{
    if (m_minitouch_parent_write == INVALID_HANDLE_VALUE) {
        Log.error("minitouch write handle invalid", m_minitouch_parent_write);
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(m_minitouch_parent_write, cmd.c_str(),
                   static_cast<DWORD>(cmd.size() * sizeof(std::string::value_type)), &written, NULL)) {
        auto err = GetLastError();
        Log.error("Failed to write to minitouch, err", err);
        return false;
    }

    return cmd.size() == written;
}

void asst::ControllerWin32::release_minitouch([[maybe_unused]] bool force)
{
    LogTraceFunction;

    if (m_minitouch_process_info.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(m_minitouch_process_info.hProcess);
        m_minitouch_process_info.hProcess = INVALID_HANDLE_VALUE;
    }
    if (m_minitouch_process_info.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(m_minitouch_process_info.hThread);
        m_minitouch_process_info.hThread = INVALID_HANDLE_VALUE;
    }
    if (m_minitouch_parent_write != INVALID_HANDLE_VALUE) {
        CloseHandle(m_minitouch_parent_write);
        m_minitouch_parent_write = INVALID_HANDLE_VALUE;
    }
}

void asst::ControllerWin32::release_adb(const std::string& adb_release, int64_t timeout)
{
    std::string pipe_data;
    std::string sock_data;
    auto start_time = std::chrono::steady_clock::now();

    call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
}
#endif
