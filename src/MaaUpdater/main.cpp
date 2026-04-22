// MAA.Updater.exe
// Applies a pending MAA update package after the main MAA process exits.
// Invoked by MaaWpfGui's PendingUpdateApplier; do NOT run manually.
//
// Usage:
//   MAA.Updater.exe <ParentProcessId> <RootDir> <ExtractDir> <BackupDir>
//                   <PackagePath> <SuccessStatusFile> <FailureStatusFile>
//                   <RelaunchExecutablePath> <PlanFile>
//
// Plan file format (UTF-8 JSON):
//   { "packageType": "full|ota", "removeList": ["rel/path", ...], "moveList": ["rel/path", ...] }

#include <windows.h>
#include <shellapi.h>

#include <cstdarg>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

struct PendingUpdatePlan;

static bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8);
static void WriteLog(const wchar_t* message);
static void WriteLogF(const wchar_t* fmt, ...);
static void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline);
static void RotateLogIfNeeded();

static std::wstring EnsureTrailingSeparator(const std::wstring& path);
static std::wstring NormalizeRelativePath(const std::wstring& relativePath);
static bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right);
static bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath);
static bool TryResolvePathUnderRoot(
    const std::wstring& rootPath,
    const std::wstring& relativePath,
    std::wstring& out);

static bool PathExistsW(const std::wstring& path);
static bool IsDirectory(const std::wstring& path);
static bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir);
static bool MovePathToRecycleBin(const std::wstring& path);
static bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath);
static void EnsureParentDirectory(const std::wstring& path);
static std::wstring CreateArchivedPath(const std::wstring& base);
static bool MovePathEntry(const std::wstring& src, const std::wstring& dst);
static void PrepareBackupDestination(const std::wstring& backupPath);
static bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup);
static bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath);
static bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath);
static bool RemoveDirectoryRecursive(const std::wstring& dir);

static bool WriteUtf8File(const std::wstring& path, const char* content);
static bool WriteUtf8File(const std::wstring& path, const std::string& content);

static std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode);
static bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason);
static std::wstring Utf8ToWide(const std::string& s);
static std::string ParseJsonString(const std::string& json, size_t& pos);
static size_t SkipJsonWhitespace(const std::string& json, size_t pos);
static size_t SkipJsonValue(const std::string& json, size_t pos);
static size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key);
static std::vector<std::wstring> ParseJsonStringArray(const std::string& json, const char* key);
static std::wstring ParseJsonStringProperty(const std::string& json, const char* key);
static bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson = nullptr);
static void PrintPlanEntries(const wchar_t* title, const std::vector<std::wstring>& entries);
static int RunPlanParserTest(const std::wstring& initialPlanFile);

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

static std::wstring g_logFile;

static bool TryConvertWideToUtf8(const std::wstring& wide, std::string& utf8)
{
    int utf8Len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (utf8Len <= 0) {
        utf8.clear();
        return false;
    }

    utf8.assign(static_cast<size_t>(utf8Len), '\0');
    return WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        utf8Len,
        nullptr,
        nullptr) == utf8Len;
}

