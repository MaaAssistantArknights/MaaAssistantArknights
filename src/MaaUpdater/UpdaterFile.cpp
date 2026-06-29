#include "UpdaterFile.h"
#include "UpdaterLog.h"

#include <ctime>

// ---------------------------------------------------------------------------
// Basic file/directory helpers
// ---------------------------------------------------------------------------

bool PathExistsW(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool IsDirectory(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir)
{
    DWORD attr = GetFileAttributesW(sourceDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    CreateDirectoryW(destinationDir.c_str(), nullptr);

    std::wstring pattern = sourceDir + L"\\*";
    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return true;
    }

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring sourceChild = sourceDir + L"\\" + fd.cFileName;
        std::wstring destinationChild = destinationDir + L"\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CopyDirectoryRecursive(sourceChild, destinationChild)) {
                FindClose(hFind);
                return false;
            }
            continue;
        }

        EnsureParentDirectory(destinationChild);
        if (!CopyFileW(sourceChild.c_str(), destinationChild.c_str(), FALSE)) {
            FindClose(hFind);
            return false;
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return true;
}

bool MovePathToRecycleBin(const std::wstring& path)
{
    std::wstring doubleNullPath = path;
    doubleNullPath.push_back(L'\0');

    SHFILEOPSTRUCTW op {};
    op.wFunc = FO_DELETE;
    op.pFrom = doubleNullPath.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&op);
    return result == 0 && !op.fAnyOperationsAborted;
}

bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath)
{
    DWORD attr = GetFileAttributesW(sourcePath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return CopyDirectoryRecursive(sourcePath, destinationPath);
    }

    EnsureParentDirectory(destinationPath);
    return CopyFileW(sourcePath.c_str(), destinationPath.c_str(), FALSE) != FALSE;
}

void EnsureParentDirectory(const std::wstring& path)
{
    size_t sep = path.rfind(L'\\');
    if (sep == std::wstring::npos) {
        return;
    }
    std::wstring parent = path.substr(0, sep);
    if (parent.empty()) {
        return;
    }

    // Recursively create
    EnsureParentDirectory(parent);
    CreateDirectoryW(parent.c_str(), nullptr);
}

std::wstring CreateArchivedPath(const std::wstring& base)
{
    SYSTEMTIME st {};
    GetLocalTime(&st);
    wchar_t ts[32];
    _snwprintf_s(
        ts,
        _countof(ts),
        _TRUNCATE,
        L".%04d%02d%02d%02d%02d%02d.",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond);

    int index = 0;
    while (true) {
        wchar_t idx[16];
        _itow_s(index, idx, _countof(idx), 10);
        std::wstring candidate = base + ts + idx;
        if (!PathExistsW(candidate)) {
            return candidate;
        }
        index++;
    }
}

// ---------------------------------------------------------------------------
// Move / backup helpers
// ---------------------------------------------------------------------------

// Move a file or directory entry (handles cross-volume by CopyFile+Delete for files).
// Uses retry and rename fallback for locked files.
bool MovePathEntry(const std::wstring& src, const std::wstring& dst)
{
    EnsureParentDirectory(dst);

    DWORD attr = GetFileAttributesW(src.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        auto moveOp = [&]() -> bool {
            return MoveFileExW(src.c_str(), dst.c_str(), 0) != FALSE;
        };
        return RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);
    }

    // For files: try atomic move first; fall back to copy+delete
    {
        auto moveOp = [&]() -> bool {
            return MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
        };

        if (RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
            return true;
        }
    }

    // Move failed (likely the source file is locked). Try copy+force-delete.
    WriteLogF(L"MovePathEntry: move failed, falling back to copy+delete for: %s", src);
    {
        auto copyOp = [&]() -> bool {
            return CopyFileW(src.c_str(), dst.c_str(), FALSE) != FALSE;
        };

        if (RetryFileOp(copyOp, 3, 500)) {
            // Copy succeeded; force-delete the source
            ForceDeleteFile(src);
            return true;
        }
    }

    // Copy also failed. As a last resort, try to rename the source out of the way
    // and then do a fresh copy to the destination.
    WriteLogF(L"MovePathEntry: copy+delete failed, trying rename-and-copy for: %s", src);
    {
        std::wstring renamed = RenameLockedFile(src);
        if (!renamed.empty()) {
            // The original path is now free; copy the renamed file to destination
            auto copyOp = [&]() -> bool {
                return CopyFileW(renamed.c_str(), dst.c_str(), FALSE) != FALSE;
            };

            if (RetryFileOp(copyOp, 3, 500)) {
                WriteLogF(L"MovePathEntry: rename-and-copy succeeded: %s", src);
                return true;
            }
        }
    }

    WriteLogF(L"MovePathEntry: all strategies failed for src=%s, dst=%s", src, dst);
    return false;
}

void PrepareBackupDestination(const std::wstring& backupPath)
{
    EnsureParentDirectory(backupPath);
    if (!PathExistsW(backupPath)) {
        return;
    }

    std::wstring archived = CreateArchivedPath(backupPath);
    MovePathEntry(backupPath, archived);
}

bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup)
{
    PrepareBackupDestination(backup);

    // First try: normal move with retry
    if (MovePathEntry(src, backup)) {
        return true;
    }

    // Second try: copy to backup, then force-delete the source
    WriteLogF(L"MoveExistingPathToBackup: move failed, trying copy+delete for: %s", src);
    {
        auto copyOp = [&]() -> bool {
            return CopyPathEntry(src, backup) != FALSE;
        };

        if (RetryFileOp(copyOp, 3, 500)) {
            // Copy succeeded; now remove the source
            ForceDeleteFile(src);
            return true;
        }
    }

    WriteLogF(L"MoveExistingPathToBackup: all strategies failed for: %s", src);
    return false;
}

bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    {
        auto copyOp = [&]() -> bool {
            return CopyDirectoryRecursive(sourcePath, backupPath) != FALSE;
        };
        if (!RetryFileOp(copyOp, 3, 500)) {
            return false;
        }
    }

    // Try recycle bin first, fall back to force-delete
    if (MovePathToRecycleBin(sourcePath)) {
        return true;
    }

    return ForceRemoveDirectoryRecursive(sourcePath);
}

bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    {
        auto copyOp = [&]() -> bool {
            return CopyPathEntry(sourcePath, backupPath) != FALSE;
        };
        if (!RetryFileOp(copyOp, 3, 500)) {
            return false;
        }
    }

    // Try recycle bin first, fall back to force-delete
    if (MovePathToRecycleBin(sourcePath)) {
        return true;
    }

    return ForceDeleteFile(sourcePath);
}

// ---------------------------------------------------------------------------
// Write a small UTF-8 text file
// ---------------------------------------------------------------------------

bool WriteUtf8File(const std::wstring& path, const char* content)
{
    return WriteUtf8File(path, std::string(content));
}

bool WriteUtf8File(const std::wstring& path, const std::string& content)
{
    EnsureParentDirectory(path);
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    DWORD len = static_cast<DWORD>(content.size());
    bool ok = WriteFile(hFile, content.data(), len, &written, nullptr) && written == len;
    CloseHandle(hFile);
    return ok;
}

// ---------------------------------------------------------------------------
// Retry and force-delete helpers
// ---------------------------------------------------------------------------

// Retries a file operation with exponential backoff on lock-related errors.
// Returns true if the operation eventually succeeded.
// Does NOT retry on non-lock errors (ERROR_FILE_NOT_FOUND, etc.).
bool RetryFileOp(const std::function<bool()>& op, int maxAttempts, DWORD initialDelayMs)
{
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (op()) {
            return true;
        }

        DWORD error = GetLastError();
        if (error != ERROR_SHARING_VIOLATION && error != ERROR_LOCK_VIOLATION && error != ERROR_ACCESS_DENIED) {
            // Not a transient locking error — no point retrying.
            return false;
        }

        if (attempt < maxAttempts) {
            DWORD delay = initialDelayMs * static_cast<DWORD>(1 << (attempt - 1));
            WriteLogF(
                L"RetryFileOp: attempt %d/%d failed (error=%u), retrying in %u ms",
                attempt,
                maxAttempts,
                error,
                delay);
            Sleep(delay);
        }
    }

    return false;
}

// Renames a locked file to a unique name with .pendingdelete suffix.
// This works on NTFS even when the file is open with FILE_SHARE_DELETE.
// Returns the new path on success, or an empty string on failure.
std::wstring RenameLockedFile(const std::wstring& path)
{
    // Generate a unique suffix using tick count + a counter
    static volatile LONG s_renameCounter = 0;
    DWORD tick = GetTickCount();
    LONG counter = InterlockedIncrement(&s_renameCounter);
    wchar_t suffix[64];
    _snwprintf_s(
        suffix,
        _countof(suffix),
        _TRUNCATE,
        L".%08x%04x" PENDING_DELETE_SUFFIX,
        tick,
        static_cast<WORD>(counter & 0xFFFF));

    std::wstring newPath = path + suffix;

    // Try the rename; if it fails (e.g. path already exists), append more entropy
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (MoveFileExW(path.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE) {
            WriteLogF(L"Renamed locked file: %s -> %s", path, newPath);
            return newPath;
        }

        // With MOVEFILE_REPLACE_EXISTING the only likely reason is sharing-violation
        // on the source. Give it a brief retry.
        Sleep(100);
        std::wstring retryPath = newPath + L"." + std::to_wstring(attempt);
        newPath = retryPath;
    }

    WriteLogF(L"Failed to rename locked file after 10 attempts: %s", path);
    return {};
}

