#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include "Utils/StringMisc.hpp"

using asst::utils::chars_to_number;
using asst::utils::string_replace_all;
using asst::utils::string_replace_all_in_place;

// ============================================================================
// string_replace_all_in_place
// ============================================================================

TEST_CASE("replace_all replaces single occurrence")
{
    std::string s = "hello world";
    string_replace_all_in_place<std::string>(s, "world", "there");
    REQUIRE(s == "hello there");
}

TEST_CASE("replace_all replaces multiple non-overlapping occurrences")
{
    std::string s = "foo bar foo baz foo";
    string_replace_all_in_place<std::string>(s, "foo", "X");
    REQUIRE(s == "X bar X baz X");
}

TEST_CASE("replace_all is a no-op when pattern is not found")
{
    std::string s = "hello";
    string_replace_all_in_place<std::string>(s, "xyz", "abc");
    REQUIRE(s == "hello");
}

TEST_CASE("replace_all on empty string produces empty string")
{
    std::string s;
    string_replace_all_in_place<std::string>(s, "a", "b");
    REQUIRE(s.empty());
}

TEST_CASE("replace_all terminates when replacement contains the pattern")
{
    // Classic footgun: if the loop forgets to skip past `to`, replacing
    // "a" with "aa" in "a" would re-find "a" at the same position and
    // loop forever.
    std::string s = "a";
    string_replace_all_in_place<std::string>(s, "a", "aa");
    REQUIRE(s == "aa");
}

TEST_CASE("replace_all expands string correctly when to-contains-from")
{
    std::string s = "aaa";
    string_replace_all_in_place<std::string>(s, "a", "aa");
    REQUIRE(s == "aaaaaa"); // 3 x 'a' -> 3 x 'aa' = 6 'a's
}

TEST_CASE("replace_all with overlapping pattern consumes greedily left-to-right")
{
    // "aaa" contains "aa" starting at pos 0 and pos 1; greedy left-to-right
    // match at pos 0 replaces with "b", leaving "ba". Searching from pos 1
    // finds no more "aa".
    std::string s = "aaa";
    string_replace_all_in_place<std::string>(s, "aa", "b");
    REQUIRE(s == "ba");
}

TEST_CASE("replace_all with shrinking pattern skips correctly")
{
    std::string s = "xxxxx";
    string_replace_all_in_place<std::string>(s, "xxx", "y");
    // "xxx" at pos 0 -> "yxx". Advance pos by 1 (length of "y"). Remaining
    // "xx" has no "xxx" match. Final result: "yxx".
    REQUIRE(s == "yxx");
}

TEST_CASE("replace_all works on std::wstring with multi-byte characters")
{
    std::wstring s = L"你好世界你好";
    string_replace_all_in_place<std::wstring>(s, L"你好", L"再见");
    REQUIRE(s == L"再见世界再见");
}

TEST_CASE("non-in-place replace_all returns new string without mutating source")
{
    const std::string src = "hello world";
    // Deliberately let template argument deduction pick up `const std::string&`
    // as StringT so the source is copied rather than moved. Explicitly
    // specifying `<std::string>` would turn the parameter into an rvalue
    // reference and reject the const lvalue.
    std::string_view from { "world" };
    std::string_view to { "there" };
    auto result = string_replace_all(src, from, to);
    REQUIRE(result == "hello there");
    REQUIRE(src == "hello world");
}

TEST_CASE("replace_all with initializer_list applies all replacements")
{
    std::string s = "ab";
    string_replace_all_in_place<std::string>(s, { { "a", "x" }, { "b", "y" } });
    REQUIRE(s == "xy");
}

TEST_CASE("replace_all initializer_list applies left-to-right, later rules see earlier results")
{
    // First rule: a -> b. String becomes "b".
    // Second rule: b -> c. String becomes "c".
    // If rules were applied independently to the original string, we'd get "b".
    std::string s = "a";
    string_replace_all_in_place<std::string>(s, { { "a", "b" }, { "b", "c" } });
    REQUIRE(s == "c");
}

// ============================================================================
// chars_to_number
// ============================================================================

TEST_CASE("chars_to_number parses a simple positive integer")
{
    int result = 0;
    REQUIRE(chars_to_number("42", result));
    REQUIRE(result == 42);
}

TEST_CASE("chars_to_number parses negative integer")
{
    int result = 0;
    REQUIRE(chars_to_number("-42", result));
    REQUIRE(result == -42);
}

TEST_CASE("chars_to_number parses zero")
{
    int result = 99;
    REQUIRE(chars_to_number("0", result));
    REQUIRE(result == 0);
}

TEST_CASE("chars_to_number fails on empty string")
{
    int result = 99;
    REQUIRE_FALSE(chars_to_number("", result));
    // result unchanged per std::from_chars contract; do not assert its value.
}

TEST_CASE("chars_to_number fails on non-numeric input")
{
    int result = 99;
    REQUIRE_FALSE(chars_to_number("abc", result));
}

TEST_CASE("chars_to_number does NOT skip leading whitespace (std::from_chars contract)")
{
    int result = 99;
    REQUIRE_FALSE(chars_to_number("  42", result));
}

TEST_CASE("chars_to_number with default FullMatch=false allows trailing garbage")
{
    int result = 0;
    REQUIRE(chars_to_number("42abc", result));
    REQUIRE(result == 42);
}

TEST_CASE("chars_to_number with default FullMatch=false allows trailing whitespace")
{
    int result = 0;
    REQUIRE(chars_to_number("42 ", result));
    REQUIRE(result == 42);
}

TEST_CASE("chars_to_number with FullMatch=true rejects trailing garbage")
{
    int result = 0;
    const bool ok = chars_to_number<int, true>("42abc", result);
    REQUIRE_FALSE(ok);
}

TEST_CASE("chars_to_number with FullMatch=true rejects trailing whitespace")
{
    int result = 0;
    const bool ok = chars_to_number<int, true>("42 ", result);
    REQUIRE_FALSE(ok);
}

TEST_CASE("chars_to_number with FullMatch=true accepts a pure integer")
{
    int result = 0;
    const bool ok = chars_to_number<int, true>("42", result);
    REQUIRE(ok);
    REQUIRE(result == 42);
}

TEST_CASE("chars_to_number reports out-of-range for int8_t overflow")
{
    int8_t result = 0;
    REQUIRE_FALSE(chars_to_number<int8_t>("1000", result));
}

TEST_CASE("chars_to_number accepts int8_t boundary value")
{
    int8_t result = 0;
    REQUIRE(chars_to_number<int8_t>("127", result));
    REQUIRE(result == 127);
}

TEST_CASE("chars_to_number parses large uint64_t value")
{
    uint64_t result = 0;
    REQUIRE(chars_to_number<uint64_t>("9999999999", result));
    REQUIRE(result == 9999999999ULL);
}

TEST_CASE("chars_to_number parses a double")
{
    double result = 0.0;
    REQUIRE(chars_to_number<double>("3.14", result));
    REQUIRE(result == 3.14);
}

TEST_CASE("chars_to_number parses scientific notation into double")
{
    double result = 0.0;
    REQUIRE(chars_to_number<double>("1e3", result));
    REQUIRE(result == 1000.0);
}