static void WriteLog(const wchar_t* message)
{
    if (g_logFile.empty()) return;

    SYSTEMTIME st {};
    GetLocalTime(&st);

    wchar_t buf[64];
    _snwprintf_s(buf, _countof(buf), _TRUNCATE,
                 L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                 st.wYear, st.wMonth, st.wDay,
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    std::wstring line = buf;
    line += message;
    line += L"\r\n";

    // Ensure parent directory exists
    size_t sep = g_logFile.rfind(L'\\');
    if (sep != std::wstring::npos) {
        std::wstring dir = g_logFile.substr(0, sep);
        CreateDirectoryW(dir.c_str(), nullptr);
    }

    HANDLE hFile = CreateFileW(
        g_logFile.c_str(),
        FILE_APPEND_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Write UTF-8
    std::string utf8;
    if (TryConvertWideToUtf8(line, utf8)) {
        DWORD written = 0;
        WriteFile(hFile, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    CloseHandle(hFile);
}

static void WriteLogF(const wchar_t* fmt, ...)
{
    wchar_t buf[2048];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    WriteLog(buf);
}

static void WriteConsoleText(FILE* stream, const std::wstring& text, bool appendNewline)
{
    std::string utf8;
    if (TryConvertWideToUtf8(text, utf8)) {
        fwrite(utf8.data(), 1, utf8.size(), stream);
    }
    if (appendNewline) {
        fwrite("\n", 1, 1, stream);
    }
    fflush(stream);
}

static constexpr LONGLONG MAX_UPDATER_LOG_SIZE = 4LL * 1024 * 1024;

static void RotateLogIfNeeded()
{
    if (g_logFile.empty() || !PathExistsW(g_logFile)) {
        return;
    }

    HANDLE hFile = CreateFileW(
        g_logFile.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER size {};
    bool shouldRotate = GetFileSizeEx(hFile, &size) && size.QuadPart > MAX_UPDATER_LOG_SIZE;
    CloseHandle(hFile);

    if (!shouldRotate) {
        return;
    }

    std::wstring bakFile = g_logFile + L".bak";
    DeleteFileW(bakFile.c_str());
    MoveFileExW(g_logFile.c_str(), bakFile.c_str(), MOVEFILE_REPLACE_EXISTING);
}

// ---------------------------------------------------------------------------
// Path utilities
// ---------------------------------------------------------------------------

static std::wstring EnsureTrailingSeparator(const std::wstring& path)
{
    if (!path.empty() && path.back() != L'\\')
        return path + L'\\';
    return path;
}

static std::wstring NormalizeRelativePath(const std::wstring& relativePath)
{
    std::wstring normalized = relativePath;
    for (wchar_t& c : normalized) {
        if (c == L'/') {
            c = L'\\';
        }
    }

    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L' ' || normalized.back() == L'\t')) {
        normalized.pop_back();
    }

    return normalized;
}

static bool EqualsIgnoreCase(const std::wstring& left, const wchar_t* right)
{
    return _wcsicmp(left.c_str(), right) == 0;
}

static bool IsRecycleAndReplaceDirectory(const std::wstring& relativePath)
{
    std::wstring normalized = NormalizeRelativePath(relativePath);
    return EqualsIgnoreCase(normalized, L"resource") || EqualsIgnoreCase(normalized, L"externals");
}

// Resolves `relativePath` under `rootPath`, writes result to `out`.
// Returns false if the path is rooted, empty, or escapes the root.
static bool TryResolvePathUnderRoot(
    const std::wstring& rootPath,
    const std::wstring& relativePath,
    std::wstring& out)
{
    out.clear();

    // Reject empty or whitespace
    if (relativePath.empty()) return false;
    bool allSpace = true;
    for (wchar_t c : relativePath)
        if (c != L' ' && c != L'\t') { allSpace = false; break; }
    if (allSpace) return false;

    // Normalise slashes
    std::wstring rel = relativePath;
    for (wchar_t& c : rel)
        if (c == L'/') c = L'\\';

    // Reject absolute paths
    if (rel.size() >= 2 && rel[1] == L':') return false;
    if (!rel.empty() && rel[0] == L'\\') return false;

    // Build candidate and canonicalise
    std::wstring combined = rootPath + L'\\' + rel;
    wchar_t full[MAX_PATH * 4];
    DWORD len = GetFullPathNameW(combined.c_str(), _countof(full), full, nullptr);
    if (len == 0 || len >= _countof(full)) return false;

    std::wstring candidate = full;

    // Root must be canonical too
    DWORD rootLen = GetFullPathNameW(rootPath.c_str(), _countof(full), full, nullptr);
    if (rootLen == 0 || rootLen >= _countof(full)) return false;
    std::wstring normalRoot = EnsureTrailingSeparator(std::wstring(full));

    // Case-insensitive prefix check
    if (candidate.size() < normalRoot.size()) return false;
    if (_wcsnicmp(candidate.c_str(), normalRoot.c_str(), normalRoot.size()) != 0) return false;

    out = candidate;
    return true;
}

// ---------------------------------------------------------------------------
// File / directory helpers
// ---------------------------------------------------------------------------

static bool PathExistsW(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool IsDirectory(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static bool CopyDirectoryRecursive(const std::wstring& sourceDir, const std::wstring& destinationDir)
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

static bool MovePathToRecycleBin(const std::wstring& path)
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

static bool CopyPathEntry(const std::wstring& sourcePath, const std::wstring& destinationPath)
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

static void EnsureParentDirectory(const std::wstring& path)
{
    size_t sep = path.rfind(L'\\');
    if (sep == std::wstring::npos) return;
    std::wstring parent = path.substr(0, sep);
    if (parent.empty()) return;

    // Recursively create
    EnsureParentDirectory(parent);
    CreateDirectoryW(parent.c_str(), nullptr);
}

static std::wstring CreateArchivedPath(const std::wstring& base)
{
    SYSTEMTIME st {};
    GetLocalTime(&st);
    wchar_t ts[32];
    _snwprintf_s(ts, _countof(ts), _TRUNCATE,
                 L".%04d%02d%02d%02d%02d%02d.",
                 st.wYear, st.wMonth, st.wDay,
                 st.wHour, st.wMinute, st.wSecond);

    int index = 0;
    while (true) {
        wchar_t idx[16];
        _itow_s(index, idx, _countof(idx), 10);
        std::wstring candidate = base + ts + idx;
        if (!PathExistsW(candidate)) return candidate;
        index++;
    }
}

// Move a file or directory entry (handles cross-volume by CopyFile+Delete for files)
static bool MovePathEntry(const std::wstring& src, const std::wstring& dst)
{
    EnsureParentDirectory(dst);

    DWORD attr = GetFileAttributesW(src.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return MoveFileExW(src.c_str(), dst.c_str(), 0) != FALSE;
    }

    // Try atomic move first; fall back to copy+delete for cross-volume
    if (MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE)
        return true;

    if (!CopyFileW(src.c_str(), dst.c_str(), FALSE)) return false;
    DeleteFileW(src.c_str());
    return true;
}

static void PrepareBackupDestination(const std::wstring& backupPath)
{
    EnsureParentDirectory(backupPath);
    if (!PathExistsW(backupPath)) return;

    std::wstring archived = CreateArchivedPath(backupPath);
    MovePathEntry(backupPath, archived);
}

static bool MoveExistingPathToBackup(const std::wstring& src, const std::wstring& backup)
{
    PrepareBackupDestination(backup);
    return MovePathEntry(src, backup);
}

static bool RecycleAndBackupDirectory(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    if (!CopyDirectoryRecursive(sourcePath, backupPath)) {
        return false;
    }

    return MovePathToRecycleBin(sourcePath);
}

static bool RecycleAndBackupPath(const std::wstring& sourcePath, const std::wstring& backupPath)
{
    PrepareBackupDestination(backupPath);

    if (!CopyPathEntry(sourcePath, backupPath)) {
        return false;
    }

    return MovePathToRecycleBin(sourcePath);
}

static bool RemoveDirectoryRecursive(const std::wstring& dir)
{
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd {};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
                continue;
            std::wstring child = dir + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                RemoveDirectoryRecursive(child);
            else
                DeleteFileW(child.c_str());
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    return RemoveDirectoryW(dir.c_str()) != FALSE;
}

// ---------------------------------------------------------------------------
// Write a small UTF-8 text file
// ---------------------------------------------------------------------------

static bool WriteUtf8File(const std::wstring& path, const char* content)
{
    return WriteUtf8File(path, std::string(content));
}

static bool WriteUtf8File(const std::wstring& path, const std::string& content)
{
    EnsureParentDirectory(path);
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    DWORD len = static_cast<DWORD>(content.size());
    bool ok = WriteFile(hFile, content.data(), len, &written, nullptr) != FALSE;
    CloseHandle(hFile);
    return ok && written == len;
}

// ---------------------------------------------------------------------------
// Minimal JSON parser for the plan file
// { "packageType": "full|ota", "removeList": [ "...", ... ], "moveList": [ "...", ... ] }
// ---------------------------------------------------------------------------

static std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode)
{
    wchar_t buf[512];
    _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%s: %s (error=%lu)", action, path.c_str(), errorCode);
    return buf;
}

static bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason)
{
    content.clear();
    failureReason.clear();

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        failureReason = BuildFileIoFailureReason(L"Failed to open file", path, GetLastError());
        return false;
    }

    std::string buf;
    char chunk[4096];
    while (true) {
        DWORD chunkRead = 0;
        if (!ReadFile(hFile, chunk, static_cast<DWORD>(sizeof(chunk)), &chunkRead, nullptr)) {
            failureReason = BuildFileIoFailureReason(L"Failed to read file", path, GetLastError());
            CloseHandle(hFile);
            return false;
        }

        if (chunkRead == 0) {
            break;
        }

        buf.append(chunk, chunkRead);
    }

    CloseHandle(hFile);
    content.swap(buf);
    return true;
}

// Convert a UTF-8 JSON string value to std::wstring.
// Handles basic \uXXXX, \n, \r, \t, \\, \/
static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 1) return {};
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

// Parse a JSON string literal starting at pos (after the opening quote).
// Advances pos past the closing quote. Returns the raw UTF-8 content.
static std::string ParseJsonString(const std::string& json, size_t& pos)
{
    std::string result;
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '"') break;
        if (c == '\\' && pos < json.size()) {
            char esc = json[pos++];
            switch (esc) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case '/':  result += '/';  break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case 'u': {
                if (pos + 4 <= json.size()) {
                    char hex[5] = {};
                    memcpy(hex, json.c_str() + pos, 4);
                    pos += 4;
                    unsigned cp = static_cast<unsigned>(strtoul(hex, nullptr, 16));
                    // Encode code point as UTF-8
                    if (cp < 0x80) {
                        result += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        result += static_cast<char>(0xC0 | (cp >> 6));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (cp >> 12));
                        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                }
                break;
            }
            default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    return result;
}

static size_t SkipJsonWhitespace(const std::string& json, size_t pos)
{
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\r' || json[pos] == '\n'))
    {
        ++pos;
    }
    return pos;
}

// Skips one JSON value starting at `pos` and returns the position right after it.
static size_t SkipJsonValue(const std::string& json, size_t pos)
{
    pos = SkipJsonWhitespace(json, pos);
    if (pos >= json.size()) return pos;

    char c = json[pos];
    if (c == '"') {
        ++pos;
        ParseJsonString(json, pos);
        return pos;
    }

    if (c == '{' || c == '[') {
        char open = c;
        char close = (c == '{') ? '}' : ']';
        int depth = 0;
        bool inString = false;
        bool escaped = false;

        for (; pos < json.size(); ++pos) {
            char ch = json[pos];
            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (ch == '\\') {
                    escaped = true;
                } else if (ch == '"') {
                    inString = false;
                }
                continue;
            }

            if (ch == '"') {
                inString = true;
                continue;
            }

            if (ch == open) {
                ++depth;
            } else if (ch == close) {
                --depth;
                if (depth == 0) {
                    ++pos;
                    break;
                }
            }
        }

        return pos;
    }

    // number / true / false / null
    while (pos < json.size()) {
        char ch = json[pos];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        ++pos;
    }
    return pos;
}

