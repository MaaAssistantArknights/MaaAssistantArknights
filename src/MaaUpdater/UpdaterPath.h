#pragma once

#include <windows.h>

#include <string>

// ---------------------------------------------------------------------------
// Path utilities
// ---------------------------------------------------------------------------

std::wstring EnsureTrailingSeparator(const std::wstring& path);
std::wstring NormalizeRelativePath(const std::wstring& relativePath);
bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right);
bool IsDriveRootDirectory(const std::wstring& path);
bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath);
bool TryResolvePathUnderRoot(const std::wstring& rootPath, const std::wstring& relativePath, std::wstring& out);
