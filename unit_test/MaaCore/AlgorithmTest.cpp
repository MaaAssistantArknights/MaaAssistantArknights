#include <catch2/catch_test_macros.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Utils/Algorithm.hpp"

namespace
{
using GroupList = std::unordered_map<std::string, std::vector<std::string>>;
using CharSet = std::unordered_set<std::string>;

void require_valid_allocation(const GroupList& group_list, const CharSet& char_set,
                              const asst::algorithm::CharAllocationResult& result)
{
    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation.size() == group_list.size());

    for (const auto& [group_name, allocated_char] : result.allocation) {
        INFO("group_name=" << group_name);
        REQUIRE(group_list.contains(group_name));
        REQUIRE(char_set.contains(allocated_char));
    }

    std::unordered_set<std::string> used_chars;
    for (const auto& [group_name, candidates] : group_list) {
        INFO("group_name=" << group_name);

        const auto allocation_it = result.allocation.find(group_name);
        REQUIRE(allocation_it != result.allocation.end());

        const auto& assigned_char = allocation_it->second;
        REQUIRE(char_set.contains(assigned_char));

        bool candidate_found = false;
        for (const auto& candidate : candidates) {
            if (candidate == assigned_char) {
                candidate_found = true;
                break;
            }
        }
        REQUIRE(candidate_found);
        REQUIRE(used_chars.emplace(assigned_char).second);
    }
}
} // namespace

TEST_CASE("Empty group list returns empty success result")
{
    const GroupList groups;
    const CharSet chars { "Amiya" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation.empty());
}