// Finds `"key": <value>` in the top-level JSON object and returns the start of `<value>`.
static size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key)
{
    size_t pos = SkipJsonWhitespace(json, 0);
    if (pos >= json.size() || json[pos] != '{') return std::string::npos;
    ++pos;

    while (pos < json.size()) {
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (json[pos] == ',') {
            ++pos;
            continue;
        }

        if (json[pos] == '}') {
            return std::string::npos;
        }

        if (json[pos] != '"') {
            ++pos;
            continue;
        }

        ++pos;
        std::string currentKey = ParseJsonString(json, pos);

        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            return std::string::npos;
        }

        ++pos;
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (currentKey == key) {
            return pos;
        }

        pos = SkipJsonValue(json, pos);
    }

    return std::string::npos;
}

// Find a JSON array by key name and return its string elements.
static std::vector<std::wstring> ParseJsonStringArray(
    const std::string& json,
    const char* key)
{
    std::vector<std::wstring> result;

    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return result;

    if (pos >= json.size() || json[pos] != '[') return result;
    pos++; // consume '['

    // Parse array elements
    while (pos < json.size()) {
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                      json[pos] == '\r' || json[pos] == '\n' ||
                                      json[pos] == ','))
            pos++;

        if (pos >= json.size()) break;
        if (json[pos] == ']') break;
        if (json[pos] == '"') {
            pos++; // consume opening quote
            std::string val = ParseJsonString(json, pos);
            result.push_back(Utf8ToWide(val));
        } else {
            pos++;
        }
    }

    return result;
}

