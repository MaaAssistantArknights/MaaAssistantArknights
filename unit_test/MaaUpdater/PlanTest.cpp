#include <catch2/catch_all.hpp>

#include <string>
#include <vector>
#include <windows.h>

#include "UpdaterPlan.h"

// ============================================================================
// Utf8ToWide
// ============================================================================

TEST_CASE("Utf8ToWide converts ASCII", "[plan][utf8]")
{
    std::wstring result = Utf8ToWide("hello");
    REQUIRE(result == L"hello");
}

TEST_CASE("Utf8ToWide converts CJK characters", "[plan][utf8]")
{
    // UTF-8 for 中文
    std::wstring result = Utf8ToWide("\xe4\xb8\xad\xe6\x96\x87");
    REQUIRE(result == L"\u4e2d\u6587");
}

TEST_CASE("Utf8ToWide handles empty string", "[plan][utf8]")
{
    std::wstring result = Utf8ToWide("");
    REQUIRE(result.empty());
}

// ============================================================================
// SkipJsonWhitespace
// ============================================================================

TEST_CASE("SkipJsonWhitespace skips spaces tabs newlines", "[plan][json]")
{
    std::string json = "  \t\r\n  \"key\"";
    size_t pos = SkipJsonWhitespace(json, 0);
    REQUIRE(pos == 7);
    REQUIRE(json[pos] == '"');
}

TEST_CASE("SkipJsonWhitespace no-op on non-whitespace", "[plan][json]")
{
    std::string json = "\"key\"";
    size_t pos = SkipJsonWhitespace(json, 0);
    REQUIRE(pos == 0);
}

TEST_CASE("SkipJsonWhitespace handles end-of-string", "[plan][json]")
{
    std::string json = "   ";
    size_t pos = SkipJsonWhitespace(json, 0);
    REQUIRE(pos == json.size());
}

// ============================================================================
// ParseJsonString
// ============================================================================

TEST_CASE("ParseJsonString parses simple string", "[plan][json]")
{
    std::string json = "\"hello\"";
    size_t pos = 1; // after opening quote
    std::string result = ParseJsonString(json, pos);
    REQUIRE(result == "hello");
    REQUIRE(pos == json.size()); // past closing quote
}

TEST_CASE("ParseJsonString handles escape sequences", "[plan][json]")
{
    std::string json = "\"a\\nb\\tc\\\"d\\\\e\"";
    size_t pos = 1;
    std::string result = ParseJsonString(json, pos);
    REQUIRE(result == "a\nb\tc\"d\\e");
}

TEST_CASE("ParseJsonString handles unicode escapes", "[plan][json]")
{
    // \u4e2d = 中
    std::string json = "\"\\u4e2d\\u6587\"";
    size_t pos = 1;
    std::string result = ParseJsonString(json, pos);
    // 中 = E4 B8 AD, 文 = E6 96 87
    REQUIRE(result == "\xe4\xb8\xad\xe6\x96\x87");
}

TEST_CASE("ParseJsonString handles empty string", "[plan][json]")
{
    std::string json = "\"\"";
    size_t pos = 1;
    std::string result = ParseJsonString(json, pos);
    REQUIRE(result.empty());
}

// ============================================================================
// SkipJsonValue
// ============================================================================

TEST_CASE("SkipJsonValue skips string value", "[plan][json]")
{
    std::string json = "\"hello\", \"world\"";
    size_t pos = SkipJsonValue(json, 0);
    REQUIRE(pos == 7); // past the first string
    REQUIRE(json[pos] == ',');
}

TEST_CASE("SkipJsonValue skips nested object", "[plan][json]")
{
    std::string json = "{\"a\":{\"b\":1}}, \"next\"";
    size_t pos = SkipJsonValue(json, 0);
    REQUIRE(pos == 13); // {"a":{"b":1}} ends at ], comma at 13
    REQUIRE(json[pos] == ',');
}

TEST_CASE("SkipJsonValue skips array", "[plan][json]")
{
    std::string json = "[1,2,3], \"next\"";
    size_t pos = SkipJsonValue(json, 0);
    REQUIRE(pos == 7);
    REQUIRE(json[pos] == ',');
}

TEST_CASE("SkipJsonValue skips number", "[plan][json]")
{
    std::string json = "12345, \"next\"";
    size_t pos = SkipJsonValue(json, 0);
    REQUIRE(pos == 5);
    REQUIRE(json[pos] == ',');
}

