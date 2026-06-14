#include "UpdaterLog.h"

#include <windows.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

std::wstring g_logFile;
bool g_writeConsoleLog = false;

static constexpr auto MAX_UPDATER_LOG_SIZE = 4LL * 1024 * 1024;

// ---------------------------------------------------------------------------
// UTF-8 conversion
// ---------------------------------------------------------------------------

bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8)
{
    const int utf8Len =
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) {
        utf8.clear();
        return false;
    }

    utf8.assign(static_cast<size_t>(utf8Len), '\0');
    return WideCharToMultiByte(
               CP_UTF8,
               0,
               wide.c_str(),
               static_cast<int>(wide.size()),
               utf8.data(),
               utf8Len,
               nullptr,
               nullptr) == utf8Len;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void WriteLogImpl(const wchar_t* message)
{
    const auto now = std::chrono::system_clock::now();
    const auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf {};
    localtime_s(&tmBuf, &nowTimeT);

    std::array<wchar_t, 64> buf {};
    std::swprintf(
        buf.data(),
        buf.size(),
        L"[%04d-%02d-%02d %02d:%02d:%02d.%03lld] ",
        tmBuf.tm_year + 1900,
        tmBuf.tm_mon + 1,
        tmBuf.tm_mday,
        tmBuf.tm_hour,
        tmBuf.tm_min,
        tmBuf.tm_sec,
        static_cast<long long>(nowMs.count()));

    const std::wstring timestampedLine = std::wstring(buf.data()) + message;

    if (g_writeConsoleLog) {
        std::wcout << timestampedLine << L'\n';
        std::wcout.flush();
    }

    if (g_logFile.empty()) {
        return;
    }

    // Ensure parent directory exists
    if (const size_t sep = g_logFile.rfind(L'\\'); sep != std::wstring::npos) {
        std::error_code ec;
        std::filesystem::create_directories(g_logFile.substr(0, sep), ec);
    }

    std::ofstream file(g_logFile, std::ios::app | std::ios::binary);
    if (!file) {
        return;
    }

    std::string utf8;
    if (TryConvertWideToUtf8(timestampedLine + L"\r\n", utf8)) {
        file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    }
}

void RotateLogIfNeeded()
{
    if (g_logFile.empty()) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(g_logFile, ec)) {
        return;
    }

    const auto fileSize = std::filesystem::file_size(g_logFile, ec);
    if (ec || fileSize <= MAX_UPDATER_LOG_SIZE) {
        return;
    }

    const std::wstring bakFile = g_logFile + L".bak";
    std::filesystem::remove(bakFile, ec);
    std::filesystem::rename(g_logFile, bakFile, ec);
}
