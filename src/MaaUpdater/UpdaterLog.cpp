#include "UpdaterLog.h"
#include "UpdaterFile.h"
#include "UpdaterHandle.h"

#include <ctime>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

std::wstring g_logFile;
bool g_writeConsoleLog = false;

static constexpr LONGLONG MAX_UPDATER_LOG_SIZE = 4LL * 1024 * 1024;

// ---------------------------------------------------------------------------
// UTF-8 conversion
// ---------------------------------------------------------------------------

bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8)
{
    const int utf8Len = WideCharToMultiByte(
        CP_UTF8, 0,
        wide.c_str(), static_cast<int>(wide.size()),
        nullptr, 0,
        nullptr, nullptr);
    if (utf8Len <= 0) {
        utf8.clear();
        return false;
    }

    utf8.assign(static_cast<size_t>(utf8Len), '\0');
    return WideCharToMultiByte(
        CP_UTF8, 0,
        wide.c_str(), static_cast<int>(wide.size()),
        utf8.data(), utf8Len,
        nullptr, nullptr) == utf8Len;
}

HANDLE GetConsoleStreamHandle(FILE* stream)
{
    const DWORD stdHandle = (stream == stderr)  ? STD_ERROR_HANDLE
                          : (stream == stdout)  ? STD_OUTPUT_HANDLE
                          : static_cast<DWORD>(-1);
    if (stdHandle == static_cast<DWORD>(-1)) {
        return nullptr;
    }

    const HANDLE handle = GetStdHandle(stdHandle);
    return (handle == INVALID_HANDLE_VALUE) ? nullptr : handle;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void WriteLogImpl(const wchar_t* message)
{
    SYSTEMTIME st{};
    GetLocalTime(&st);

    std::array<wchar_t, 64> timestampBuf{};
    std::swprintf(timestampBuf.data(), timestampBuf.size(),
                  L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                  st.wYear, st.wMonth, st.wDay,
                  st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    std::wstring timestampedLine = timestampBuf.data();
    timestampedLine += message;

    if (g_writeConsoleLog) {
        WriteConsoleText(stdout, timestampedLine, true);
    }

    if (g_logFile.empty()) {
        return;
    }

    std::wstring line = timestampedLine;
    line += L"\r\n";

    // Ensure parent directory exists
    if (const size_t sep = g_logFile.rfind(L'\\'); sep != std::wstring::npos) {
        std::wstring dir = g_logFile.substr(0, sep);
        CreateDirectoryW(dir.c_str(), nullptr);
    }

    ScopedHandle hFile(CreateFileW(
        g_logFile.c_str(),
        FILE_APPEND_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!hFile) {
        return;
    }

    std::string utf8;
    if (TryConvertWideToUtf8(line, utf8)) {
        DWORD written = 0;
        WriteFile(hFile.get(), utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
}

void WriteLogEntries(const std::wstring& title, const std::vector<std::wstring>& entries)
{
    WriteLogF(L"%s (%zu)", title.c_str(), entries.size());
    for (const std::wstring& entry : entries) {
        WriteLog(L"  - " + entry);
    }
}

void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline)
{
    std::wstring outputText = text;
    if (appendNewline) {
        outputText += L"\n";
    }

    const HANDLE consoleHandle = GetConsoleStreamHandle(stream);
    DWORD consoleMode = 0;
    if (consoleHandle && GetConsoleMode(consoleHandle, &consoleMode)) {
        DWORD written = 0;
        WriteConsoleW(
            consoleHandle,
            outputText.c_str(),
            static_cast<DWORD>(outputText.size()),
            &written,
            nullptr);
        return;
    }

    std::string utf8;
    if (TryConvertWideToUtf8(outputText, utf8)) {
        fwrite(utf8.data(), 1, utf8.size(), stream);
    }
    fflush(stream);
}

void RotateLogIfNeeded()
{
    if (g_logFile.empty() || !PathExistsW(g_logFile)) {
        return;
    }

    LARGE_INTEGER size{};
    {
        ScopedHandle hFile(CreateFileW(
            g_logFile.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!hFile) {
            return;
        }

        const bool shouldRotate = GetFileSizeEx(hFile.get(), &size) && size.QuadPart > MAX_UPDATER_LOG_SIZE;
        if (!shouldRotate) {
            return;
        }
    }

    std::wstring bakFile = g_logFile + L".bak";
    DeleteFileW(bakFile.c_str());
    MoveFileExW(g_logFile.c_str(), bakFile.c_str(), MOVEFILE_REPLACE_EXISTING);
}

bool HasArgument(int argc, wchar_t* argv[], const wchar_t* argument)
{
    for (int index = 1; index < argc; ++index) {
        if (_wcsicmp(argv[index], argument) == 0) {
            return true;
        }
    }

    return false;
}