TEST_CASE("SkipJsonValue skips boolean and null", "[plan][json]")
{
    std::string json = "true, false, null";
    size_t p1 = SkipJsonValue(json, 0);
    REQUIRE(json[p1] == ',');
    size_t p2 = SkipJsonValue(json, p1 + 1);
    REQUIRE(json[p2] == ',');
    size_t p3 = SkipJsonValue(json, p2 + 1);
    REQUIRE(p3 == json.size());
}

// ============================================================================
// FindTopLevelJsonValueStartByKey
// ============================================================================

TEST_CASE("FindTopLevelJsonValueStartByKey finds simple key", "[plan][json]")
{
    std::string json = "{\"key\": \"value\"}";
    size_t pos = FindTopLevelJsonValueStartByKey(json, "key");
    REQUIRE(pos != std::string::npos);
    REQUIRE(json[pos] == '"');
}

TEST_CASE("FindTopLevelJsonValueStartByKey finds key with object value", "[plan][json]")
{
    std::string json = "{\"data\": {\"nested\": 1}}";
    size_t pos = FindTopLevelJsonValueStartByKey(json, "data");
    REQUIRE(pos != std::string::npos);
    REQUIRE(json[pos] == '{');
}

TEST_CASE("FindTopLevelJsonValueStartByKey returns npos for missing key", "[plan][json]")
{
    std::string json = "{\"a\": 1, \"b\": 2}";
    size_t pos = FindTopLevelJsonValueStartByKey(json, "c");
    REQUIRE(pos == std::string::npos);
}

// ============================================================================
// ParseJsonStringArray
// ============================================================================

TEST_CASE("ParseJsonStringArray parses string array", "[plan][json]")
{
    std::string json = "{\"items\": [\"a\", \"b\", \"c\"]}";
    auto result = ParseJsonStringArray(json, "items");
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == L"a");
    REQUIRE(result[1] == L"b");
    REQUIRE(result[2] == L"c");
}

TEST_CASE("ParseJsonStringArray handles empty array", "[plan][json]")
{
    std::string json = "{\"items\": []}";
    auto result = ParseJsonStringArray(json, "items");
    REQUIRE(result.empty());
}

TEST_CASE("ParseJsonStringArray returns empty for missing key", "[plan][json]")
{
    std::string json = "{\"other\": [\"x\"]}";
    auto result = ParseJsonStringArray(json, "items");
    REQUIRE(result.empty());
}

// ============================================================================
// ParseJsonStringProperty
// ============================================================================

TEST_CASE("ParseJsonStringProperty extracts string value", "[plan][json]")
{
    std::string json = "{\"packageType\": \"full\"}";
    auto result = ParseJsonStringProperty(json, "packageType");
    REQUIRE(result == L"full");
}

TEST_CASE("ParseJsonStringProperty returns empty for missing key", "[plan][json]")
{
    std::string json = "{\"other\": \"value\"}";
    auto result = ParseJsonStringProperty(json, "missing");
    REQUIRE(result.empty());
}

// ============================================================================
// BuildFileIoFailureReason
// ============================================================================

TEST_CASE("BuildFileIoFailureReason formats correctly", "[plan][io]")
{
    auto result = BuildFileIoFailureReason(L"Read", L"C:\\test.txt", 2);
    REQUIRE(result.find(L"Read") != std::wstring::npos);
    REQUIRE(result.find(L"C:\\test.txt") != std::wstring::npos);
    REQUIRE(result.find(L"error=2") != std::wstring::npos);
}

// ============================================================================
// LoadPendingUpdatePlan integration test using testdata file
// ============================================================================

TEST_CASE("LoadPendingUpdatePlan loads real plan file", "[plan][integration]")
{
    // Path relative to the unit_test build directory; adjust as needed
    std::wstring planFile = L"../testdata/文件名输出测试.json";

    PendingUpdatePlan plan;
    std::wstring failureReason;

    bool ok = LoadPendingUpdatePlan(planFile, plan, failureReason);
    CHECK(ok);
    if (!ok) {
        // Convert wstring failure reason to UTF-8 for display
        int len = WideCharToMultiByte(CP_UTF8, 0, failureReason.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string reason(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, failureReason.c_str(), -1, reason.data(), len, nullptr, nullptr);
        INFO("failureReason: " << reason);
    }
    REQUIRE(ok);

    CHECK(plan.packageType == L"ota");
    CHECK(plan.removeList.size() == 3);
    CHECK(plan.moveList.size() == 4);
}

TEST_CASE("LoadPendingUpdatePlan fails on missing file", "[plan][integration]")
{
    PendingUpdatePlan plan;
    std::wstring failureReason;

    bool ok = LoadPendingUpdatePlan(L"nonexistent_file.json", plan, failureReason);
    REQUIRE_FALSE(ok);
    REQUIRE_FALSE(failureReason.empty());
}
