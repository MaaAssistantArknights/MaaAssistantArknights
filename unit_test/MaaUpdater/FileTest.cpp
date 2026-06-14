#include <catch2/catch_all.hpp>

#include <string>
#include <windows.h>

#include "UpdaterFile.h"

// Helper: get a temp directory path for test file operations
static std::wstring GetTempTestDir()
{
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring dir = std::wstring(tempPath) + L"MaaUpdaterTest\\";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

// Helper: remove a test directory and all its contents
static void CleanupTempTestDir(const std::wstring& dir)
{
    ForceRemoveDirectoryRecursive(dir);
}

// ============================================================================
// PathExistsW
// ============================================================================

TEST_CASE("PathExistsW returns true for existing directory", "[file][exists]")
{
    auto dir = GetTempTestDir();
    REQUIRE(PathExistsW(dir));
    CleanupTempTestDir(dir);
}

TEST_CASE("PathExistsW returns true for existing file", "[file][exists]")
{
    auto dir = GetTempTestDir();
    std::wstring file = dir + L"test.txt";
    HANDLE h = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    CloseHandle(h);

    REQUIRE(PathExistsW(file));
    CleanupTempTestDir(dir);
}

TEST_CASE("PathExistsW returns false for nonexistent path", "[file][exists]")
{
    REQUIRE_FALSE(PathExistsW(L"C:\\NONEXISTENT_PATH_12345_xyz"));
}

// ============================================================================
// IsDirectory
// ============================================================================

TEST_CASE("IsDirectory returns true for directory", "[file][isdir]")
{
    auto dir = GetTempTestDir();
    REQUIRE(IsDirectory(dir));
    CleanupTempTestDir(dir);
}

TEST_CASE("IsDirectory returns false for file", "[file][isdir]")
{
    auto dir = GetTempTestDir();
    std::wstring file = dir + L"test.txt";
    HANDLE h = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    CloseHandle(h);

    REQUIRE_FALSE(IsDirectory(file));
    CleanupTempTestDir(dir);
}

// ============================================================================
// EnsureParentDirectory
// ============================================================================

TEST_CASE("EnsureParentDirectory creates nested directories", "[file][parent]")
{
    auto root = GetTempTestDir();
    std::wstring deep = root + L"a\\b\\c\\file.txt";
    EnsureParentDirectory(deep);

    REQUIRE(PathExistsW(root + L"a"));
    REQUIRE(PathExistsW(root + L"a\\b"));
    REQUIRE(PathExistsW(root + L"a\\b\\c"));
    REQUIRE(IsDirectory(root + L"a\\b\\c"));

    CleanupTempTestDir(root);
}

// ============================================================================
// WriteUtf8File / read back
// ============================================================================

TEST_CASE("WriteUtf8File writes and can be read back", "[file][io]")
{
    auto dir = GetTempTestDir();
    std::wstring file = dir + L"utf8test.txt";

    const char* content = "Hello UTF-8: \xe4\xb8\xad\xe6\x96\x87"; // "中文"
    REQUIRE(WriteUtf8File(file, content));

    // Read back with Win32 API
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);

    char buf[256] = {};
    DWORD read = 0;
    REQUIRE(ReadFile(h, buf, sizeof(buf) - 1, &read, nullptr));
    CloseHandle(h);

    REQUIRE(std::string(buf, read) == content);

    CleanupTempTestDir(dir);
}

TEST_CASE("WriteUtf8File with std::string overload", "[file][io]")
{
    auto dir = GetTempTestDir();
    std::wstring file = dir + L"utf8test2.txt";

    std::string content = "test content";
    REQUIRE(WriteUtf8File(file, content));

    REQUIRE(PathExistsW(file));

    CleanupTempTestDir(dir);
}

// ============================================================================
// CreateArchivedPath
// ============================================================================

TEST_CASE("CreateArchivedPath generates timestamped path", "[file][archive]")
{
    auto dir = GetTempTestDir();
    std::wstring base = dir + L"backup";
    std::wstring archived = CreateArchivedPath(base);

    // Should have format: base.YYYYMMDDHHMMSS.0
    REQUIRE(archived.size() > base.size());
    REQUIRE(archived.find(base) == 0);

    CleanupTempTestDir(dir);
}

TEST_CASE("CreateArchivedPath avoids collision with existing paths", "[file][archive]")
{
    auto dir = GetTempTestDir();
    std::wstring base = dir + L"backup";

    // Create the first candidate path so it's taken
    std::wstring first = CreateArchivedPath(base);
    // Touch the file so PathExistsW returns true
    HANDLE h = CreateFileW(first.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    CloseHandle(h);

    std::wstring second = CreateArchivedPath(base);
    REQUIRE(second != first);

    CleanupTempTestDir(dir);
}

// ============================================================================
// CopyDirectoryRecursive
// ============================================================================

TEST_CASE("CopyDirectoryRecursive copies files and subdirs", "[file][copy]")
{
    auto dir = GetTempTestDir();
    std::wstring src = dir + L"src";
    std::wstring dst = dir + L"dst";

    // Create source structure
    CreateDirectoryW(src.c_str(), nullptr);
    CreateDirectoryW((src + L"\\sub").c_str(), nullptr);
    {
        HANDLE h = CreateFileW((src + L"\\a.txt").c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        CloseHandle(h);
    }
    {
        HANDLE h = CreateFileW((src + L"\\sub\\b.txt").c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        CloseHandle(h);
    }

    REQUIRE(CopyDirectoryRecursive(src, dst));

    REQUIRE(PathExistsW(dst));
    REQUIRE(IsDirectory(dst));
    REQUIRE(PathExistsW(dst + L"\\a.txt"));
    REQUIRE(PathExistsW(dst + L"\\sub\\b.txt"));

    CleanupTempTestDir(dir);
}

// ============================================================================
// MovePathEntry
// ============================================================================

TEST_CASE("MovePathEntry moves file within same volume", "[file][move]")
{
    auto dir = GetTempTestDir();
    std::wstring src = dir + L"move_src.txt";
    std::wstring dst = dir + L"move_dst.txt";

    {
        HANDLE h = CreateFileW(src.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(h != INVALID_HANDLE_VALUE);
        CloseHandle(h);
    }

    REQUIRE(MovePathEntry(src, dst));
    REQUIRE_FALSE(PathExistsW(src));
    REQUIRE(PathExistsW(dst));

    CleanupTempTestDir(dir);
}
