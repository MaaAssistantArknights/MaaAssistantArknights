#include <catch2/catch_all.hpp>

#include <string>
#include <windows.h>

#include "UpdaterLog.h"

// ============================================================================
// TryConvertWideToUtf8
// ============================================================================

TEST_CASE("TryConvertWideToUtf8 converts ASCII", "[log][utf8]")
{
    std::string utf8;
    REQUIRE(TryConvertWideToUtf8(L"hello", utf8));
    REQUIRE(utf8 == "hello");
}

TEST_CASE("TryConvertWideToUtf8 converts CJK", "[log][utf8]")
{
    std::string utf8;
    REQUIRE(TryConvertWideToUtf8(L"\u4e2d\u6587", utf8)); // 中文
    // 中 = E4 B8 AD, 文 = E6 96 87
    REQUIRE(utf8 == "\xe4\xb8\xad\xe6\x96\x87");
}

TEST_CASE("TryConvertWideToUtf8 handles empty string", "[log][utf8]")
{
    // Implementation returns false for empty string (WideCharToMultiByte reports 0 bytes)
    std::string utf8;
    REQUIRE_FALSE(TryConvertWideToUtf8(L"", utf8));
    REQUIRE(utf8.empty());
}

TEST_CASE("TryConvertWideToUtf8 round-trips", "[log][utf8]")
{
    std::wstring original = L"Hello World \u4e2d\u6587!";
    std::string utf8;
    REQUIRE(TryConvertWideToUtf8(original, utf8));

    // Convert back
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    REQUIRE(len > 1);
    std::wstring back(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, back.data(), len);
    REQUIRE(back == original);
}
