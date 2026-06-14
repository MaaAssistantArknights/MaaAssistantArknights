#pragma once

#include <windows.h>

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
void WriteLog(const std::wstring& message);
void WriteLog(const wchar_t* message);
void WriteLogF(const wchar_t* fmt, ...);
void WriteLogEntries(const std::wstring& title, const std::vector<std::wstring>& entries);
void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline);
void RotateLogIfNeeded();
bool HasArgument(int argc, wchar_t* argv[], const wchar_t* argument);
