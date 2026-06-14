#include <catch2/catch_all.hpp>

#include "UpdaterPath.h"

// ============================================================================
// EnsureTrailingSeparator
// ============================================================================

TEST_CASE("EnsureTrailingSeparator adds backslash to non-empty paths", "[path][separator]")
{
    REQUIRE(EnsureTrailingSeparator(L"C:\\MAA") == L"C:\\MAA\\");
    REQUIRE(EnsureTrailingSeparator(L"C:\\MAA\\sub") == L"C:\\MAA\\sub\\");
}

TEST_CASE("EnsureTrailingSeparator preserves existing backslash", "[path][separator]")
{
    REQUIRE(EnsureTrailingSeparator(L"C:\\MAA\\") == L"C:\\MAA\\");
    REQUIRE(EnsureTrailingSeparator(L"D:\\") == L"D:\\");
}

TEST_CASE("EnsureTrailingSeparator handles empty string", "[path][separator]")
{
    REQUIRE(EnsureTrailingSeparator(L"") == L"");
}

// ============================================================================
// NormalizeRelativePath
// ============================================================================

TEST_CASE("NormalizeRelativePath converts forward slashes to backslashes", "[path][normalize]")
{
    REQUIRE(NormalizeRelativePath(L"a/b/c") == L"a\\b\\c");
    REQUIRE(NormalizeRelativePath(L"a/b/c/") == L"a\\b\\c");
}

TEST_CASE("NormalizeRelativePath trims trailing spaces and tabs", "[path][normalize]")
{
    REQUIRE(NormalizeRelativePath(L"foo   ") == L"foo");
    REQUIRE(NormalizeRelativePath(L"foo\t\t") == L"foo");
    REQUIRE(NormalizeRelativePath(L"foo \\ ") == L"foo");
}

TEST_CASE("NormalizeRelativePath handles empty and already-clean paths", "[path][normalize]")
{
    REQUIRE(NormalizeRelativePath(L"").empty());
    REQUIRE(NormalizeRelativePath(L"foo") == L"foo");
}

// ============================================================================
// EqualsIgnoreCase
// ============================================================================

TEST_CASE("EqualsIgnoreCase matches same string", "[path][equals]")
{
    REQUIRE(EqualsIgnoreCase(L"resource", L"resource"));
}

TEST_CASE("EqualsIgnoreCase matches different case", "[path][equals]")
{
    REQUIRE(EqualsIgnoreCase(L"Resource", L"resource"));
    REQUIRE(EqualsIgnoreCase(L"RESOURCE", L"resource"));
    REQUIRE(EqualsIgnoreCase(L"resource", L"RESOURCE"));
}

TEST_CASE("EqualsIgnoreCase rejects different strings", "[path][equals]")
{
    REQUIRE_FALSE(EqualsIgnoreCase(L"resource", L"resources"));
    REQUIRE_FALSE(EqualsIgnoreCase(L"", L"resource"));
    REQUIRE_FALSE(EqualsIgnoreCase(L"resource", L""));
}

// ============================================================================
// IsRecycleAndReplaceDirectory
// ============================================================================

TEST_CASE("IsRecycleAndReplaceDirectory matches resource", "[path][recycle]")
{
    REQUIRE(IsRecycleAndReplaceDirectory(L"resource"));
    REQUIRE(IsRecycleAndReplaceDirectory(L"Resource"));
    REQUIRE(IsRecycleAndReplaceDirectory(L"RESOURCE"));
}

TEST_CASE("IsRecycleAndReplaceDirectory matches externals", "[path][recycle]")
{
    REQUIRE(IsRecycleAndReplaceDirectory(L"externals"));
    REQUIRE(IsRecycleAndReplaceDirectory(L"Externals"));
}

TEST_CASE("IsRecycleAndReplaceDirectory ignores trailing content", "[path][recycle]")
{
    // NormalizeRelativePath trims trailing backslashes/spaces first
    REQUIRE(IsRecycleAndReplaceDirectory(L"resource\\") == true);
    REQUIRE(IsRecycleAndReplaceDirectory(L"resource ") == true);
}

TEST_CASE("IsRecycleAndReplaceDirectory rejects other paths", "[path][recycle]")
{
    REQUIRE_FALSE(IsRecycleAndReplaceDirectory(L""));
    REQUIRE_FALSE(IsRecycleAndReplaceDirectory(L"docs"));
    REQUIRE_FALSE(IsRecycleAndReplaceDirectory(L"resource/sub"));
}

// ============================================================================
// TryResolvePathUnderRoot
// ============================================================================

TEST_CASE("TryResolvePathUnderRoot resolves valid relative paths", "[path][resolve]")
{
    std::wstring out;
    REQUIRE(TryResolvePathUnderRoot(L"C:\\MAA", L"resource", out));
    CHECK(out.size() > 0);
    // Must start with the canonical root
    CHECK(out.find(L"C:\\MAA") != std::wstring::npos);
}

TEST_CASE("TryResolvePathUnderRoot rejects empty relative path", "[path][resolve]")
{
    std::wstring out;
    REQUIRE_FALSE(TryResolvePathUnderRoot(L"C:\\MAA", L"", out));
    REQUIRE(out.empty());
}

TEST_CASE("TryResolvePathUnderRoot rejects whitespace-only relative path", "[path][resolve]")
{
    std::wstring out;
    REQUIRE_FALSE(TryResolvePathUnderRoot(L"C:\\MAA", L"   ", out));
    REQUIRE(out.empty());
}

TEST_CASE("TryResolvePathUnderRoot rejects absolute drive-letter path", "[path][resolve]")
{
    std::wstring out;
    REQUIRE_FALSE(TryResolvePathUnderRoot(L"C:\\MAA", L"D:\\evil", out));
    REQUIRE(out.empty());
}

TEST_CASE("TryResolvePathUnderRoot rejects root-relative path", "[path][resolve]")
{
    std::wstring out;
    REQUIRE_FALSE(TryResolvePathUnderRoot(L"C:\\MAA", L"\\evil", out));
    REQUIRE(out.empty());
}

TEST_CASE("TryResolvePathUnderRoot normalizes mixed slashes", "[path][resolve]")
{
    std::wstring out;
    REQUIRE(TryResolvePathUnderRoot(L"C:\\MAA", L"sub/dir/file.txt", out));
    CHECK(out.find(L"MAA\\sub\\dir\\file.txt") != std::wstring::npos);
}
