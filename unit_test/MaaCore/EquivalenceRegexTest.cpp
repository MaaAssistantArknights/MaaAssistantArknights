#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "Vision/Config/OCREquivalenceRegex.hpp"

namespace
{
using EquivalenceClasses = std::vector<std::vector<std::string>>;

// A subset of the Japanese configuration, which is the one that still ships multi-member classes.
const EquivalenceClasses jp_classes {
    { "-", "ー", "一", "−" }, { "o", "O", "о" }, { "s", "S" }, { "a", "а" }, { "c", "с" },
};
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
    const auto expanded = asst::expand_equivalence_in_regex("[Oo]$", jp_classes);
    REQUIRE(expanded == "[Ooо]$");
    REQUIRE(expanded.find(':') == std::string::npos);
}
