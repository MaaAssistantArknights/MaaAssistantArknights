#ifdef _WIN32
#include "PlatformWin32.h"
#include "Platform.h"

#include <atomic>
#include <format>
#include <mbctype.h>

#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

static size_t get_page_size()
{
    SYSTEM_INFO sysInfo {};
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

const size_t asst::platform::page_size = get_page_size();

void* asst::platform::aligned_alloc(size_t len, size_t align)
{
    return _aligned_malloc(len, align);
}

void asst::platform::aligned_free(void* ptr)
{
    _aligned_free(ptr);
}

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

static std::string get_ansi_short_path(const std::filesystem::path& path)
{
    wchar_t short_path[MAX_PATH] {};
    auto& osstr = path.native();
    auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
    if (shortlen == 0) return {};
    BOOL failed = FALSE;
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
    if (failed) return {};
    std::string result(ansilen, 0);
    WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, result.data(), ansilen, nullptr, nullptr);
    return result;
}

std::string asst::platform::path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize = 0;
    auto& osstr = path.native();
    auto err = wcstombs_s(&mbsize, nullptr, 0, osstr.c_str(), osstr.size());
    if (err == 0) {
        std::string result(mbsize, 0);
        err = wcstombs_s(&mbsize, result.data(), mbsize, osstr.c_str(), osstr.size());
        if (err != 0) return {};
        return result.substr(0, mbsize - 1);
    }
    else {
        // cannot convert (CRT is not using UTF-8), fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
}

std::string asst::platform::path_to_ansi_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    BOOL failed = FALSE;
    auto& osstr = path.native();
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), nullptr, 0, nullptr, &failed);
    if (!failed) {
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), result.data(), ansilen, nullptr, &failed);
        return result;
    }
    else {
        // contains character that cannot be converted, fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
}

asst::platform::os_string asst::platform::to_osstring(const std::string& utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), nullptr, 0);
    os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), result.data(), len);
    return result;
}

std::string asst::platform::from_osstring(const os_string& os_str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), result.data(), len, nullptr, nullptr);
    return result;
}

std::string asst::platform::call_command(const std::string& cmdline, bool* exit_flag)
{
    constexpr int PipeBuffSize = 4096;
    auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);
    std::string pipe_str;

    HANDLE pipe_parent_read = INVALID_HANDLE_VALUE, pipe_child_write = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa_inherit { .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
    if (!asst::win32::CreateOverlappablePipe(&pipe_parent_read, &pipe_child_write, nullptr, &sa_inherit, PipeBuffSize,
                                             true, false)) {
        DWORD err = GetLastError();
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

    auto cmdline_osstr = to_osstring(cmdline);
    BOOL create_ret =
        CreateProcessW(nullptr, cmdline_osstr.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &process_info);
    if (!create_ret) {
        DWORD err = GetLastError();
        Log.error("Call `", cmdline, "` create process failed, ret", create_ret, "error code:", err);
        return {};
    }

    CloseHandle(pipe_child_write);

    std::vector<HANDLE> wait_handles;
    wait_handles.reserve(2);
    bool process_running = true;
    bool pipe_eof = false;

    OVERLAPPED pipeov { .hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
    (void)ReadFile(pipe_parent_read, pipe_buffer.get(), PipeBuffSize, nullptr, &pipeov);

    while (!(exit_flag && *exit_flag)) {
        wait_handles.clear();
        if (process_running) wait_handles.push_back(process_info.hProcess);
        if (!pipe_eof) wait_handles.push_back(pipeov.hEvent);
        if (wait_handles.empty()) break;
        auto wait_result =
            WaitForMultipleObjectsEx((DWORD)wait_handles.size(), wait_handles.data(), FALSE, INFINITE, TRUE);
        HANDLE signaled_object = INVALID_HANDLE_VALUE;
        if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + wait_handles.size()) {
            signaled_object = wait_handles[(size_t)wait_result - WAIT_OBJECT_0];
        }
        else if (wait_result == WAIT_TIMEOUT) {
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
                pipe_str.insert(pipe_str.end(), pipe_buffer.get(), pipe_buffer.get() + len);
                (void)ReadFile(pipe_parent_read, pipe_buffer.get(), PipeBuffSize, nullptr, &pipeov);
            }
            else {
                DWORD err = GetLastError();
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
    if (pipeov.hEvent) {
        CloseHandle(pipeov.hEvent);
    }
    return pipe_str;
}

#define REPARSE_MOUNTPOINT_HEADER_SIZE 8

struct REPARSE_MOUNTPOINT_DATA_BUFFER
{
    DWORD ReparseTag;
    DWORD ReparseDataLength;
    WORD Reserved;
    WORD ReparseTargetLength;
    WORD ReparseTargetMaximumLength;
    WORD Reserved1;
    WCHAR ReparseTarget[1];
};

struct REPARSE_DATA_BUFFER
{
    DWORD ReparseTag;
    WORD ReparseDataLength;
    WORD Reserved;
    union {
        struct
        {
            WORD SubstituteNameOffset;
            WORD SubstituteNameLength;
            WORD PrintNameOffset;
            WORD PrintNameLength;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            WORD SubstituteNameOffset;
            WORD SubstituteNameLength;
            WORD PrintNameOffset;
            WORD PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            BYTE DataBuffer[1];
        } GenericReparseBuffer;
    };
};

#define REPARSE_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

HANDLE asst::win32::OpenDirectory(const std::filesystem::path& path, BOOL bReadWrite)
{
    // Obtain backup/restore privilege in case we don't have it
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    LookupPrivilegeValueW(NULL, (bReadWrite ? SE_RESTORE_NAME : SE_BACKUP_NAME), &tp.Privileges[0].Luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);

    // Open the directory
    DWORD dwAccess = bReadWrite ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    HANDLE hDir = CreateFileW(path.c_str(), dwAccess, 0, NULL, OPEN_EXISTING,
                              FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    return hDir;
}

bool asst::win32::SetDirectoryReparsePoint(const std::filesystem::path& link, const std::filesystem::path& target)
{
    auto normtarget = asst::platform::path(asst::utils::string_replace_all(target.native(), L"/", L"\\"));

    auto nttarget = L"\\GLOBAL??\\" + std::filesystem::absolute(normtarget).native();
    if (nttarget.back() != L'\\') nttarget.push_back(L'\\');

    // set reparse point
    auto hReparsePoint = OpenDirectory(link.c_str(), TRUE);

    if (hReparsePoint == INVALID_HANDLE_VALUE) return false;

    BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;

    // Prepare reparse point data
    memset(buf, 0, sizeof(buf));
    ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    wcsncpy_s(ReparseBuffer.ReparseTarget, MAX_PATH + 1, nttarget.c_str(), MAX_PATH);
    ReparseBuffer.ReparseTargetMaximumLength = (WORD)((nttarget.size() + 1) * sizeof(WCHAR));
    ReparseBuffer.ReparseTargetLength = (WORD)(nttarget.size() * sizeof(WCHAR));
    ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;

    // Attach reparse point
    auto success =
        DeviceIoControl(hReparsePoint, FSCTL_SET_REPARSE_POINT, &ReparseBuffer,
                        ReparseBuffer.ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE, nullptr, 0, nullptr, nullptr);

    CloseHandle(hReparsePoint);

    return success;
}

#endif
