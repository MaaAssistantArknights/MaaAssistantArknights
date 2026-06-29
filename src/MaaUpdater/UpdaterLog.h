#pragma once

#include <array>
#include <string>

// ---------------------------------------------------------------------------
// Logging globals (defined in UpdaterLog.cpp)
// ---------------------------------------------------------------------------

extern std::wstring g_logFile;
extern bool g_writeConsoleLog;

// ---------------------------------------------------------------------------
// Logging functions
// ---------------------------------------------------------------------------

bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8);

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
namespace detail {
    // Default: pass through (int, DWORD, size_t, const wchar_t*, etc.)
    template <typename T>
    T FmtArg(T&& arg) { return std::forward<T>(arg); }

    // wstring → c_str()
    inline const wchar_t* FmtArg(const std::wstring& s) { return s.c_str(); }
    inline const wchar_t* FmtArg(std::wstring& s)       { return s.c_str(); }
    inline const wchar_t* FmtArg(std::wstring&& s)      { return s.c_str(); }
}

template <typename... Args>
void WriteLogF(const wchar_t* fmt, Args&&... args)
{
    std::array<wchar_t, 2048> buf{};
    std::swprintf(buf.data(), buf.size(), fmt, detail::FmtArg(std::forward<Args>(args))...);
    WriteLog(buf.data());
}

void RotateLogIfNeeded();
