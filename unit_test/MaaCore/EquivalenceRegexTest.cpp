#include <catch2/catch_test_macros.hpp>

#include <boost/regex.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "Vision/Config/OCREquivalenceRegex.hpp"

namespace
{
using EquivalenceClasses = std::vector<std::vector<std::string>>;

// A subset of the Japanese configuration, which is the one that still ships multi-member classes.
const EquivalenceClasses jp_classes {
    { "-", "ー", "一", "−" }, { "o", "O", "о" }, { "s", "S" }, { "a", "а" }, { "c", "с" },
};

std::wstring utf8_to_wstring(std::string_view text)
{
    std::wstring out;
    out.reserve(text.size());
    for (std::size_t index = 0; index < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[index]);
        char32_t code_point = 0;
        std::size_t length = 1;
        if (lead < 0x80) {
            code_point = lead;
        }
        else if ((lead & 0xE0) == 0xC0 && index + 1 < text.size()) {
            code_point =
                static_cast<char32_t>(((lead & 0x1F) << 6) | (static_cast<unsigned char>(text[index + 1]) & 0x3F));
            length = 2;
        }
        else if ((lead & 0xF0) == 0xE0 && index + 2 < text.size()) {
            code_point = static_cast<char32_t>(
                ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(text[index + 1]) & 0x3F) << 6) |
                (static_cast<unsigned char>(text[index + 2]) & 0x3F));
            length = 3;
        }
        else if ((lead & 0xF8) == 0xF0 && index + 3 < text.size()) {
            code_point = static_cast<char32_t>(
                ((lead & 0x07) << 18) | ((static_cast<unsigned char>(text[index + 1]) & 0x3F) << 12) |
                ((static_cast<unsigned char>(text[index + 2]) & 0x3F) << 6) |
                (static_cast<unsigned char>(text[index + 3]) & 0x3F));
            length = 4;
        }
        else {
            code_point = 0xFFFD;
        }

        if constexpr (sizeof(wchar_t) == 2) {
            if (code_point > 0xFFFF) {
                code_point -= 0x10000;
                out.push_back(static_cast<wchar_t>(0xD800 + (code_point >> 10)));
                out.push_back(static_cast<wchar_t>(0xDC00 + (code_point & 0x3FF)));
            }
            else {
                out.push_back(static_cast<wchar_t>(code_point));
            }
        }
        else {
            out.push_back(static_cast<wchar_t>(code_point));
        }
        index += length;
    }
    return out;
}

