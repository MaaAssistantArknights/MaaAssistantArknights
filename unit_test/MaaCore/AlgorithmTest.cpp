#include <catch2/catch_test_macros.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Utils/Algorithm.hpp"

namespace
{
using Tag = asst::battle::OperNameTag;
using Role = asst::battle::Role;

using GroupList = std::unordered_map<Tag, std::vector<Tag>>;
using CharSet = std::unordered_set<Tag>;

// 组名不带职业，职业未知表示该组不限定干员职业
Tag group(std::string name)
{
    return Tag { Role::Unknown, std::move(name) };
}

Tag oper(Role role, std::string name)
{
    return Tag { role, std::move(name) };
}

void require_valid_allocation(
    const GroupList& group_list,
    const CharSet& char_set,
    const asst::algorithm::CharAllocationResult& result)
{
    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation.size() == group_list.size());

    for (const auto& [group_name, allocated_char] : result.allocation) {
        INFO("group_name=" << group_name.to_string());
        REQUIRE(group_list.contains(group_name));
        REQUIRE(char_set.contains(allocated_char));
    }

    std::unordered_set<Tag> used_chars;
    for (const auto& [group_name, candidates] : group_list) {
        INFO("group_name=" << group_name.to_string());

        const auto allocation_it = result.allocation.find(group_name);
        REQUIRE(allocation_it != result.allocation.end());

        const auto& assigned_char = allocation_it->second;
        REQUIRE(char_set.contains(assigned_char));

        bool candidate_found = false;
        for (const auto& candidate : candidates) {
            if (candidate.name == assigned_char.name) {
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
    const CharSet chars { oper(Role::Pioneer, "Amiya") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(result.allocation.empty());
}

TEST_CASE("Empty char set returns no solution")
{
    const GroupList groups { { group("Pioneer"), { oper(Role::Pioneer, "Texas") } } };
    const CharSet chars;

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Exact matching returns expected allocation")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Texas") } },
        { group("Caster"), { oper(Role::Caster, "Amiya") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Texas"), oper(Role::Caster, "Amiya") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(
        result.allocation == std::unordered_map<Tag, Tag> {
                                 { group("Pioneer"), oper(Role::Pioneer, "Texas") },
                                 { group("Caster"), oper(Role::Caster, "Amiya") },
                             });
}

TEST_CASE("Duplicate candidates do not break matching")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Texas"), oper(Role::Pioneer, "Texas") } },
        { group("Caster"), { oper(Role::Caster, "Amiya"), oper(Role::Caster, "Amiya") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Texas"), oper(Role::Caster, "Amiya") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(
        result.allocation == std::unordered_map<Tag, Tag> {
                                 { group("Pioneer"), oper(Role::Pioneer, "Texas") },
                                 { group("Caster"), oper(Role::Caster, "Amiya") },
                             });
}

TEST_CASE("Unowned candidates are filtered before matching")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Fuldren"), oper(Role::Pioneer, "Texas") } },
        { group("Caster"), { oper(Role::Caster, "Ceobe"), oper(Role::Caster, "Amiya") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Texas"), oper(Role::Caster, "Amiya") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::Success);
    REQUIRE(result.has_value());
    REQUIRE(
        result.allocation == std::unordered_map<Tag, Tag> {
                                 { group("Pioneer"), oper(Role::Pioneer, "Texas") },
                                 { group("Caster"), oper(Role::Caster, "Amiya") },
                             });
}

TEST_CASE("Conflicting groups return no solution")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Ursus") } },
        { group("Warrior"), { oper(Role::Warrior, "Ursus") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Ursus") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Multiple groups can find a valid allocation")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Texas"), oper(Role::Pioneer, "Myrrh") } },
        { group("Caster"), { oper(Role::Caster, "Amiya"), oper(Role::Caster, "Ifrit") } },
        { group("Medic"), { oper(Role::Medic, "Shining"), oper(Role::Medic, "Nightmare") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Myrrh"), oper(Role::Caster, "Amiya"), oper(Role::Medic, "Shining") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    require_valid_allocation(groups, chars, result);
}

TEST_CASE("Multiple groups allocation ignores extra owned chars")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Texas"), oper(Role::Pioneer, "Myrrh") } },
        { group("Caster"), { oper(Role::Caster, "Amiya"), oper(Role::Caster, "Ifrit") } },
        { group("Medic"), { oper(Role::Medic, "Shining"), oper(Role::Medic, "Nightmare") } },
    };
    const CharSet chars {
        oper(Role::Pioneer, "Myrrh"),  oper(Role::Caster, "Amiya"), oper(Role::Medic, "Shining"),
        oper(Role::Pioneer, "Texas"),  oper(Role::Caster, "Ifrit"), oper(Role::Medic, "Nightmare"),
        oper(Role::Sniper, "Exusiai"),
    };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    require_valid_allocation(groups, chars, result);
}

TEST_CASE("Group without any owned candidate returns no solution")
{
    const GroupList groups {
        { group("Pioneer"), { oper(Role::Pioneer, "Texas") } },
        { group("Caster"), { oper(Role::Caster, "Amiya") } },
    };
    const CharSet chars { oper(Role::Pioneer, "Texas") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Group with empty candidate list returns no solution")
{
    const GroupList groups {
        { group("Pioneer"), {} },
        { group("Caster"), { oper(Role::Caster, "Amiya") } },
    };
    const CharSet chars { oper(Role::Caster, "Amiya") };

    const auto result = asst::algorithm::get_char_allocation_for_each_group(groups, chars);

    REQUIRE(result.status == asst::algorithm::CharAllocationStatus::NoSolution);
    REQUIRE_FALSE(result.has_value());
}
