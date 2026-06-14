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

struct V2Parser {
    int argc;
    wchar_t** argv;
    int pos = 0; // current parsing position (1-indexed, as argv[0] is exe name)

    bool done() const { return pos >= argc; }

    const wchar_t* current() const {
        return (pos < argc) ? argv[pos] : L"";
    }

    void advance() { ++pos; }

    bool tryConsumeValue(std::wstring& out) {
        advance();
        if (done()) return false;
        out = current();
        advance();
        return true;
    }

    UpdaterArgs parse() {
        UpdaterArgs args;
        args.valid = true;

        // Skip argv[0] (exe) and argv[1] ("-v2")
        pos = 2;

        while (!done()) {
            const wchar_t* key = current();
            advance();

            if (MatchArg(key, ARG_PARENT_PID)) {
                std::wstring val;
                if (!tryConsumeValue(val)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_PARENT_PID);
                    return args;
                }
                args.parentPid = static_cast<DWORD>(_wtol(val.c_str()));
                continue;
            }

            if (MatchArg(key, ARG_ROOT_DIR)) {
                if (!tryConsumeValue(args.rootDir)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_ROOT_DIR);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_EXTRACT_DIR)) {
                if (!tryConsumeValue(args.extractDir)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_EXTRACT_DIR);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_BACKUP_DIR)) {
                if (!tryConsumeValue(args.backupDir)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_BACKUP_DIR);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_PACKAGE_PATH)) {
                if (!tryConsumeValue(args.packagePath)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_PACKAGE_PATH);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_SUCCESS_STATUS_FILE)) {
                if (!tryConsumeValue(args.successStatusFile)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_SUCCESS_STATUS_FILE);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_FAILURE_STATUS_FILE)) {
                if (!tryConsumeValue(args.failureStatusFile)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_FAILURE_STATUS_FILE);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_RELAUNCH_EXECUTABLE)) {
                if (!tryConsumeValue(args.relaunchExecutable)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_RELAUNCH_EXECUTABLE);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_PLAN_FILE)) {
                if (!tryConsumeValue(args.planFile)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_PLAN_FILE);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_MUTEX_NAME)) {
                if (!tryConsumeValue(args.mutexName)) {
                    args.valid = false;
                    args.errorMessage = L"Missing value for " + std::wstring(ARG_MUTEX_NAME);
                    return args;
                }
                continue;
            }

            if (MatchArg(key, ARG_SHOW_CONSOLE)) {
                args.showConsole = true;
                continue;
            }

            // Unknown argument
            args.valid = false;
            args.errorMessage = L"Unknown argument: " + std::wstring(key);
            return args;
        }

        // Validate required arguments
        if (args.parentPid == 0) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_PARENT_PID);
            return args;
        }
        if (args.rootDir.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_ROOT_DIR);
            return args;
        }
        if (args.extractDir.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_EXTRACT_DIR);
            return args;
        }
        if (args.backupDir.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_BACKUP_DIR);
            return args;
        }
        if (args.packagePath.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_PACKAGE_PATH);
            return args;
        }
        if (args.successStatusFile.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_SUCCESS_STATUS_FILE);
            return args;
        }
        if (args.failureStatusFile.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_FAILURE_STATUS_FILE);
            return args;
        }
        if (args.relaunchExecutable.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_RELAUNCH_EXECUTABLE);
            return args;
        }
        if (args.planFile.empty()) {
            args.valid = false;
            args.errorMessage = L"Missing required argument: " + std::wstring(ARG_PLAN_FILE);
            return args;
        }

        return args;
    }
};

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
        return V2Parser{argc, argv}.parse();
    }

    return ParseLegacyArgs(argc, argv);
}