std::string wstring_to_utf8(std::wstring_view text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t index = 0; index < text.size();) {
        char32_t code_point = static_cast<char32_t>(text[index]);
        ++index;
        if constexpr (sizeof(wchar_t) == 2) {
            if (code_point >= 0xD800 && code_point <= 0xDBFF && index < text.size()) {
                const char32_t trail = static_cast<char32_t>(text[index]);
                if (trail >= 0xDC00 && trail <= 0xDFFF) {
                    code_point = 0x10000 + ((code_point - 0xD800) << 10) + (trail - 0xDC00);
                    ++index;
                }
            }
        }
        if (code_point < 0x80) {
            out.push_back(static_cast<char>(code_point));
        }
        else if (code_point < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
        else if (code_point < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
        else {
            out.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
    }
    return out;
}

boost::wregex compile_wregex(const std::string& pattern)
{
    return boost::wregex(utf8_to_wstring(pattern));
}

std::string regex_replace_utf8(const std::string& pattern, const std::string& text, const std::string& replacement)
{
    return wstring_to_utf8(
        boost::regex_replace(utf8_to_wstring(text), compile_wregex(pattern), utf8_to_wstring(replacement)));
}

bool regex_search_utf8(const std::string& pattern, const std::string& text)
{
    return boost::regex_search(utf8_to_wstring(text), compile_wregex(pattern));
}
} // namespace

TEST_CASE("Patterns without equivalent characters are not modified")
{
    const EquivalenceClasses classes {
        { "o", "O", "о" },
    };

    const std::string pattern = "^\\d+(小|分)$";
    REQUIRE(asst::expand_equivalence_in_regex(pattern, classes) == pattern);
}

TEST_CASE("Single member classes are ignored")
{
    const EquivalenceClasses classes {
        { "x" },
    };

    REQUIRE(asst::expand_equivalence_in_regex("x[xy]x", classes) == "x[xy]x");
}

TEST_CASE("Without classes the pattern is returned byte for byte")
{
    const std::string pattern = "^[A-Z]{2}(?:-|−)EX-\\d+[^)]*$";
    REQUIRE(asst::expand_equivalence_in_regex(pattern, {}) == pattern);
}

TEST_CASE("A match outside a character class becomes an alternation")
{
    REQUIRE(asst::expand_equivalence_in_regex("o", jp_classes) == "(?:o|O|о)");
    REQUIRE(asst::expand_equivalence_in_regex("s.", jp_classes) == "(?:s|S).");
}

TEST_CASE("Matches inside a character class are inlined as literals")
{
    const auto expanded = asst::expand_equivalence_in_regex("[Oo]", jp_classes);
    REQUIRE(expanded == "[Ooо]");
    REQUIRE(expanded.find('(') == std::string::npos);
    REQUIRE(expanded.find('|') == std::string::npos);
    REQUIRE(expanded.find(':') == std::string::npos);

    // The alternating member of the class is inlined once, not once per member of the expansion.
    REQUIRE(asst::expand_equivalence_in_regex("[CcOo0]", jp_classes) == "[CcсOoо0]");
}

TEST_CASE("Ranges are preserved")
{
    REQUIRE(asst::expand_equivalence_in_regex("[A-Z]", jp_classes) == "[A-Z]");
    REQUIRE(asst::expand_equivalence_in_regex("([A-Z])0(?=-)", jp_classes) == "([A-Z])0(?=(?:-|ー|一|−))");
}

TEST_CASE("Escape sequences are not expanded")
{
    // `\]` is a literal `]` inside the class, the `o` after it is still a member.
    REQUIRE(asst::expand_equivalence_in_regex("[\\]o]", jp_classes) == "[\\]oOо]");
    // `\s` is the whitespace class, not the member `s`.
    REQUIRE(asst::expand_equivalence_in_regex("\\s", jp_classes) == "\\s");
    REQUIRE(asst::expand_equivalence_in_regex("\\sc", jp_classes) == "\\s(?:c|с)");
}

TEST_CASE("Negative classes and a leading bracket stay intact")
{
    REQUIRE(asst::expand_equivalence_in_regex("[^Oo]", jp_classes) == "[^Ooо]");
    REQUIRE(asst::expand_equivalence_in_regex("[]]", jp_classes) == "[]]");
}

// https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/15084
TEST_CASE("A time string is no longer matched by the class of a number replacement")
{
    const auto expanded = asst::expand_equivalence_in_regex("[Oo]", jp_classes);
    REQUIRE(expanded == "[Ooо]");
    REQUIRE(expanded.find(':') == std::string::npos);

    REQUIRE(regex_replace_utf8(expanded, "03:53:4", "0") == "03:53:4");
    REQUIRE(regex_replace_utf8(expanded, "о3:53", "0") == "03:53");
}

TEST_CASE("Members must be a single Unicode scalar")
{
    using asst::equivalence_regex_detail::is_single_unicode_scalar;

    REQUIRE(is_single_unicode_scalar("o"));
    REQUIRE(is_single_unicode_scalar("ー"));
    REQUIRE(is_single_unicode_scalar("о"));
    REQUIRE(is_single_unicode_scalar("\u00A0"));
    REQUIRE_FALSE(is_single_unicode_scalar(""));
    REQUIRE_FALSE(is_single_unicode_scalar("ab"));
    REQUIRE_FALSE(is_single_unicode_scalar("ー-"));
    REQUIRE_FALSE(is_single_unicode_scalar("\xFF"));
}

TEST_CASE("Multi-scalar members are ignored instead of producing a short-first alternation")
{
    const EquivalenceClasses classes {
        { "a", "ab" },
    };

    // `"ab"` is not a single scalar, so it is not treated as a longest match. Expanding it as
    // `(?:a|ab)` would make boost::regex_replace("ab", ..., "X") yield "Xb".
    const auto expanded = asst::expand_equivalence_in_regex("ab", classes);
    REQUIRE(expanded == "(?:a)b");
    REQUIRE(regex_replace_utf8(expanded, "ab", "X") == "X");
}

TEST_CASE("POSIX bracket expressions are copied atomically")
{
    REQUIRE(asst::expand_equivalence_in_regex("[[:space:]]", jp_classes) == "[[:space:]]");
    REQUIRE(asst::expand_equivalence_in_regex("[[:alpha:]]", jp_classes) == "[[:alpha:]]");
    REQUIRE(asst::expand_equivalence_in_regex("[[.ch.]]", jp_classes) == "[[.ch.]]");
    REQUIRE(asst::expand_equivalence_in_regex("[[=ch=]]", jp_classes) == "[[=ch=]]");
    REQUIRE(asst::expand_equivalence_in_regex("[^[:space:]]", jp_classes) == "[^[:space:]]");
    REQUIRE(asst::expand_equivalence_in_regex("[[:space:]o]", jp_classes) == "[[:space:]oOо]");

    const auto space = asst::expand_equivalence_in_regex("[[:space:]]", jp_classes);
    REQUIRE_NOTHROW(compile_wregex(space));
    REQUIRE(regex_search_utf8(space, " "));
    REQUIRE_FALSE(regex_search_utf8(space, "s"));
    REQUIRE_FALSE(regex_search_utf8(space, "S"));
}

TEST_CASE("Boost extension prefixes are not rewritten")
{
    REQUIRE(asst::expand_equivalence_in_regex("(?s:.)", jp_classes) == "(?s:.)");
    REQUIRE(asst::expand_equivalence_in_regex("(?is:o)", jp_classes) == "(?is:(?:o|O|о))");
    REQUIRE(asst::expand_equivalence_in_regex("(?:s)", jp_classes) == "(?:(?:s|S))");

    const auto dotted = asst::expand_equivalence_in_regex("(?s:.)", jp_classes);
    REQUIRE_NOTHROW(compile_wregex(dotted));
    REQUIRE(regex_search_utf8(dotted, "\n"));
}

TEST_CASE("Quoted spans are not expanded")
{
    REQUIRE(asst::expand_equivalence_in_regex("\\Q[s]\\E", jp_classes) == "\\Q[s]\\E");
    REQUIRE(asst::expand_equivalence_in_regex("\\Qs\\E", jp_classes) == "\\Qs\\E");
    REQUIRE(asst::expand_equivalence_in_regex("\\Q[s]", jp_classes) == "\\Q[s]");

    const auto quoted = asst::expand_equivalence_in_regex("\\Q[s]\\E", jp_classes);
    REQUIRE_NOTHROW(compile_wregex(quoted));
    REQUIRE(regex_replace_utf8(quoted, "[s]", "X") == "X");
    REQUIRE(regex_replace_utf8(quoted, "[S]", "X") == "[S]");
    REQUIRE(regex_replace_utf8(quoted, "[sS]", "X") == "[sS]");
}
