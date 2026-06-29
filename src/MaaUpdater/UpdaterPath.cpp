#include "UpdaterPath.h"

std::wstring EnsureTrailingSeparator(const std::wstring& path)
{
    if (!path.empty() && path.back() != L'\\') {
        return path + L'\\';
    }
    return path;
}

std::wstring NormalizeRelativePath(const std::wstring& relativePath)
{
    std::wstring normalized = relativePath;
    for (wchar_t& c : normalized) {
        if (c == L'/') {
            c = L'\\';
        }
    }

    while (!normalized.empty() &&
           (normalized.back() == L'\\' || normalized.back() == L' ' || normalized.back() == L'\t')) {
        normalized.pop_back();
    }

    return normalized;
}

bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right)
{
    return _wcsicmp(left.c_str(), right) == 0;
}

bool IsDriveRootDirectory(const std::wstring& path)
{
    if (path.empty()) {
        return false;
    }

    wchar_t full[MAX_PATH * 4];
    DWORD len = GetFullPathNameW(path.c_str(), _countof(full), full, nullptr);
    if (len == 0 || len >= _countof(full)) {
        return false;
    }

    std::wstring normalized = full;
    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }

    if (normalized.size() < 2) {
        return false;
    }

    wchar_t drive = normalized[0];
    bool hasDriveLetter = (drive >= L'A' && drive <= L'Z') || (drive >= L'a' && drive <= L'z');
    return hasDriveLetter && normalized.size() == 2 && normalized[1] == L':';
}

bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath)
{
    std::wstring normalized = NormalizeRelativePath(relativePath);
    return EqualsIgnoreCase(normalized, L"resource") || EqualsIgnoreCase(normalized, L"externals");
}

// Resolves `relativePath` under `rootPath`, writes result to `out`.
// Returns false if the path is rooted, empty, or escapes the root.
bool TryResolvePathUnderRoot(const std::wstring& rootPath, const std::wstring& relativePath, std::wstring& out)
{
    out.clear();

    // Reject empty or whitespace
    if (relativePath.empty()) {
        return false;
    }
    bool allSpace = true;
    for (wchar_t c : relativePath) {
        if (c != L' ' && c != L'\t') {
            allSpace = false;
            break;
        }
    }
    if (allSpace) {
        return false;
    }

    // Normalise slashes
    std::wstring rel = relativePath;
    for (wchar_t& c : rel) {
        if (c == L'/') {
            c = L'\\';
        }
    }

    // Reject absolute paths
    if (rel.size() >= 2 && rel[1] == L':') {
        return false;
    }
    if (!rel.empty() && rel[0] == L'\\') {
        return false;
    }

    // Build candidate and canonicalise
    std::wstring combined = rootPath + L'\\' + rel;
    wchar_t full[MAX_PATH * 4];
    DWORD len = GetFullPathNameW(combined.c_str(), _countof(full), full, nullptr);
    if (len == 0 || len >= _countof(full)) {
        return false;
    }

    std::wstring candidate = full;

    // Root must be canonical too
    DWORD rootLen = GetFullPathNameW(rootPath.c_str(), _countof(full), full, nullptr);
    if (rootLen == 0 || rootLen >= _countof(full)) {
        return false;
    }
    std::wstring normalRoot = EnsureTrailingSeparator(std::wstring(full));

    // Case-insensitive prefix check
    if (candidate.size() < normalRoot.size()) {
        return false;
    }
    if (_wcsnicmp(candidate.c_str(), normalRoot.c_str(), normalRoot.size()) != 0) {
        return false;
    }

    out = candidate;
    return true;
}
