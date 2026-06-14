#pragma once

#include <windows.h>

#include <array>
#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Logging globals (defined in UpdaterLog.cpp)
// ---------------------------------------------------------------------------

extern std::wstring g_logFile;
extern bool g_writeConsoleLog;

// ---------------------------------------------------------------------------
// Logging functions
// ---------------------------------------------------------------------------

bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8);
HANDLE GetConsoleStreamHandle(FILE* stream);

// Core implementation (defined in UpdaterLog.cpp)
void WriteLogImpl(const wchar_t* message);

// Helper to extract const wchar_t* from wstring / wchar_t* / array types
namespace detail {
    inline const wchar_t* GetLogCStr(const wchar_t* msg)       { return msg; }
    inline const wchar_t* GetLogCStr(wchar_t* msg)             { return msg; }
    inline const wchar_t* GetLogCStr(const std::wstring& msg)  { return msg.c_str(); }
}

// Template wrapper — accepts std::wstring, const wchar_t*, wchar_t[], etc.
template <typename StringLike>
void WriteLog(const StringLike& message)
{
    WriteLogImpl(detail::GetLogCStr(message));
}

// Variadic template — replaces C-style va_list WriteLogF
template <typename... Args>
void WriteLogF(const wchar_t* fmt, Args&&... args)
{
    std::array<wchar_t, 2048> buf{};
    std::swprintf(buf.data(), buf.size(), fmt, std::forward<Args>(args)...);
    WriteLog(buf.data());
}

void WriteLogEntries(const std::wstring& title, const std::vector<std::wstring>& entries);
void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline);
void RotateLogIfNeeded();
bool HasArgument(int argc, wchar_t* argv[], const wchar_t* argument);
