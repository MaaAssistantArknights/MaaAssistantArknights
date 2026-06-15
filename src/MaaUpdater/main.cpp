// MAA.Updater.exe
// Applies a pending MAA update package after the main MAA process exits.
// Invoked by MaaWpfGui's PendingUpdateApplier; do NOT run manually.
//
// Usage (new format, recommended):
//   MAA.Updater.exe -v2
//       --parent-pid <ParentProcessId>
//       --root-dir <RootDir>
//       --extract-dir <ExtractDir>
//       --backup-dir <BackupDir>
//       --package-path <PackagePath>
//       --success-status-file <SuccessStatusFile>
//       --failure-status-file <FailureStatusFile>
//       --relaunch-executable <RelaunchExecutablePath>
//       --plan-file <PlanFile>
//       [--mutex-name <name>] [--show-console]
//
// Usage (legacy format, deprecated):
//   MAA.Updater.exe <ParentProcessId> <RootDir> <ExtractDir> <BackupDir>
//                   <PackagePath> <SuccessStatusFile> <FailureStatusFile>
//                   <RelaunchExecutablePath> <PlanFile>
//                   [--mutex-name <name>] [--show-console]
//
// Plan file format (UTF-8 JSON):
//   { "packageType": "full|ota", "removeList": ["rel/path", ...], "moveList": ["rel/path", ...] }

#include <windows.h>

#include <shellapi.h>

#include <string>

#include "UpdaterArgs.h"
#include "UpdaterFile.h"
#include "UpdaterLog.h"
#include "UpdaterPath.h"
#include "UpdaterPlan.h"
#include "UpdaterUI.h"

static constexpr wchar_t MAA_UPDATER_LOG_FILENAME[] = L"\\debug\\pending-update-applier.log";
static constexpr DWORD UPDATE_MUTEX_TIMEOUT_MS = 3000;

// ---------------------------------------------------------------------------
// Update mutex helpers
// ---------------------------------------------------------------------------

static HANDLE AcquireUpdateMutex(const std::wstring& mutexName)
{
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (hMutex == nullptr) {
        WriteLogF(L"CreateMutexW failed, error=%lu", GetLastError());
        return nullptr;
    }

    const DWORD waitResult = WaitForSingleObject(hMutex, UPDATE_MUTEX_TIMEOUT_MS);
    if (waitResult == WAIT_OBJECT_0) {
        WriteLogF(L"Mutex acquired: %s", mutexName);
        return hMutex;
    }
    if (waitResult == WAIT_ABANDONED) {
        WriteLogF(L"Mutex acquired after WAIT_ABANDONED (previous instance crashed): %s", mutexName);
        return hMutex;
    }

    if (waitResult == WAIT_TIMEOUT) {
        WriteLogF(L"Mutex acquisition timed out after %lums: %s", UPDATE_MUTEX_TIMEOUT_MS, mutexName);
    }
    else {
        WriteLogF(L"WaitForSingleObject failed, error=%lu", GetLastError());
    }
    CloseHandle(hMutex);
    return nullptr;
}

static void ReleaseUpdateMutex(HANDLE hMutex)
{
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
}

static void SetFileUpdateTime(const std::wstring& path)
{
    HANDLE hFile = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    SetFileTime(hFile, nullptr, nullptr, &ft);
    CloseHandle(hFile);
}

