#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "Vision/Config/OCREquivalenceTraits.hpp"

namespace
{
using EquivalenceClasses = std::vector<std::vector<std::string>>;

// A subset of the Japanese configuration, which is the one that still ships multi-member classes.
const EquivalenceClasses jp_classes {
    { "-", "ー", "一", "−" }, { "o", "O", "о" }, { "s", "S" }, { "a", "а" }, { "c", "с" },
};

const EquivalenceClasses kr_classes {
    { " ", "\u00A0" },
};

struct EquivalenceGuard
{
    explicit EquivalenceGuard(const EquivalenceClasses& classes) { asst::OcrEquivalenceTraits::set_classes(classes); }

    ~EquivalenceGuard() { asst::OcrEquivalenceTraits::set_classes({}); }
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

std::string regex_replace_utf8(const std::string& pattern, const std::string& text, const std::string& replacement)
{
    const auto& re = asst::ocr_eq_regex(utf8_to_wstring(pattern));
    return wstring_to_utf8(boost::regex_replace(utf8_to_wstring(text), re, utf8_to_wstring(replacement)));
}

bool regex_search_utf8(const std::string& pattern, const std::string& text)
{
    return boost::regex_search(utf8_to_wstring(text), asst::ocr_eq_regex(utf8_to_wstring(pattern)));
}
} // namespace

TEST_CASE("Members must be a single Unicode scalar")
{
    REQUIRE(asst::is_single_unicode_scalar("o"));
    REQUIRE(asst::is_single_unicode_scalar("ー"));
    REQUIRE(asst::is_single_unicode_scalar("о"));
    REQUIRE(asst::is_single_unicode_scalar("\u00A0"));
    REQUIRE_FALSE(asst::is_single_unicode_scalar(""));
    REQUIRE_FALSE(asst::is_single_unicode_scalar("ab"));
    REQUIRE_FALSE(asst::is_single_unicode_scalar("ー-"));
    REQUIRE_FALSE(asst::is_single_unicode_scalar("\xFF"));

    REQUIRE(asst::utf8_scalar_to_wchar("o").has_value());
    REQUIRE_FALSE(asst::utf8_scalar_to_wchar("ab").has_value());
}

TEST_CASE("A time string is no longer matched by the class of a number replacement")
{
    // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/15084
    const EquivalenceGuard guard(jp_classes);

    REQUIRE(regex_replace_utf8("[Oo]", "03:53:4", "0") == "03:53:4");
    REQUIRE(regex_replace_utf8("[Oo]", "о3:53", "0") == "03:53");
    REQUIRE(regex_replace_utf8("[Oo]", "O3", "0") == "03");
}

TEST_CASE("Literals and class members follow the equivalence class")
{
    const EquivalenceGuard guard(jp_classes);

    REQUIRE(regex_search_utf8("o", "o"));
    REQUIRE(regex_search_utf8("o", "O"));
    REQUIRE(regex_search_utf8("o", "о"));
    REQUIRE(regex_search_utf8("[Oo]", "о"));
    REQUIRE(regex_search_utf8("A[Ss].", "Aso"));
    REQUIRE(regex_search_utf8("A[Ss].", "ASo"));
}

TEST_CASE("Katakana long-vowel mark is equivalent to ASCII hyphen")
{
    const EquivalenceGuard guard(jp_classes);

    REQUIRE(regex_search_utf8("アーミヤ", "アーミヤ"));
    REQUIRE(regex_search_utf8("アーミヤ", "ア-ミヤ"));
    REQUIRE(regex_search_utf8("アーミヤ", "ア一ミヤ"));
}

TEST_CASE("Named POSIX classes stay intact and are equivalence-aware")
{
    const EquivalenceGuard guard(jp_classes);

    REQUIRE_NOTHROW(asst::ocr_eq_regex(L"[[:space:]]"));
    REQUIRE(regex_search_utf8("[[:space:]]", " "));
    REQUIRE_FALSE(regex_search_utf8("[[:space:]]", "s"));
    REQUIRE_FALSE(regex_search_utf8("[[:space:]]", "S"));

    REQUIRE_NOTHROW(asst::ocr_eq_regex(L"(?s:.)"));
    REQUIRE(regex_search_utf8("(?s:.)", "\n"));
}

TEST_CASE("Stage-name named class recovers S0-1 after the range rewrite")
{
    const EquivalenceGuard guard(jp_classes);

    REQUIRE(regex_replace_utf8("([[:upper:]])0(?=-)", "S0-1", "${1}O") == "SO-1");
    REQUIRE(regex_replace_utf8("([[:upper:]])0(?=-)", "O0-1", "${1}O") == "OO-1");
    REQUIRE(regex_replace_utf8("([[:upper:]])0(?=-)", "A0-1", "${1}O") == "AO-1");
    REQUIRE(regex_replace_utf8(".*?([[:upper:]]{2}-EX-\\d+)", "fooOS-EX-1bar", "$1") == "OS-EX-1bar");
    REQUIRE(regex_replace_utf8(".*?([[:upper:]]{2}-EX-\\d+)", "fooCE-EX-1bar", "$1") == "CE-EX-1bar");
}

TEST_CASE("Raw [A-Z] ranges stay unaware of mixed-case classes")
{
    const EquivalenceGuard guard(jp_classes);

    // This is why resources rewrite [A-Z] to [[:upper:]]. Traits translate range endpoints, so S
    // collapses to s and falls out of [A-Z].
    REQUIRE(regex_replace_utf8("([A-Z])0(?=-)", "S0-1", "${1}O") == "S0-1");
    REQUIRE(regex_replace_utf8("([A-Z])0(?=-)", "A0-1", "${1}O") == "AO-1");
}

TEST_CASE("isctype is equivalence-aware for named classes")
{
    SECTION("A class with an uppercase member is accepted by [[:upper:]]")
    {
        const EquivalenceGuard guard(EquivalenceClasses { { "s", "S" } });
        REQUIRE(regex_search_utf8("[[:upper:]]", "S"));
        REQUIRE(regex_search_utf8("[[:upper:]]", "s"));
        REQUIRE(regex_search_utf8("[[:lower:]]", "S"));
        REQUIRE(regex_search_utf8("[[:lower:]]", "s"));
    }

    SECTION("A class without an uppercase member is rejected by [[:upper:]]")
    {
        // Analogue of (Medic, "阿米娅"): 阿 has no uppercase equivalent, Latin a/а are both lower.
        const EquivalenceGuard guard(EquivalenceClasses { { "a", "а" } });
        REQUIRE_FALSE(regex_search_utf8("[[:upper:]]", "a"));
        REQUIRE_FALSE(regex_search_utf8("[[:upper:]]", "а"));
        REQUIRE_FALSE(regex_search_utf8("[[:upper:]]", "阿"));
        REQUIRE_FALSE(regex_search_utf8("[[:upper:]]", "米"));
        REQUIRE(regex_search_utf8("[[:lower:]]", "a"));
        REQUIRE(regex_search_utf8("[[:lower:]]", "а"));
    }

    SECTION("[[:alpha:]] follows the class, [[:space:]] does not leak s/S")
    {
        const EquivalenceGuard guard(jp_classes);
        REQUIRE(regex_search_utf8("[[:alpha:]]", "о"));
        REQUIRE_FALSE(regex_search_utf8("[[:space:]]", "s"));
    }
}

TEST_CASE("Quoted spans are translated, unlike the former expander")
{
    const EquivalenceGuard guard(jp_classes);

    REQUIRE(regex_replace_utf8("\\Q[s]\\E", "[s]", "X") == "X");
    // Documented semantic change: \\Q...\\E literals also go through translate, so [S] matches.
    REQUIRE(regex_replace_utf8("\\Q[s]\\E", "[S]", "X") == "X");
    REQUIRE(regex_replace_utf8("\\Q[s]\\E", "[sS]", "X") == "[sS]");
}

TEST_CASE("Regex cache is dropped when the client equivalence table changes")
{
    {
        const EquivalenceGuard jp(jp_classes);
        REQUIRE(regex_search_utf8("[Oo]", "о"));
        REQUIRE(regex_replace_utf8("[Oo]", "о3", "0") == "03");
    }

    // Guard destructor cleared the table. The same pattern text must not keep the JP compilation.
    REQUIRE_FALSE(regex_search_utf8("[Oo]", "о"));
    REQUIRE(regex_replace_utf8("[Oo]", "о3", "0") == "о3");
    REQUIRE(regex_replace_utf8("[Oo]", "O3", "0") == "03");

    {
        const EquivalenceGuard kr(kr_classes);
        REQUIRE(regex_replace_utf8(" ", "a\u00A0b", "X") == "aXb");
    }
}

TEST_CASE("Multi-scalar members are ignored instead of producing a short-first alternation")
{
    const EquivalenceGuard guard(EquivalenceClasses { { "a", "ab" } });

    // `"ab"` is not a single scalar, so it is not entered into the traits table. Matching stays on
    // the single-character member `a`; replacing in "ab" therefore yields "Xb", not a two-character
    // match of "ab".
    REQUIRE(regex_replace_utf8("a", "ab", "X") == "Xb");
    REQUIRE_FALSE(regex_search_utf8("ab", "a"));
}

TEST_CASE("Single member classes are ignored")
{
    const EquivalenceGuard guard(EquivalenceClasses { { "x" } });
    REQUIRE(regex_search_utf8("x", "x"));
    REQUIRE_FALSE(regex_search_utf8("x", "X"));
}

TEST_CASE("KR non-breaking space is equivalent to ASCII space")
{
    const EquivalenceGuard guard(kr_classes);
    REQUIRE(regex_search_utf8(" ", "\u00A0"));
    REQUIRE(regex_replace_utf8(" ", "a\u00A0b", "-") == "a-b");
}