// Attempts to delete a file through multiple escalation layers:
//   1. Normal DeleteFileW + retry
//   2. Rename the file out of the way, then delete
//   3. Schedule for deletion on next reboot (MOVEFILE_DELAY_UNTIL_REBOOT)
// Returns true if the file is no longer present at the original path.
bool ForceDeleteFile(const std::wstring& path)
{
    // --- Layer 1: Normal delete with retry ---
    auto deleteOp = [&]() -> bool {
        return DeleteFileW(path.c_str()) != FALSE;
    };

    if (RetryFileOp(deleteOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
        WriteLogF(L"Deleted file: %s", path);
        return true;
    }

    if (!PathExistsW(path)) {
        return true; // already gone
    }

    // --- Layer 2: Rename then delete ---
    std::wstring renamed = RenameLockedFile(path);
    if (!renamed.empty()) {
        // The rename succeeded; now try to delete the renamed file
        RetryFileOp([&]() -> bool { return DeleteFileW(renamed.c_str()) != FALSE; }, 3, 500);
        if (PathExistsW(renamed)) {
            WriteLogF(L"Renamed file could not be deleted immediately (will remain as .pendingdelete): %s", renamed);
        }
        // Even if we couldn't delete the renamed file, the original path is now free.
        WriteLogF(L"File vacated via rename: %s -> %s", path, renamed);
        return true;
    }

    // --- Layer 3: Schedule for deletion on next reboot ---
    if (MoveFileExW(path.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT) != FALSE) {
        WriteLogF(L"Scheduled file for deletion on next reboot: %s", path);
        return true;
    }

    WriteLogF(L"All deletion strategies failed for: %s (error=%lu)", path, GetLastError());
    return false;
}

// Recursively removes a directory, using ForceDeleteFile for each file.
bool ForceRemoveDirectoryRecursive(const std::wstring& dir)
{
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
                continue;
            }
            std::wstring child = dir + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ForceRemoveDirectoryRecursive(child);
            }
            else {
                ForceDeleteFile(child);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    return RemoveDirectoryW(dir.c_str()) != FALSE || !PathExistsW(dir);
}

// Atomically installs a file, sets last-write time to now.
bool InstallFileAtomic(const std::wstring& sourcePath, const std::wstring& targetPath)
{
    EnsureParentDirectory(targetPath);

    if (!PathExistsW(sourcePath)) {
        WriteLogF(L"InstallFileAtomic: source not found: %s", sourcePath);
        return false;
    }

    // Generate a temporary path alongside the target for ReplaceFileW
    const std::wstring tempPath = targetPath + L"." + std::to_wstring(GetTickCount()) + L".tmpinstall";

    auto copyOp = [&]() -> bool {
        return CopyFileW(sourcePath.c_str(), tempPath.c_str(), FALSE) != FALSE;
    };
    if (!RetryFileOp(copyOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS)) {
        WriteLogF(L"InstallFileAtomic: failed to copy to temp: %s (error=%lu)", tempPath, GetLastError());
        DeleteFileW(tempPath.c_str());
        return false;
    }

    // Try ReplaceFileW (atomic swap, handles locked targets). If it fails,
    // rename any locked target out of the way then try a direct move.
    auto replaceOp = [&]() -> bool {
        return ReplaceFileW(targetPath.c_str(), tempPath.c_str(), nullptr,
                            REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_WRITE_THROUGH,
                            nullptr, nullptr) != FALSE;
    };
    auto moveOp = [&]() -> bool {
        return MoveFileExW(tempPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
    };

    bool ok = RetryFileOp(replaceOp, 3, 500);
    if (!ok && PathExistsW(targetPath)) {
        std::wstring renamed = RenameLockedFile(targetPath);
        if (!renamed.empty()) ok = moveOp();
    }
    if (!ok) ok = RetryFileOp(moveOp, FILE_OP_MAX_RETRIES, FILE_OP_INITIAL_DELAY_MS);

    DeleteFileW(tempPath.c_str());
    if (!ok) {
        WriteLogF(L"InstallFileAtomic: all strategies failed for: %s", targetPath);
        return false;
    }

    WriteLogF(L"InstallFileAtomic: installed: %s", targetPath);

    // Set last-write time to now
    HANDLE hFile = CreateFileW(targetPath.c_str(), FILE_WRITE_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        SetFileTime(hFile, nullptr, nullptr, &ft);
        CloseHandle(hFile);
    }
    return true;
}

// Scans rootDir and deletes any remaining .pendingdelete files left
// from a previous interrupted update.
void CleanupPendingDeleteFiles(const std::wstring& rootDir)
{
    std::wstring pattern = rootDir + L"\\*" + PENDING_DELETE_SUFFIX;
    int cleanedCount = 0;

    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        std::wstring filePath = rootDir + L"\\" + fd.cFileName;
        auto deleteOp = [&]() -> bool {
            return DeleteFileW(filePath.c_str()) != FALSE;
        };

        if (RetryFileOp(deleteOp, 3, 500)) {
            ++cleanedCount;
        }
        else {
            WriteLogF(L"Could not clean up pending-delete file: %s", filePath);
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    if (cleanedCount > 0) {
        WriteLogF(L"Cleaned up %d pending-delete file(s).", cleanedCount);
    }
}