static std::wstring ParseJsonStringProperty(const std::string& json, const char* key)
{
    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return {};

    if (pos >= json.size() || json[pos] != '"') return {};

    pos++;
    return Utf8ToWide(ParseJsonString(json, pos));
}

struct PendingUpdatePlan
{
    std::wstring packageType;
    std::vector<std::wstring> removeList;
    std::vector<std::wstring> moveList;
};

static bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson)
{
    outPlan = {};
    failureReason.clear();

    if (!PathExistsW(planFile)) {
        failureReason = L"Plan file not found: " + planFile;
        return false;
    }

    std::string planJson;
    if (!TryReadUtf8File(planFile, planJson, failureReason)) {
        return false;
    }

    if (rawJson != nullptr) {
        *rawJson = planJson;
    }

    if (planJson.empty()) {
        failureReason = L"Plan file is empty: " + planFile;
        return false;
    }

    outPlan.packageType = ParseJsonStringProperty(planJson, "packageType");
    outPlan.removeList = ParseJsonStringArray(planJson, "removeList");
    outPlan.moveList = ParseJsonStringArray(planJson, "moveList");
    return true;
}

static void PrintPlanEntries(const wchar_t* title, const std::vector<std::wstring>& entries)
{
    WriteConsoleText(stdout, std::wstring(title) + L" (" + std::to_wstring(entries.size()) + L")", true);
    for (const std::wstring& entry : entries) {
        WriteConsoleText(stdout, L"  - " + entry, true);
    }
}

