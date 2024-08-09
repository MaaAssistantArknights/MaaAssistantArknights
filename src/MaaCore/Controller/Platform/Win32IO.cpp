#ifdef _WIN32
#include "Win32IO.h"

#include <ws2tcpip.h>

#include "Utils/Logger.hpp"

asst::Win32IO::Win32IO(Assistant* inst) : InstHelper(inst)
{
    LogTraceFunction;

    m_support_socket = WsaHelper::get_instance()();
}

asst::Win32IO::~Win32IO()
{
    if (m_server_sock != INVALID_SOCKET) {
        ::closesocket(m_server_sock);
        m_server_sock = INVALID_SOCKET;
    }
}

std::optional<int> asst::Win32IO::call_command(const std::string& cmd, bool recv_by_socket, std::string& pipe_data,
                                               std::string& sock_data, int64_t timeout,
                                               std::chrono::steady_clock::time_point start_time)
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

    STARTUPINFOEXW si {};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.StartupInfo.wShowWindow = SW_HIDE;
    si.StartupInfo.hStdOutput = pipe_child_write;
    si.StartupInfo.hStdError = pipe_child_write;
    ASST_AUTO_DEDUCED_ZERO_INIT_START
    PROCESS_INFORMATION process_info = { nullptr }; // 进程信息结构体
    ASST_AUTO_DEDUCED_ZERO_INIT_END

    std::vector<uint8_t> attrs;
    size_t attrsize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrsize);
    if (attrsize == 0) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` InitializeProcThreadAttributeList failed, ret error code:", err);
        return std::nullopt;
    }
    attrs.resize(attrsize);
    si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs.data());
    auto attr_success = InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrsize);
    if (!attr_success) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` InitializeProcThreadAttributeList failed, ret error code:", err);
        return std::nullopt;
    }
    attr_success = UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &pipe_child_write, sizeof(HANDLE), nullptr, nullptr);
    if (!attr_success) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` UpdateProcThreadAttribute failed, ret error code:", err);
        return std::nullopt;
    }
    auto cmdline_osstr = asst::utils::to_osstring(cmd);
    BOOL create_ret =
        CreateProcessW(nullptr, cmdline_osstr.data(), nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &si.StartupInfo, &process_info);
    DeleteProcThreadAttributeList(si.lpAttributeList);
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
        auto wait_time = (std::min)(timeout - duration_cast<milliseconds>(elapsed).count(), 5LL * 1000);
        if (wait_time < 0 || !process_running) wait_time = 0;
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
                Log.warn("Wait handles:", handle_string, " after", timeout, "ms.");
                if (process_running) {
                    TerminateProcess(process_info.hProcess, 0);
                }
                // 处理超时后返回 std::nullopt
                CloseHandle(pipe_parent_read);
                CloseHandle(pipeov.hEvent);
                CloseHandle(process_info.hProcess);
                CloseHandle(process_info.hThread);
                if (recv_by_socket) {
                    CloseHandle(sockov.hEvent);
                    if (client_socket != INVALID_SOCKET) {
                        closesocket(client_socket);
                    }
                }
                return std::nullopt;
                // break;
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

    if (recv_by_socket) {
        if (accept_pending) {
            Log.warn("cancel AcceptEx");
            CancelIoEx(reinterpret_cast<HANDLE>(m_server_sock), &sockov);
            closesocket(client_socket);
        }
        else if (!socket_eof) {
            Log.warn("cancel ReadFile");
            CancelIoEx(reinterpret_cast<HANDLE>(client_socket), &sockov);
            closesocket(client_socket);
        }
        CloseHandle(sockov.hEvent);
    }

    if (process_running) {
        TerminateProcess(process_info.hProcess, 0);
    }

    DWORD exit_ret = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_ret);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    CloseHandle(pipe_parent_read);
    CloseHandle(pipeov.hEvent);
    return static_cast<int>(exit_ret);
}

std::optional<unsigned short> asst::Win32IO::init_socket(const std::string& local_address)
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

void asst::Win32IO::close_socket() noexcept
{
    if (m_server_sock != INVALID_SOCKET) {
        ::closesocket(m_server_sock);
        m_server_sock = INVALID_SOCKET;
    }
}

std::shared_ptr<asst::IOHandler> asst::Win32IO::interactive_shell(const std::string& cmd)
{
    constexpr int PipeReadBuffSize = 4096ULL;
    constexpr int PipeWriteBuffSize = 64 * 1024ULL;

    SECURITY_ATTRIBUTES sa_attr_inherit {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };
    PROCESS_INFORMATION m_process_info = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    HANDLE pipe_child_read = INVALID_HANDLE_VALUE, pipe_parent_write = INVALID_HANDLE_VALUE;
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_attr_inherit,
                                             PipeReadBuffSize, true, false) ||
        !asst::win32::CreateOverlappablePipe(&pipe_child_read, &pipe_parent_write, &sa_attr_inherit, nullptr,
                                             PipeWriteBuffSize, false, false)) {
        DWORD err = GetLastError();
        Log.error("Failed to create pipe for minitouch, err", err);
        return nullptr;
    }


    HANDLE handles_to_inherit[] = { pipe_child_read, pipe_child_write };
    STARTUPINFOEXW si {};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.StartupInfo.wShowWindow = SW_HIDE;
    si.StartupInfo.hStdInput = pipe_child_read;
    si.StartupInfo.hStdOutput = pipe_child_write;
    si.StartupInfo.hStdError = pipe_child_write;

    std::vector<uint8_t> attrs;
    size_t attrsize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrsize);
    if (attrsize == 0) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` InitializeProcThreadAttributeList failed, ret error code:", err);
        return nullptr;
    }
    attrs.resize(attrsize);
    si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs.data());
    auto attr_success = InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrsize);
    if (!attr_success) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` InitializeProcThreadAttributeList failed, ret error code:", err);
        return nullptr;
    }
    attr_success = UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &handles_to_inherit, sizeof(handles_to_inherit), nullptr, nullptr);
    if (!attr_success) {
        DWORD err = GetLastError();
        Log.error("Call `", cmd, "` UpdateProcThreadAttribute failed, ret error code:", err);
        return nullptr;
    }
    auto cmd_osstr = utils::to_osstring(cmd);
    BOOL create_ret =
        CreateProcessW(NULL, cmd_osstr.data(), nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &si.StartupInfo, &m_process_info);
    DeleteProcThreadAttributeList(si.lpAttributeList);

    CloseHandle(pipe_child_write);
    CloseHandle(pipe_child_read);
    pipe_child_write = INVALID_HANDLE_VALUE;
    pipe_child_read = INVALID_HANDLE_VALUE;

    if (!create_ret) {
        DWORD err = GetLastError();
        Log.error("Failed to create process for minitouch, err", err);
        CloseHandle(m_process_info.hProcess);
        CloseHandle(m_process_info.hThread);
        CloseHandle(pipe_parent_read);
        CloseHandle(pipe_parent_write);
        return nullptr;
    }

    return std::make_shared<IOHandlerWin32>(pipe_parent_read, pipe_parent_write, m_process_info);
}

void asst::Win32IO::release_adb(const std::string& adb_release, int64_t timeout)
{
    std::string pipe_data;
    std::string sock_data;
    auto start_time = std::chrono::steady_clock::now();

    call_command(adb_release, false, pipe_data, sock_data, timeout, start_time);
}

asst::IOHandlerWin32::~IOHandlerWin32()
{
    if (m_process_info.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(m_process_info.hProcess);
        m_process_info.hProcess = INVALID_HANDLE_VALUE;
    }
    if (m_process_info.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(m_process_info.hThread);
        m_process_info.hThread = INVALID_HANDLE_VALUE;
    }
    if (m_read != INVALID_HANDLE_VALUE) {
        CloseHandle(m_read);
        m_read = INVALID_HANDLE_VALUE;
    }
    if (m_write != INVALID_HANDLE_VALUE) {
        CloseHandle(m_write);
        m_write = INVALID_HANDLE_VALUE;
    }
}

std::string asst::IOHandlerWin32::read(unsigned timeout_sec)
{
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    auto pipe_buffer = std::make_unique<char[]>(PipeBufferSize);
    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    std::ignore = ReadFile(m_read, pipe_buffer.get(), PipeBufferSize, nullptr, &pipeov);

    while (true) {
        if (!check_timeout(start_time)) {
            CancelIoEx(m_read, &pipeov);
            Log.error("read timeout");
            break;
        }
        DWORD len = 0;
        if (GetOverlappedResult(m_read, &pipeov, &len, FALSE)) {
            break;
        }
    }

    return pipe_buffer.get();
}

bool asst::IOHandlerWin32::write(std::string_view data)
{
    if (m_write == INVALID_HANDLE_VALUE) {
        Log.error("IOHandler write handle invalid", m_write);
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(m_write, data.data(), static_cast<DWORD>(data.size() * sizeof(std::string::value_type)), &written,
                   NULL)) {
        auto err = GetLastError();
        Log.error("Failed to write to IOHandlerWin32, err", err);
        return false;
    }

    return data.size() == written;
}
#endif
