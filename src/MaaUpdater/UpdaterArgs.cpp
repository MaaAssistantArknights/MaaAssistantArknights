#include "UpdaterArgs.h"

// ---------------------------------------------------------------------------
// Helper: check if argument matches a key, case-insensitive
// ---------------------------------------------------------------------------

static bool MatchArg(const wchar_t* arg, const wchar_t* key)
{
    return _wcsicmp(arg, key) == 0;
}

// ---------------------------------------------------------------------------
// V2 format detection
// ---------------------------------------------------------------------------

bool IsV2Format(int argc, wchar_t* argv[])
{
    return argc >= 2 && MatchArg(argv[1], ARG_V2);
}

// ---------------------------------------------------------------------------
// New-format parser: --key value
// ---------------------------------------------------------------------------

namespace {

using WstrField = std::wstring UpdaterArgs::*;

struct ArgDef {
    const wchar_t* key;
    WstrField field;
    bool required;
};

constexpr ArgDef STRING_ARGS[] = {
    { ARG_ROOT_DIR,            &UpdaterArgs::rootDir,            true },
    { ARG_EXTRACT_DIR,         &UpdaterArgs::extractDir,         true },
    { ARG_BACKUP_DIR,          &UpdaterArgs::backupDir,          true },
    { ARG_PACKAGE_PATH,        &UpdaterArgs::packagePath,        true },
    { ARG_SUCCESS_STATUS_FILE, &UpdaterArgs::successStatusFile,  true },
    { ARG_FAILURE_STATUS_FILE, &UpdaterArgs::failureStatusFile,  true },
    { ARG_RELAUNCH_EXECUTABLE, &UpdaterArgs::relaunchExecutable, true },
    { ARG_PLAN_FILE,           &UpdaterArgs::planFile,           true },
    { ARG_MUTEX_NAME,          &UpdaterArgs::mutexName,          false },
};

UpdaterArgs ParseV2Args(int argc, wchar_t* argv[])
{
    UpdaterArgs args;
    args.valid = true;

    int pos = 2; // skip argv[0] (exe) and argv[1] ("-v2")

    while (pos < argc) {
        const wchar_t* key = argv[pos++];

        if (pos >= argc) {
            args.valid = false;
            args.errorMessage = L"Missing value for " + std::wstring(key);
            return args;
        }

        // String fields via lookup table
        bool matched = false;
        for (const auto& def : STRING_ARGS) {
            if (MatchArg(key, def.key)) {
                args.*(def.field) = argv[pos++];
                matched = true;
                break;
            }
        }
        if (matched) continue;

        // Special fields
        if (MatchArg(key, ARG_PARENT_PID)) {
            args.parentPid = static_cast<DWORD>(_wtol(argv[pos++]));
            continue;
        }

        if (MatchArg(key, ARG_SHOW_CONSOLE)) {
            args.showConsole = true;
            continue;
        }

        // Unknown
        args.valid = false;
        args.errorMessage = L"Unknown argument: " + std::wstring(key);
        return args;
    }

    // Validate required fields
    if (args.parentPid == 0) {
        args.valid = false;
        args.errorMessage = L"Missing required argument: " + std::wstring(ARG_PARENT_PID);
        return args;
    }

    for (const auto& def : STRING_ARGS) {
        if (def.required && (args.*(def.field)).empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(def.key);
            return args;
        }
    }

    return args;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Legacy positional parser
// ---------------------------------------------------------------------------

namespace {

UpdaterArgs ParseLegacyArgs(int argc, wchar_t* argv[])
{
    constexpr int REQUIRED_ARGS = 9; // excluding argv[0]

    UpdaterArgs args;

    if (argc - 1 < REQUIRED_ARGS) {
        args.valid = false;
        args.errorMessage = L"Insufficient arguments for legacy format. Expected at least "
                            + std::to_wstring(REQUIRED_ARGS) + L" positional arguments.";
        return args;
    }

    args.parentPid          = static_cast<DWORD>(_wtoi(argv[1]));
    args.rootDir            = argv[2];
    args.extractDir         = argv[3];
    args.backupDir          = argv[4];
    args.packagePath        = argv[5];
    args.successStatusFile  = argv[6];
    args.failureStatusFile  = argv[7];
    args.relaunchExecutable = argv[8];
    args.planFile           = argv[9];

    // Optional flags (same as before: --mutex-name <name>, --show-console)
    for (int i = REQUIRED_ARGS + 1; i < argc; ++i) {
        if (MatchArg(argv[i], ARG_MUTEX_NAME) && i + 1 < argc) {
            args.mutexName = argv[++i];
        } else if (MatchArg(argv[i], ARG_SHOW_CONSOLE)) {
            args.showConsole = true;
        }
    }

    args.valid = true;
    return args;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

UpdaterArgs ParseUpdaterArgs(int argc, wchar_t* argv[])
{
    if (IsV2Format(argc, argv)) {
        return ParseV2Args(argc, argv);
    }

    return ParseLegacyArgs(argc, argv);
}
