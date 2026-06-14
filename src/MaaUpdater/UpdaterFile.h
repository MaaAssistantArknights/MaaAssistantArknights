#pragma once

#include <windows.h>

#include <shellapi.h>

#include <functional>
#include <string>

// ---------------------------------------------------------------------------
// File operation constants
// ---------------------------------------------------------------------------

#define PENDING_DELETE_SUFFIX L".pendingdelete"
constexpr int FILE_OP_MAX_RETRIES = 5;
constexpr DWORD FILE_OP_INITIAL_DELAY_MS = 200;

// ---------------------------------------------------------------------------
// Basic file/directory helpers
// ---------------------------------------------------------------------------

bool PathExistsW(const std::wstring& path);
bool IsDirectory(const std::wstring& path);
bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir);
bool MovePathToRecycleBin(const std::wstring& path);
bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath);
void EnsureParentDirectory(const std::wstring& path);
std::wstring CreateArchivedPath(const std::wstring& base);

// ---------------------------------------------------------------------------
// Move / backup helpers
// ---------------------------------------------------------------------------

bool MovePathEntry(const std::wstring& src, const std::wstring& dst);
void PrepareBackupDestination(const std::wstring& backupPath);
bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup);
bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath);
bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath);

// ---------------------------------------------------------------------------
// UTF-8 file I/O
// ---------------------------------------------------------------------------

bool WriteUtf8File(const std::wstring& path, const char* content);
bool WriteUtf8File(const std::wstring& path, const std::string& content);

// ---------------------------------------------------------------------------
// Retry and force-delete helpers
// ---------------------------------------------------------------------------

bool RetryFileOp(const std::function<bool()>& op, int maxAttempts = 5, DWORD initialDelayMs = 200);
std::wstring RenameLockedFile(const std::wstring& path);
bool ForceDeleteFile(const std::wstring& path);
bool ForceRemoveDirectoryRecursive(const std::wstring& dir);
bool InstallFileAtomic(const std::wstring& sourcePath, const std::wstring& targetPath);
void CleanupPendingDeleteFiles(const std::wstring& rootDir);