static int RunPlanParserTest(const std::wstring& initialPlanFile)
{
    std::wstring planFile = initialPlanFile;
    if (planFile.empty()) {
        WriteConsoleText(stdout, L"请输入要解析的 plan 文件路径: ", false);
        std::getline(std::wcin, planFile);
    }

    if (planFile.empty()) {
        WriteConsoleText(stderr, L"未提供 plan 文件路径。", true);
        return 1;
    }

    PendingUpdatePlan plan;
    std::wstring failureReason;
    std::string rawJson;
    if (!LoadPendingUpdatePlan(planFile, plan, failureReason, &rawJson)) {
        WriteConsoleText(stderr, failureReason, true);
        return 2;
    }

    WriteConsoleText(stdout, L"文件读取成功: " + planFile, true);
    WriteConsoleText(stdout, L"原始字节数: " + std::to_wstring(rawJson.size()), true);
    WriteConsoleText(stdout, L"packageType: " + (plan.packageType.empty() ? std::wstring(L"<empty>") : plan.packageType), true);
    PrintPlanEntries(L"removeList", plan.removeList);
    PrintPlanEntries(L"moveList", plan.moveList);
    return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int wmain(int argc, wchar_t* argv[])
{
    constexpr int REQUIRED_ARGS = 9; // excluding argv[0]

    if (argc >= 2 && wcscmp(argv[1], L"--test-plan") == 0) {
        return RunPlanParserTest(argc >= 3 ? argv[2] : L"");
    }

    if (argc - 1 < REQUIRED_ARGS) {
        MessageBoxW(
            nullptr,
            L"MAA.Updater.exe 是 MAA 内部使用的更新程序，不应被手动启动。\n\n"
            L"请直接运行 MAA.exe。\n\n"
            L"MAA.Updater.exe is an updater used internally by MAA and should not be manually started.\n\n"
            L"Please run MAA.exe directly.",
            L"MAA 更新程序 / MAA Updater",
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

    g_logFile = rootDir + L"\\debug\\pending-update-applier.log";
    RotateLogIfNeeded();

    WriteLog(L"MAA.Updater started (C++ external updater).");
    WriteLogF(L"ParentPID=%lu RootDir=%s", parentPid, rootDir.c_str());
    WriteLogF(L"PlanFile=%s ExtractDir=%s", planFile.c_str(), extractDir.c_str());

    bool shouldRelaunch = false;
    bool success = false;
    std::wstring failureReason;

    // ------------------------------------------------------------------
    // Wait for parent process to exit
    // ------------------------------------------------------------------
    HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (hParent != nullptr) {
        WriteLogF(L"Waiting for parent process %lu to exit...", parentPid);
        WaitForSingleObject(hParent, INFINITE);
        CloseHandle(hParent);
        WriteLog(L"Parent process exited.");
    } else {
        WriteLogF(L"Could not open parent process %lu (already exited?), continuing.", parentPid);
    }

    // ------------------------------------------------------------------
    // Read plan
    // ------------------------------------------------------------------
    do {
        PendingUpdatePlan plan;
        if (!LoadPendingUpdatePlan(planFile, plan, failureReason)) {
            WriteLog(failureReason.c_str());
            break;
        }

        bool isFullPackage = EqualsIgnoreCase(plan.packageType, L"full");
        const std::vector<std::wstring>& removeList = plan.removeList;
        const std::vector<std::wstring>& moveList = plan.moveList;

        WriteLogF(L"Plan loaded: packageType=%s removeList=%zu moveList=%zu",
                  plan.packageType.c_str(), removeList.size(), moveList.size());

        CreateDirectoryW(backupDir.c_str(), nullptr);

        // ---- Remove entries ----
        for (const std::wstring& rel : removeList) {
            std::wstring targetPath;
            if (!TryResolvePathUnderRoot(rootDir, rel, targetPath)) {
                failureReason = L"Illegal path in removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }
            if (!PathExistsW(targetPath)) continue;

            std::wstring backupPath;
            if (!TryResolvePathUnderRoot(backupDir, rel, backupPath)) {
                failureReason = L"Illegal backup path for removeList: " + rel;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }

            WriteLogF(L"Removing: %s -> backup %s", targetPath.c_str(), backupPath.c_str());
            bool backupOk = isFullPackage
                ? RecycleAndBackupPath(targetPath, backupPath)
                : MoveExistingPathToBackup(targetPath, backupPath);
            if (!backupOk) {
                failureReason = L"Failed to move to backup: " + targetPath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }
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
                WriteLogF(L"Backing up existing: %s", targetPath.c_str());
                bool backupOk = IsRecycleAndReplaceDirectory(rel)
                    ? RecycleAndBackupDirectory(targetPath, backupPath)
                    : MoveExistingPathToBackup(targetPath, backupPath);
                if (!backupOk) {
                    failureReason = L"Failed to backup existing file: " + targetPath;
                    WriteLog(failureReason.c_str());
                    goto apply_failed;
                }
            }

            WriteLogF(L"Installing: %s -> %s", sourcePath.c_str(), targetPath.c_str());
            if (!MovePathEntry(sourcePath, targetPath)) {
                failureReason = L"Failed to move file into place: " + sourcePath;
                WriteLog(failureReason.c_str());
                goto apply_failed;
            }
        }

        // ---- Cleanup package ----
        if (PathExistsW(packagePath)) {
            DeleteFileW(packagePath.c_str());
            WriteLogF(L"Deleted package: %s", packagePath.c_str());
        }

        if (PathExistsW(failureStatusFile))
            DeleteFileW(failureStatusFile.c_str());

        if (WriteUtf8File(successStatusFile, "succeeded")) {
            WriteLogF(L"Wrote success status: %s", successStatusFile.c_str());
            success = true;
            shouldRelaunch = true;
        } else {
            failureReason = L"Failed to write success status file: " + successStatusFile;
            WriteLog(failureReason.c_str());
        }

        break; // normal exit from do-while

    apply_failed:
        success = false;
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
        WriteLogF(L"Update failed: %s", failureReason.c_str());
    }

    // ------------------------------------------------------------------
    // Cleanup extract dir and plan file
    // ------------------------------------------------------------------
    if (PathExistsW(extractDir)) {
        WriteLogF(L"Cleaning extract dir: %s", extractDir.c_str());
        RemoveDirectoryRecursive(extractDir);
    }

    if (PathExistsW(planFile))
        DeleteFileW(planFile.c_str());

    // ------------------------------------------------------------------
    // Relaunch MAA
    // ------------------------------------------------------------------
    if (shouldRelaunch && PathExistsW(relaunchExecutable)) {
        WriteLogF(L"Relaunching: %s", relaunchExecutable.c_str());

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
            WriteLog(L"Relaunch successful.");
        } else {
            WriteLogF(L"Relaunch failed, error=%lu", GetLastError());
        }
    }

    WriteLog(L"MAA.Updater exiting.");
    return success ? 0 : 2;
}