// ---------------------------------------------------------------------------
// Apply a loaded update plan. Each file operation is independently fault-
// tolerant: a single failure is logged but does not abort the whole update.
// failureReason is populated on the first fatal error or left empty if all
// operations succeed (or only have recoverable, non-fatal failures).
// ---------------------------------------------------------------------------
static void ApplyUpdatePlan(const UpdaterArgs& args, const PendingUpdatePlan& plan)
{
    const bool isFullPackage = EqualsIgnoreCase(plan.packageType, L"full");
    const std::vector<std::wstring>& removeList = plan.removeList;
    const std::vector<std::wstring>& moveList = plan.moveList;

    SetProgressUiTotalFileCount(static_cast<int>(removeList.size() + moveList.size()));
    SetProgressUiStatus(
        L"正在分析更新内容... | Analyzing update contents...",
        L"更新计划读取完成 | Update plan loaded");

    WriteLogF(
        L"Plan loaded, package type: %s, remove entries: %zu, install entries: %zu",
        plan.packageType,
        removeList.size(),
        moveList.size());
    WriteLogF(L"Files to remove (%zu)", removeList.size());
    for (const std::wstring& entry : removeList) {
        WriteLog(L"  - " + entry);
    }
    WriteLogF(L"Files to install (%zu)", moveList.size());
    for (const std::wstring& entry : moveList) {
        WriteLog(L"  - " + entry);
    }

    CreateDirectoryW(args.backupDir.c_str(), nullptr);

    // ---- Remove entries ----
    for (const std::wstring& rel : removeList) {
        std::wstring targetPath;
        if (!TryResolvePathUnderRoot(args.rootDir, rel, targetPath)) {
            WriteLogF(L"Illegal path in removeList: %s", rel);
            AdvanceProgressUi(L"正在清理旧文件... | Cleaning old files...", rel);
            continue;
        }
        if (!PathExistsW(targetPath)) {
            AdvanceProgressUi(L"正在清理旧文件... | Cleaning old files...", rel);
            continue;
        }

        if (!isFullPackage && IsDirectory(targetPath)) {
            WriteLogF(L"Skipping directory removal for a non-full package, entry: %s, target: %s", rel, targetPath);
            AdvanceProgressUi(L"正在清理旧文件... | Cleaning old files...", rel);
            continue;
        }

        std::wstring backupPath;
        if (!TryResolvePathUnderRoot(args.backupDir, rel, backupPath)) {
            WriteLogF(L"Illegal backup path for removeList: %s", rel);
            AdvanceProgressUi(L"正在清理旧文件... | Cleaning old files...", rel);
            continue;
        }

        WriteLogF(L"Removing and backing up: %s -> %s", targetPath, backupPath);
        const bool ok = isFullPackage ? RecycleAndBackupPath(targetPath, backupPath)
                                      : MoveExistingPathToBackup(targetPath, backupPath);
        if (!ok) {
            WriteLogF(L"Failed to remove (will retry next time): %s", targetPath);
        }

        AdvanceProgressUi(L"正在清理旧文件... | Cleaning old files...", rel);
    }

    // ---- Move/install entries ----
    for (const std::wstring& rel : moveList) {
        std::wstring sourcePath;
        std::wstring targetPath;
        std::wstring backupPath;

        if (!TryResolvePathUnderRoot(args.extractDir, rel, sourcePath) ||
            !TryResolvePathUnderRoot(args.rootDir, rel, targetPath) ||
            !TryResolvePathUnderRoot(args.backupDir, rel, backupPath)) {
            WriteLogF(L"Illegal path in moveList: %s", rel);
            AdvanceProgressUi(L"正在安装新文件... | Installing new files...", rel);
            continue;
        }

        if (PathExistsW(targetPath)) {
            if (!isFullPackage && IsDirectory(targetPath)) {
                WriteLogF(L"Cannot replace directory with file in non-full package: %s", targetPath);
                AdvanceProgressUi(L"正在安装新文件... | Installing new files...", rel);
                continue;
            }

            WriteLogF(L"Backing up existing entry: %s", targetPath);
            const bool backupOk = IsRecycleAndReplaceDirectory(rel) ? RecycleAndBackupDirectory(targetPath, backupPath)
                                                                    : MoveExistingPathToBackup(targetPath, backupPath);
            if (!backupOk) {
                WriteLogF(L"Failed to back up existing entry (will retry next time): %s", targetPath);
                AdvanceProgressUi(L"正在安装新文件... | Installing new files...", rel);
                continue;
            }
        }

        WriteLogF(L"Installing new file: %s -> %s", sourcePath, targetPath);

        const DWORD sourceAttr = GetFileAttributesW(sourcePath.c_str());
        const bool isSourceFile = (sourceAttr != INVALID_FILE_ATTRIBUTES) && !(sourceAttr & FILE_ATTRIBUTE_DIRECTORY);

        bool installOk = false;
        if (isSourceFile) {
            installOk = InstallFileAtomic(sourcePath, targetPath);
        }
        else {
            auto moveOp = [&]() -> bool {
                return MoveFileExW(sourcePath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
            };
            installOk = RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);
        }

        if (!installOk) {
            WriteLogF(L"Failed to install file (will retry next time): %s", sourcePath);
        }
        else {
            SetFileUpdateTime(targetPath);
        }

        AdvanceProgressUi(L"正在安装新文件... | Installing new files...", rel);
    }
}

