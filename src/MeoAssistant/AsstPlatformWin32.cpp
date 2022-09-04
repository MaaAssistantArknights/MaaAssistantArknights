#ifdef _WIN32
#include "AsstPlatformWin32.h"

#include <atomic>
#include <format>
#include <mbctype.h>

#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::win32::CreateOverlappablePipe(HANDLE* read, HANDLE* write, SECURITY_ATTRIBUTES* secattr_read,
                                         SECURITY_ATTRIBUTES* secattr_write, DWORD bufsize, bool overlapped_read,
                                         bool overlapped_write)
{
    static std::atomic<size_t> pipeid {};
    auto pipename = std::format(L"\\\\.\\pipe\\maa-pipe-{}-{}", GetCurrentProcessId(), pipeid++);
    DWORD read_flag = PIPE_ACCESS_INBOUND;
    if (overlapped_read) read_flag |= FILE_FLAG_OVERLAPPED;
    DWORD write_flag = GENERIC_WRITE;
    if (overlapped_write) write_flag |= FILE_FLAG_OVERLAPPED;
    auto pipe_read =
        CreateNamedPipeW(pipename.c_str(), read_flag, PIPE_TYPE_BYTE | PIPE_WAIT, 1, bufsize, bufsize, 0, secattr_read);
    if (pipe_read == INVALID_HANDLE_VALUE) return false;
    auto pipe_write =
        CreateFileW(pipename.c_str(), write_flag, 0, secattr_write, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pipe_write == INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_read);
        return false;
    }

    *read = pipe_read;
    *write = pipe_write;
    return true;
}


std::string asst::utils::path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize = 0;
    auto osstr = path.native();
    auto err = wcstombs_s(&mbsize, nullptr, 0, osstr.c_str(), osstr.size());
    if (err == 0) {
        std::string result(mbsize, 0);
        err = wcstombs_s(&mbsize, &result[0], mbsize, osstr.c_str(), osstr.size());
        if (err != 0) return {};
        return result.substr(0, mbsize - 1);
    }
    else {
        // cannot convert (CRT is not using UTF-8), fallback to short path name in ACP
        wchar_t short_path[MAX_PATH];
        auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
        if (shortlen == 0) return {};
        BOOL failed = FALSE;
        auto ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
        if (failed) return {};
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, &result[0], ansilen, nullptr, nullptr);
        return result;
    }
}

std::string asst::utils::path_to_ansi_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    BOOL failed = FALSE;
    auto osstr = path.native();
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), nullptr, 0, nullptr, &failed);
    if (!failed) {
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), &result[0], ansilen, nullptr, &failed);
        return result;
    }
    else {
        // contains character that cannot be converted, fallback to short path name in ACP
        wchar_t short_path[MAX_PATH];
        auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
        if (shortlen == 0) return {};
        ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
        if (failed) return {};
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, &result[0], ansilen, nullptr, nullptr);
        return result;
    }
}

asst::utils::os_string asst::utils::to_osstring(const std::string& utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), nullptr, 0);
    asst::utils::os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), &result[0], len);
    return result;
}

std::string asst::utils::from_osstring(const asst::utils::os_string& os_str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), &result[0], len, nullptr, nullptr);
    return result;
}

std::string asst::utils::callcmd(const std::string& cmdline)
{
    constexpr int PipeBuffSize = 4096;
    auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);
    std::string pipe_str;

    DWORD err = 0;
    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa_inherit { .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_inherit, PipeBuffSize,
                                             true, false)) {
        err = GetLastError();
        asst::Log.error("CreateOverlappablePipe failed, err", err);
        return {};
    }

    STARTUPINFOW si {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;
    ASST_AUTO_DEDUCED_ZERO_INIT_START
    PROCESS_INFORMATION process_info = { nullptr };
    ASST_AUTO_DEDUCED_ZERO_INIT_END

    auto cmdline_osstr = asst::utils::to_osstring(cmdline);
    BOOL create_ret =
        CreateProcessW(nullptr, &cmdline_osstr[0], nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &process_info);
    if (!create_ret) {
        Log.error("Call `", cmdline, "` create process failed, ret", create_ret);
        return {};
    }

    CloseHandle(pipe_child_write);

    std::vector<HANDLE> wait_handles;
    wait_handles.reserve(2);
    bool process_running = true;
    bool pipe_eof = false;

    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    (void)ReadFile(pipe_parent_read, pipe_buffer.get(), PipeBuffSize, nullptr, &pipeov);

    while (1) {
        wait_handles.clear();
        if (process_running) wait_handles.push_back(process_info.hProcess);
        if (!pipe_eof) wait_handles.push_back(pipeov.hEvent);
        if (wait_handles.empty()) break;
        auto wait_result =
            WaitForMultipleObjectsEx((DWORD)wait_handles.size(), &wait_handles[0], FALSE, INFINITE, TRUE);
        HANDLE signaled_object = INVALID_HANDLE_VALUE;
        if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + wait_handles.size()) {
            signaled_object = wait_handles[(size_t)wait_result - WAIT_OBJECT_0];
        }
        else if (wait_result == WAIT_TIMEOUT) {
            continue;
        }
        else {
            // something bad happened
            err = GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }

        if (signaled_object == process_info.hProcess) {
            process_running = false;
        }
        else if (signaled_object == pipeov.hEvent) {
            // pipe read
            DWORD len = 0;
            if (GetOverlappedResult(pipe_parent_read, &pipeov, &len, FALSE)) {
                pipe_str.insert(pipe_str.end(), pipe_buffer.get(), pipe_buffer.get() + len);
                (void)ReadFile(pipe_parent_read, pipe_buffer.get(), PipeBuffSize, nullptr, &pipeov);
            }
            else {
                err = GetLastError();
                if (err == ERROR_HANDLE_EOF || err == ERROR_BROKEN_PIPE) {
                    pipe_eof = true;
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
    return pipe_str;
}

#endif
