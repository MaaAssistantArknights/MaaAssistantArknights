// MAA.Updater.exe
// Applies a pending MAA update package after the main MAA process exits.
// Invoked by MaaWpfGui's PendingUpdateApplier; do NOT run manually.
//
// Usage:
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

#include "UpdaterFile.h"
#include "UpdaterLog.h"
#include "UpdaterMutex.h"
#include "UpdaterPath.h"
#include "UpdaterPlan.h"
#include "UpdaterUI.h"

int wmain(int argc, wchar_t* argv[])
{
    constexpr int REQUIRED_ARGS = 9; // excluding argv[0]

    if (argc - 1 < REQUIRED_ARGS) {
        MessageBoxW(
            nullptr,
            L"MAA.Updater.exe 是 MAA 内部使用的更新程序，不应被手动启动。\n\n"
            L"请直接运行 MAA.exe。\n\n"
            L"MAA.Updater.exe is an updater used internally by MAA and should not be manually started.\n\n"
            L"Please run MAA.exe directly.",
            L"MAA 更新程序 | MAA Updater",
            MB_OK | MB_ICONINFORMATION);
        return 1;
    }

    DWORD     parentPid              = static_cast<DWORD>(_wtoi(argv[1]));
    std::wstring rootDir             = argv[2];
    std::wstring extractDir          = argv[3];
    std::wstring backupDir           = argv[4];
    std::wstring packagePath         = argv[5];
    std::wstring successStatusFile   = argv[6];
    std::wstring failureStatusFile   = argv[7];
    std::wstring relaunchExecutable  = argv[8];
    std::wstring planFile            = argv[9];
    g_writeConsoleLog = HasArgument(argc, argv, L"--show-console");

    std::wstring mutexName;
    for (int i = 1; i < argc - 1; ++i) {
        if (_wcsicmp(argv[i], MUTEX_NAME_ARG) == 0) {
            mutexName = argv[i + 1];
            break;
        }
    }

    g_logFile = rootDir + L"\\debug\\pending-update-applier.log";
    RotateLogIfNeeded();
    InitializeProgressUi();
    SetProgressUiStatus(
        L"正在准备更新... | Preparing update...",
        L"等待 MAA 主程序退出 | Waiting for the main MAA process to exit");

    WriteLog(L"MAA.Updater started (C++ external updater).");
    WriteLog((std::wstring(L"Console output: ") + (g_writeConsoleLog ? L"enabled" : L"disabled")).c_str());
    WriteLog((L"Parent PID: " + std::to_wstring(parentPid) + L", root dir: " + rootDir).c_str());
    WriteLog((L"Plan file: " + planFile + L", extract dir: " + extractDir).c_str());

    bool shouldRelaunch = false;
    bool success = false;
    std::wstring failureReason;
    HANDLE hUpdateMutex = nullptr;

    // ------------------------------------------------------------------
    // Wait for parent process to exit
    // ------------------------------------------------------------------
    HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (hParent != nullptr) {
        WriteLog((L"Waiting for parent process to exit, PID=" + std::to_wstring(parentPid)).c_str());
        while (WaitForSingleObject(hParent, 100) == WAIT_TIMEOUT) {
            PumpProgressUiMessages();
        }
        CloseHandle(hParent);
        WriteLog(L"Parent process exited.");
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"已确认主程序退出，开始读取更新计划 | Parent process exited, reading update plan");
    } else {
        WriteLog((L"Could not open the parent process, it may have already exited, PID=" + std::to_wstring(parentPid) + L". Continuing.").c_str());
        SetProgressUiStatus(
            L"正在准备更新... | Preparing update...",
            L"主程序已退出，开始读取更新计划 | Main process already exited, reading update plan");
    }

    // ------------------------------------------------------------------
    // Acquire update mutex to prevent new MAA instances from starting
    // ------------------------------------------------------------------
    if (!mutexName.empty()) {
        hUpdateMutex = AcquireUpdateMutex(mutexName);
        if (hUpdateMutex == nullptr) {
            failureReason =
                L"检测到另一个 MAA 实例正在运行，无法执行更新。请关闭所有 MAA 窗口后重试。\n\n"
                L"Another MAA instance is running. Please close all MAA windows and try again.";
            WriteLog(failureReason.c_str());
            SetProgressUiStatus(
                L"无法继续更新 | Update blocked",
                L"检测到另一个 MAA 实例正在运行 | Another MAA instance is running");
        } else {
            WriteLog(L"Update mutex acquired, preventing new MAA instances from starting.");
        }

        // Clean up any .pendingdelete files left from a previous interrupted update
        CleanupPendingDeleteFiles(rootDir);
    } else {
        WriteLog(L"No mutex name provided, update mutex will not be used.");
    }

    if (IsDriveRootDirectory(rootDir)) {
        failureReason =
            L"检测到 MAA 安装在盘符根目录，已阻止更新继续执行。请先将 MAA 移动到独立文件夹后再重试。\n\n"
            L"Detected MAA installed directly in a drive root. Update execution was blocked. Please move MAA into a dedicated folder and try again.";
        WriteLog(failureReason.c_str());
        SetProgressUiStatus(
            L"无法继续更新 | Update blocked",
            L"检测到盘符根目录安装 | Drive-root install detected");
    }

    // ------------------------------------------------------------------
    // Read plan
    // ------------------------------------------------------------------
    do {
        if (!failureReason.empty()) {
            break;
        }

        PendingUpdatePlan plan;
        if (!LoadPendingUpdatePlan(planFile, plan, failureReason)) {
            WriteLog(failureReason.c_str());
            break;
        }

        bool isFullPackage = EqualsIgnoreCase(plan.packageType, L"full");
        const std::vector<std::wstring>& removeList = plan.removeList;
        const std::vector<std::wstring>& moveList = plan.moveList;

        SetProgressUiTotalFileCount(static_cast<int>(removeList.size() + moveList.size()));
        SetProgressUiStatus(
            L"正在分析更新内容... | Analyzing update contents...",
            L"更新计划读取完成 | Update plan loaded");

        WriteLog((L"Plan loaded, package type: " + plan.packageType + L", remove entries: " + std::to_wstring(removeList.size()) + L", install entries: " + std::to_wstring(moveList.size())).c_str());
        WriteLogEntries(L"Files to remove", removeList);
        WriteLogEntries(L"Files to install", moveList);

        CreateDirectoryW(backupDir.c_str(), nullptr);

        // ---- Remove entries ----
        for (const std::wstring& rel : removeList) {
            std::wstring targetPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath)) {
                failureReason = L"Illegal path in removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }
            if (!PathExistsW(targetPath)) {
                AdvanceProgressUi(
                    L"正在清理旧文件... | Cleaning old files...",
                    rel);
                continue;
            }

            if (!isFullPackage && IsDirectory(targetPath)) {
                WriteLog((L"Skipping directory removal for a non-full package, entry: " + rel + L", target: " + targetPath).c_str());
                AdvanceProgressUi(
                    L"正在清理旧文件... | Cleaning old files...",
                    rel);
                continue;
            }

            std::wstring backupPath;
            if (!TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                failureReason = L"Illegal backup path for removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            WriteLog((L"Removing and backing up: " + targetPath + L" -> " + backupPath).c_str());
            bool backupOk = isFullPackage
                ? RecycleAndBackupPath(targetPath, backupPath)
                : MoveExistingPathToBackup(targetPath, backupPath);
            if (!backupOk) {
                failureReason = L"Failed to move to backup: " + targetPath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            AdvanceProgressUi(
                L"正在清理旧文件... | Cleaning old files...",
                rel);
        }

        // ---- Move/install entries ----
        for (const std::wstring& rel : moveList) {
            std::wstring sourcePath, targetPath, backupPath;

            if (!TryResolvePathUnderRoot(extractDir, rel, sourcePath) ||
                !TryResolvePathUnderRoot(rootDir,    rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir,  rel, backupPath))
            {
                failureReason = L"Illegal path in moveList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            if (PathExistsW(targetPath)) {
                if (!isFullPackage && IsDirectory(targetPath)) {
                    failureReason = L"Non-full package cannot replace directory: " + targetPath;
                    WriteLog(failureReason.c_str());
                    goto apply_failed;
                }

                WriteLog((L"Backing up existing entry: " + targetPath).c_str());
                bool backupOk = IsRecycleAndReplaceDirectory(rel)
                    ? RecycleAndBackupDirectory(targetPath, backupPath)
                    : MoveExistingPathToBackup(targetPath, backupPath);
                if (!backupOk) {
                    failureReason = L"Failed to back up existing entry: " + targetPath;
                    WriteLog(failureReason.c_str());
                    goto apply_failed;
                }
            }

            WriteLog((L"Installing new file: " + sourcePath + L" -> " + targetPath).c_str());

            bool installOk = false;
            DWORD sourceAttr = GetFileAttributesW(sourcePath.c_str());
            bool isSourceFile = (sourceAttr != INVALID_FILE_ATTRIBUTES) &&
                                !(sourceAttr & FILE_ATTRIBUTE_DIRECTORY);

            if (isSourceFile) {
                // Use atomic file replacement for individual files
                installOk = InstallFileAtomic(sourcePath, targetPath);
            } else {
                // Directories: use the original move logic
                auto moveOp = [&]() -> bool {
                    return MoveFileExW(sourcePath.c_str(), targetPath.c_str(),
                                       MOVEFILE_REPLACE_EXISTING) != FALSE;
                };
                installOk = RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);
            }

            if (!installOk) {
                failureReason = L"Failed to move file into place: " + sourcePath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            AdvanceProgressUi(
                L"正在安装新文件... | Installing new files...",
                rel);
        }

        // ---- Cleanup package ----
        if (PathExistsW(packagePath)) {
            DeleteFileW(packagePath.c_str());
            WriteLog((L"Deleted update package: " + packagePath).c_str());
        }

        if (PathExistsW(failureStatusFile))
            DeleteFileW(failureStatusFile.c_str());

        SetProgressUiStatus(
            L"正在完成更新... | Finalizing update...",
            L"正在写入更新结果 | Writing update result");
        if (WriteUtf8File(successStatusFile, "succeeded")) {
            WriteLog((L"Wrote success status file: " + successStatusFile).c_str());
            success = true;
            shouldRelaunch = true;
        } else {
            failureReason = L"Failed to write success status file: " + successStatusFile;
            WriteLog(failureReason.c_str());
        }

        break; // normal exit from do-while

    apply_failed:
        success = false;

        // Attempt rollback: restore files that were already backed up
        WriteLog(L"Update failed, attempting rollback from backup directory.");
        for (const std::wstring& rel : removeList) {
            std::wstring targetPath, backupPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                continue;
            }
            if (PathExistsW(backupPath) && !PathExistsW(targetPath)) {
                WriteLog((L"Rollback: restoring " + backupPath + L" -> " + targetPath).c_str());
                MovePathEntry(backupPath, targetPath);
            }
        }
        for (const std::wstring& rel : moveList) {
            std::wstring targetPath, backupPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath) ||
                !TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                continue;
            }
            if (PathExistsW(backupPath) && !PathExistsW(targetPath)) {
                WriteLog((L"Rollback: restoring " + backupPath + L" -> " + targetPath).c_str());
                MovePathEntry(backupPath, targetPath);
            }
        }
    } while (false);

    // ------------------------------------------------------------------
    // On failure: write failure status
    // ------------------------------------------------------------------
    if (!success && !failureReason.empty()) {
        // Convert wstring reason to UTF-8 for file
        std::string utf8Reason;
        if (TryConvertWideToUtf8(failureReason, utf8Reason)) {
            WriteUtf8File(failureStatusFile, utf8Reason);
        }
        if (PathExistsW(successStatusFile))
            DeleteFileW(successStatusFile.c_str());
        WriteLog((L"Update failed: " + failureReason).c_str());
        ShowProgressUiFailure(failureReason);
    }

    // ------------------------------------------------------------------
    // Cleanup extract dir and plan file
    // ------------------------------------------------------------------
    if (PathExistsW(extractDir)) {
        WriteLog((L"Cleaning extract directory: " + extractDir).c_str());
        ForceRemoveDirectoryRecursive(extractDir);
    }

    if (PathExistsW(planFile))
        DeleteFileW(planFile.c_str());

    // ------------------------------------------------------------------
    // Release update mutex
    // ------------------------------------------------------------------
    if (hUpdateMutex != nullptr) {
        ReleaseUpdateMutex(hUpdateMutex);
        hUpdateMutex = nullptr;
        WriteLog(L"Update mutex released.");
    }

    // ------------------------------------------------------------------
    // Relaunch MAA
    // ------------------------------------------------------------------
    if (shouldRelaunch && PathExistsW(relaunchExecutable)) {
        CompleteProgressUi(
            L"更新完成 | Update completed",
            L"正在重新启动 MAA... | Relaunching MAA...");
        WriteLog((L"Relaunching MAA: " + relaunchExecutable).c_str());

        // Find working directory (parent of the executable)
        std::wstring workDir = relaunchExecutable;
        size_t sep = workDir.rfind(L'\\');
        if (sep != std::wstring::npos) workDir = workDir.substr(0, sep);

        STARTUPINFOW si {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi {};
        std::wstring cmdLine = L"\"" + relaunchExecutable + L"\"";
        if (CreateProcessW(
                relaunchExecutable.c_str(),
                cmdLine.data(),
                nullptr, nullptr, FALSE, 0, nullptr,
                workDir.c_str(),
                &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            WriteLog(L"Relaunch succeeded.");
        } else {
            WriteLog((L"Relaunch failed, error=" + std::to_wstring(GetLastError())).c_str());
            ShowProgressUiFailure(
                L"更新已完成，但重新启动 MAA 失败，请手动启动 MAA。\n"
                L"Update finished, but failed to relaunch MAA. Please start MAA manually.");
        }
    } else if (success) {
        CompleteProgressUi(
            L"更新完成 | Update completed",
            L"更新已完成，请手动启动 MAA。 | Update completed. Please start MAA manually.");
    }

    WriteLog(L"MAA.Updater exiting.");
    DestroyProgressUi();
    return success ? 0 : 2;
}