int wmain(int argc, wchar_t* argv[])
{
    // ------------------------------------------------------------------
    // Parse arguments (supports both legacy and -v2 formats)
    // ------------------------------------------------------------------
    UpdaterArgs args = ParseUpdaterArgs(argc, argv);
    if (!args.valid) {
        MessageBoxW(
            nullptr,
            (L"MAA.Updater.exe 是 MAA 内部使用的更新程序，不应被手动启动。\n\n"
             L"请直接运行 MAA.exe。\n\n"
             L"MAA.Updater.exe is an updater used internally by MAA and should not be manually started.\n\n"
             L"Please run MAA.exe directly.\n\n"
             L"错误信息 | Error:\n" +
             args.errorMessage)
                .c_str(),
            L"MAA 更新程序 | MAA Updater",
            MB_OK | MB_ICONINFORMATION);
        return EXIT_FAILURE;
    }

    g_writeConsoleLog = args.showConsole;

    // ------------------------------------------------------------------
    // Set up logging
    // ------------------------------------------------------------------
    g_logFile = args.rootDir + MAA_UPDATER_LOG_FILENAME;
    RotateLogIfNeeded();
    InitializeProgressUi();
    SetProgressUiStatus(
        L"正在准备更新... | Preparing update...",
        L"等待 MAA 主程序退出 | Waiting for the main MAA process to exit");

    WriteLog(L"MAA.Updater started (C++ external updater).");
    WriteLogF(L"Console output: %s", g_writeConsoleLog ? L"enabled" : L"disabled");
    WriteLogF(L"Argument format: %s", IsV2Format(argc, argv) ? L"v2 (--key value)" : L"legacy (positional)");
    WriteLogF(L"Parent PID: %lu, root dir: %s", args.parentPid, args.rootDir);
    WriteLogF(L"Plan file: %s, extract dir: %s", args.planFile, args.extractDir);

    bool success = false;
    std::wstring failureReason;
    HANDLE hUpdateMutex = nullptr;

    // ------------------------------------------------------------------
    // Wait for parent process to exit
    // ------------------------------------------------------------------
    HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, args.parentPid);
    if (hParent) {
        WriteLogF(L"Waiting for parent process to exit, PID=%lu", args.parentPid);
        while (WaitForSingleObject(hParent, 100) == WAIT_TIMEOUT) {
            PumpProgressUiMessages();
        }
        CloseHandle(hParent);
        WriteLog(L"Parent process exited.");
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"已确认主程序退出，开始读取更新计划 | Parent process exited, reading update plan");
    }
    else {
        WriteLogF(
            L"Could not open the parent process, it may have already exited, PID=%lu. Continuing.",
            args.parentPid);
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"主程序已退出，开始读取更新计划 | Main process already exited, reading update plan");
    }

    // ------------------------------------------------------------------
    // Acquire update mutex to prevent new MAA instances from starting
    // ------------------------------------------------------------------
    if (!args.mutexName.empty()) {
        hUpdateMutex = AcquireUpdateMutex(args.mutexName);
        if (hUpdateMutex == nullptr) {
            failureReason =
                L"检测到另一个 MAA 实例正在运行，无法执行更新。请关闭所有 MAA 窗口后重试。\n\n"
                L"Another MAA instance is running. Please close all MAA windows and try again.";
            WriteLog(failureReason);
            SetProgressUiStatus(
                L"无法继续更新 | Update blocked",
                L"检测到另一个 MAA 实例正在运行 | Another MAA instance is running");
        }
        else {
            WriteLog(L"Update mutex acquired, preventing new MAA instances from starting.");
        }

        // Clean up any .pendingdelete files left from a previous interrupted update
        CleanupPendingDeleteFiles(args.rootDir);
    }
    else {
        WriteLog(L"No mutex name provided, update mutex will not be used.");
    }

    if (IsDriveRootDirectory(args.rootDir)) {
        failureReason =
            L"检测到 MAA 安装在盘符根目录，已阻止更新继续执行。请先将 MAA 移动到独立文件夹后再重试。\n\n"
            L"Detected MAA installed directly in a drive root. Update execution was blocked. Please move MAA into a "
            L"dedicated folder and try again.";
        WriteLog(failureReason);
        SetProgressUiStatus(L"无法继续更新 | Update blocked", L"检测到盘符根目录安装 | Drive-root install detected");
    }

    // ------------------------------------------------------------------
    // Apply update plan
    // ------------------------------------------------------------------
    if (failureReason.empty()) {
        PendingUpdatePlan plan;
        if (LoadPendingUpdatePlan(args.planFile, plan, failureReason)) {
            ApplyUpdatePlan(args, plan);
        }
        else {
            WriteLog(failureReason);
        }
    }

    // ------------------------------------------------------------------
    // Determine outcome
    // ------------------------------------------------------------------
    if (failureReason.empty()) {
        success = true;
    }

    // ------------------------------------------------------------------
    // On success: cleanup and mark
    // ------------------------------------------------------------------
    if (success) {
        if (PathExistsW(args.packagePath)) {
            DeleteFileW(args.packagePath.c_str());
            WriteLogF(L"Deleted update package: %s", args.packagePath);
        }

        if (PathExistsW(args.failureStatusFile)) {
            DeleteFileW(args.failureStatusFile.c_str());
        }

        SetProgressUiStatus(L"正在完成更新... | Finalizing update...", L"正在写入更新结果 | Writing update result");
        if (WriteUtf8File(args.successStatusFile, "succeeded")) {
            WriteLogF(L"Wrote success status file: %s", args.successStatusFile);
        }
        else {
            WriteLogF(L"Failed to write success status file: %s", args.successStatusFile);
        }

        // Cleanup temporary files
        if (PathExistsW(args.extractDir)) {
            WriteLogF(L"Cleaning extract directory: %s", args.extractDir);
            ForceRemoveDirectoryRecursive(args.extractDir);
        }

        if (PathExistsW(args.planFile)) {
            DeleteFileW(args.planFile.c_str());
        }
    }

    // ------------------------------------------------------------------
    // On failure: write status but keep extractDir/planFile for retry
    // ------------------------------------------------------------------
    if (!success) {
        // Write failure reason for the main process to consume
        if (!failureReason.empty()) {
            std::string utf8Reason;
            if (TryConvertWideToUtf8(failureReason, utf8Reason)) {
                WriteUtf8File(args.failureStatusFile, utf8Reason);
            }
        }
        if (PathExistsW(args.successStatusFile)) {
            DeleteFileW(args.successStatusFile.c_str());
        }

        WriteLogF(L"Update failed: %s", failureReason);
        ShowProgressUiFailure(
            L"更新未完全成功。\n"
            L"请关闭所有 MAA 进程后重新运行 MAA.Updater.exe。\n\n"
            L"The update was not fully completed.\n"
            L"Please close all MAA processes and run MAA.Updater.exe again.");
    }

    // ------------------------------------------------------------------
    // Release update mutex (always required)
    // ------------------------------------------------------------------
    if (hUpdateMutex) {
        ReleaseUpdateMutex(hUpdateMutex);
        hUpdateMutex = nullptr;
        WriteLog(L"Update mutex released.");
    }

    // ------------------------------------------------------------------
    // Relaunch MAA (only on success)
    // ------------------------------------------------------------------
    if (success && PathExistsW(args.relaunchExecutable)) {
        CompleteProgressUi(L"更新完成 | Update completed", L"正在重新启动 MAA... | Relaunching MAA...");
        WriteLogF(L"Relaunching MAA: %s", args.relaunchExecutable);

        std::wstring workDir = args.relaunchExecutable;
        if (const size_t sep = workDir.rfind(L'\\'); sep != std::wstring::npos) {
            workDir = workDir.substr(0, sep);
        }

        STARTUPINFOW si {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi {};
        std::wstring cmdLine = L"\"" + args.relaunchExecutable + L"\"";
        if (CreateProcessW(
                args.relaunchExecutable.c_str(),
                cmdLine.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                workDir.c_str(),
                &si,
                &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            WriteLog(L"Relaunch succeeded.");
        }
        else {
            WriteLogF(L"Relaunch failed, error=%lu", GetLastError());
            ShowProgressUiFailure(
                L"更新已完成，但重新启动 MAA 失败，请手动启动 MAA。\n"
                L"Update finished, but failed to relaunch MAA. Please start MAA manually.");
        }
    }

    WriteLog(L"MAA.Updater exiting.");
    DestroyProgressUi();
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
