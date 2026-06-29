#pragma once

#include <windows.h>

#include <string>

// ---------------------------------------------------------------------------
// Parsed updater arguments
// ---------------------------------------------------------------------------

struct UpdaterArgs
{
    DWORD parentPid = 0;
    std::wstring rootDir;
    std::wstring extractDir;
    std::wstring backupDir;
    std::wstring packagePath;
    std::wstring successStatusFile;
    std::wstring failureStatusFile;
    std::wstring relaunchExecutable;
    std::wstring planFile;
    std::wstring mutexName;
    bool showConsole = false;
    bool valid = false;
    std::wstring errorMessage;
};

// ---------------------------------------------------------------------------
// Argument parsing
// ---------------------------------------------------------------------------

// Parses command-line arguments.
// If the first argument is "-v2", uses the new "--key value" format.
// Otherwise falls back to the legacy positional argument format.
UpdaterArgs ParseUpdaterArgs(int argc, wchar_t* argv[]);

// Returns true if the first argument is "-v2" (new format).
bool IsV2Format(int argc, wchar_t* argv[]);

// ---------------------------------------------------------------------------
// New-format argument names
// ---------------------------------------------------------------------------

// clang-format off
constexpr wchar_t ARG_V2[]                  = L"-v2";
constexpr wchar_t ARG_PARENT_PID[]          = L"--parent-pid";
constexpr wchar_t ARG_ROOT_DIR[]            = L"--root-dir";
constexpr wchar_t ARG_EXTRACT_DIR[]         = L"--extract-dir";
constexpr wchar_t ARG_BACKUP_DIR[]          = L"--backup-dir";
constexpr wchar_t ARG_PACKAGE_PATH[]        = L"--package-path";
constexpr wchar_t ARG_SUCCESS_STATUS_FILE[] = L"--success-status-file";
constexpr wchar_t ARG_FAILURE_STATUS_FILE[] = L"--failure-status-file";
constexpr wchar_t ARG_RELAUNCH_EXECUTABLE[] = L"--relaunch-executable";
constexpr wchar_t ARG_PLAN_FILE[]           = L"--plan-file";
constexpr wchar_t ARG_MUTEX_NAME[]          = L"--mutex-name";
constexpr wchar_t ARG_SHOW_CONSOLE[]        = L"--show-console";
// clang-format on