TEST_CASE("Empty char set returns no solution")
{
    const GroupList groups { { "先锋", { "德克萨斯" } } };
    const CharSet chars;

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Exact matching returns expected allocation")
{
    const GroupList groups {
        { "先锋", { "德克萨斯" } },
        { "术师", { "阿米娅" } },
    };
    const CharSet chars { "德克萨斯", "阿米娅" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation == std::unordered_map<std::string, std::string> {
                                   { "先锋", "德克萨斯" },
                                   { "术师", "阿米娅" },
                               });
}

TEST_CASE("Duplicate candidates do not break matching")
{
    const GroupList groups {
        { "先锋", { "德克萨斯", "德克萨斯" } },
        { "术师", { "阿米娅", "阿米娅" } },
    };
    const CharSet chars { "德克萨斯", "阿米娅" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation == std::unordered_map<std::string, std::string> {
                                   { "先锋", "德克萨斯" },
                                   { "术师", "阿米娅" },
                               });
}

TEST_CASE("Unowned candidates are filtered before matching")
{
    const GroupList groups {
        { "先锋", { "风笛", "德克萨斯" } },
        { "术师", { "刻俄柏", "阿米娅" } },
    };
    const CharSet chars { "德克萨斯", "阿米娅" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation == std::unordered_map<std::string, std::string> {
                                   { "先锋", "德克萨斯" },
                                   { "术师", "阿米娅" },
                               });
}

TEST_CASE("Conflicting groups return no solution")
{
    const GroupList groups {
        { "先锋", { "推进之王" } },
        { "近卫", { "推进之王" } },
    };
    const CharSet chars { "推进之王" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Multiple groups can find a valid allocation")
{
    const GroupList groups {
        { "先锋", { "德克萨斯", "桃金娘" } },
        { "术师", { "阿米娅", "伊芙利特" } },
        { "医疗", { "闪灵", "夜莺" } },
    };
    const CharSet chars { "桃金娘", "阿米娅", "夜莺" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    require_valid_allocation(groups, chars, result);
}

TEST_CASE("Multiple groups allocation ignores extra owned chars")
{
    const GroupList groups {
        { "先锋", { "德克萨斯", "桃金娘" } },
        { "术师", { "阿米娅", "伊芙利特" } },
        { "医疗", { "闪灵", "夜莺" } },
    };
    const CharSet chars {
        "桃金娘",
        "阿米娅",
        "夜莺",
        "德克萨斯",
        "伊芙利特",
        "闪灵",
        "能天使",
    };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    require_valid_allocation(groups, chars, result);
}

TEST_CASE("Group without any owned candidate returns no solution")
{
    const GroupList groups {
        { "先锋", { "德克萨斯" } },
        { "术师", { "阿米娅" } },
    };
    const CharSet chars { "德克萨斯" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Group with empty candidate list returns no solution")
{
    const GroupList groups {
        { "先锋", {} },
        { "术师", { "阿米娅" } },
    };
    const CharSet chars { "阿米娅" };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

namespace
{
using EquivalenceClasses = std::vector<std::vector<std::string>>;
}

TEST_CASE("inject_equivalence_classes leaves patterns unchanged when there are no classes")
{
    const EquivalenceClasses eq_classes;
    REQUIRE(asst::algorithm::inject_equivalence_classes("[Oo]", eq_classes) == "[Oo]");
    REQUIRE(asst::algorithm::inject_equivalence_classes("abc", eq_classes) == "abc");
}

TEST_CASE("inject_equivalence_classes ignores single-member classes")
{
    const EquivalenceClasses eq_classes { { "o" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes("o", eq_classes) == "o");
}

TEST_CASE("inject_equivalence_classes expands members outside a character class")
{
    const EquivalenceClasses eq_classes { { "o", "0" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes("o", eq_classes) == "(?:o|0)");
    REQUIRE(asst::algorithm::inject_equivalence_classes("xoy", eq_classes) == "x(?:o|0)y");
}

TEST_CASE("inject_equivalence_classes keeps character classes valid")
{
    // Regression test for issue #15084: injecting into "[Oo]" must not produce
    // "[O(?:o|0)]", which would also match '(', '?', ':', '|' and ')' and therefore
    // turn a time string like "12:34:56" into "12034056".
    const EquivalenceClasses eq_classes { { "o", "0" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes("[Oo]", eq_classes) == "[Oo0]");
}

TEST_CASE("inject_equivalence_classes preserves backslash escapes")
{
    const EquivalenceClasses eq_classes { { "o", "0" } };
    // An escaped ']' must not close the character class, and the escaped character is
    // copied verbatim instead of being expanded.
    REQUIRE(asst::algorithm::inject_equivalence_classes("[\\]o]", eq_classes) == "[\\]o0]");
}

TEST_CASE("inject_equivalence_classes handles multi-codepoint space classes")
{
    // Mirrors the KR/JP resources, whose equivalence class holds two space variants
    // (U+0020 and the ideographic space U+3000, encoded as \xe3\x80\x80 in UTF-8).
    const EquivalenceClasses eq_classes { { " ", "\xe3\x80\x80" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes(" ", eq_classes) == "(?: |\xe3\x80\x80)");
    REQUIRE(asst::algorithm::inject_equivalence_classes("[ a]", eq_classes) == "[ \xe3\x80\x80""a]");
}

TEST_CASE("inject_equivalence_classes handles negated character classes")
{
    // '^' right after '[' is the negation marker and must be preserved, while the
    // members are still inserted as plain class atoms.
    const EquivalenceClasses eq_classes { { "o", "0" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes("[^o]", eq_classes) == "[^o0]");
}

TEST_CASE("inject_equivalence_classes handles multiple equivalence classes")
{
    const EquivalenceClasses eq_classes { { "o", "0" }, { "l", "1" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes("lol", eq_classes) == "(?:l|1)(?:o|0)(?:l|1)");
    REQUIRE(asst::algorithm::inject_equivalence_classes("[ol]", eq_classes) == "[o0l1]");
}

TEST_CASE("inject_equivalence_classes escapes regex-special members")
{
    // Members are treated as literal characters even when they are regex metacharacters,
    // both outside and inside a character class.
    const EquivalenceClasses eq_classes { { ".", "x" } };
    REQUIRE(asst::algorithm::inject_equivalence_classes(".", eq_classes) == "(?:\\.|x)");
    REQUIRE(asst::algorithm::inject_equivalence_classes("[.]", eq_classes) == "[.x]");
}
