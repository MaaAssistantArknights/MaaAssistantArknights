#include "AsstUtils.hpp"

#ifdef _WIN32

#include <mbctype.h>

std::string asst::utils::path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize;
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
    static std::atomic<size_t> pipeid {};
    constexpr int PipeBuffSize = 4096;
    std::string pipe_str;
    auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);
    ASST_AUTO_DEDUCED_ZERO_INIT_START
    SECURITY_ATTRIBUTES pipe_sec_attr = { 0 };
    ASST_AUTO_DEDUCED_ZERO_INIT_END
    pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipe_sec_attr.lpSecurityDescriptor = nullptr;
    pipe_sec_attr.bInheritHandle = TRUE;
    auto pipename = std::format(L"\\\\.\\pipe\\maa-callcmd-{}-{}", GetCurrentProcessId(), pipeid++);
    auto pipe_read =
        CreateNamedPipeW(pipename.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_WAIT, 1, PipeBuffSize, PipeBuffSize, 0, nullptr);
    auto pipe_child_write =
        CreateFileW(pipename.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    ASST_AUTO_DEDUCED_ZERO_INIT_START
    STARTUPINFOW si = { 0 };
    ASST_AUTO_DEDUCED_ZERO_INIT_END
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;

    ASST_AUTO_DEDUCED_ZERO_INIT_START
    PROCESS_INFORMATION pi = { nullptr };
    ASST_AUTO_DEDUCED_ZERO_INIT_END

    auto cmdline_osstr = asst::utils::to_osstring(cmdline);
    BOOL p_ret = CreateProcessW(nullptr, &cmdline_osstr[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                                &si, &pi);
    if (p_ret) {
        DWORD read_num = 0;
        OVERLAPPED ov { .hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr) };
        bool reading = false;
        bool process_running = true;
        bool pipe_eof = false;
        bool process_signaled = false, pipe_signaled = false;
        HANDLE wait_handles[] { pi.hProcess, ov.hEvent };
        while (process_running || !pipe_eof) {
            if (!pipe_eof && !reading) {
                (void)ReadFile(pipe_read, pipe_buffer.get(), PipeBuffSize, nullptr, &ov);
                reading = true;
            }
            DWORD wait_result = WAIT_FAILED;
            if (process_running && !pipe_eof) {
                wait_result = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);
                process_signaled = wait_result == WAIT_OBJECT_0;
                pipe_signaled = wait_result == WAIT_OBJECT_0 + 1;
            }
            else if (!pipe_eof) { // process exited, pipe not eof yet
                wait_result = WaitForMultipleObjects(1, wait_handles + 1, FALSE, INFINITE);
                pipe_signaled = wait_result == WAIT_OBJECT_0;
            }
            else if (process_running) { // pipe eof, process not exit yet
                wait_result = WaitForMultipleObjects(1, wait_handles, FALSE, INFINITE);
                process_signaled = wait_result == WAIT_OBJECT_0;
            }
            if (wait_result == WAIT_FAILED) {
                // something bad happened
                abort();
            }
            if (process_signaled) {
                // process died, wait for data remaining in pipe
                process_running = false;
            }
            if (pipe_signaled) {
                reading = false;
                // pipe read
                if (GetOverlappedResult(pipe_read, &ov, &read_num, TRUE)) {
                    pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                }
                else {
                    if (GetLastError() == ERROR_HANDLE_EOF) {
                        pipe_eof = true;
                    }
                }
            }
        };

        // fetch data in pipe buffer (if any)
        //if (reading) {
        //    if (GetOverlappedResult(pipe_read, &ov, &read_num, FALSE)) {
        //        pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
        //    }
        //    else {
        //        CancelIo(pipe_read);
        //    }
        //}

        //while (PeekNamedPipe(pipe_read, nullptr, 0, nullptr, &peek_num, nullptr) && peek_num > 0) {
        //    ReadFile(pipe_read, pipe_buffer.get(), PipeBuffSize, nullptr, &ov);
        //    if (GetOverlappedResult(pipe_read, &ov, &read_num, TRUE)) {
        //        pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
        //    }
        //}

        DWORD exit_ret = 255;
        GetExitCodeProcess(pi.hProcess, &exit_ret);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(ov.hEvent);
    }

    CloseHandle(pipe_read);
    CloseHandle(pipe_child_write);

    return pipe_str;
}

#else
std::string asst::utils::callcmd(const std::string& cmdline)
{
    constexpr int PipeBuffSize = 4096;
    std::string pipe_str;
    auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);

    constexpr static int PIPE_READ = 0;
    constexpr static int PIPE_WRITE = 1;
    int pipe_in[2] = { 0 };
    int pipe_out[2] = { 0 };
    int pipe_in_ret = pipe(pipe_in);
    int pipe_out_ret = pipe(pipe_out);
    if (pipe_in_ret != 0 || pipe_out_ret != 0) {
        return {};
    }
    fcntl(pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);
    int exit_ret = 0;
    int child = fork();
    if (child == 0) {
        // child process
        dup2(pipe_in[PIPE_READ], STDIN_FILENO);
        dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
        dup2(pipe_out[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        close(pipe_in[PIPE_READ]);
        close(pipe_in[PIPE_WRITE]);
        close(pipe_out[PIPE_READ]);
        close(pipe_out[PIPE_WRITE]);

        exit_ret = execlp("sh", "sh", "-c", cmdline.c_str(), nullptr);
        exit(exit_ret);
    }
    else if (child > 0) {
        // parent process

        // close unused file descriptors, these are for child only
        close(pipe_in[PIPE_READ]);
        close(pipe_out[PIPE_WRITE]);

        do {
            ssize_t read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);

            while (read_num > 0) {
                pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);
            };
        } while (::waitpid(child, &exit_ret, WNOHANG) == 0);

        close(pipe_in[PIPE_WRITE]);
        close(pipe_out[PIPE_READ]);
    }
    else {
        // failed to create child process
        close(pipe_in[PIPE_READ]);
        close(pipe_in[PIPE_WRITE]);
        close(pipe_out[PIPE_READ]);
        close(pipe_out[PIPE_WRITE]);
    }
    return pipe_str;
}


#endif
