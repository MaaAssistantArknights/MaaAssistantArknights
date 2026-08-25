#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_set>

#include <meojson/json.hpp>

#include "OperatorDevelopmentPlan.h"

namespace asst
{
inline std::optional<int> read_operator_development_integer(const json::value& value, std::string_view key)
{
    const auto found = value.find(std::string(key));
    if (!found || !found->is_number()) {
        return std::nullopt;
    }
    const double number = found->as_double();
    if (!std::isfinite(number) || std::trunc(number) != number || number < std::numeric_limits<int>::min() ||
        number > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return static_cast<int>(number);
}

inline std::optional<OperatorDevelopmentPlan> parse_operator_development_plan(const json::value& params)
{
    constexpr std::string_view Name = "name";
    constexpr std::string_view Elite = "elite";
    constexpr std::string_view Skills = "skills";
    constexpr std::string_view Skill = "skill";
    constexpr std::string_view SkillMaster = "skill_master";

    const auto plans = params.find<json::array>("plans");
    if (!plans) {
        return std::nullopt;
    }

    const std::unordered_set<std::string_view> allowed { Name, Elite, Skills, Skill, SkillMaster };
    OperatorDevelopmentPlan result;
    result.reserve(plans->size());
    for (const auto& value : *plans) {
        if (!value.is_object()) {
            return std::nullopt;
        }
        for (const auto& [key, ignored] : value.as_object()) {
            (void)ignored;
            if (!allowed.contains(key)) {
                return std::nullopt;
            }
        }

        const auto name = value.find<std::string>("name");
        if (!name || name->empty() || std::ranges::all_of(*name, [](unsigned char ch) { return std::isspace(ch); })) {
            return std::nullopt;
        }
        const bool has_elite = value.as_object().contains("elite");
        const bool has_skills = value.as_object().contains("skills");
        const bool has_skill = value.as_object().contains("skill");
        const bool has_mastery = value.as_object().contains("skill_master");
        const int actions =
            static_cast<int>(has_elite) + static_cast<int>(has_skills) + static_cast<int>(has_skill || has_mastery);
        if (actions != 1 || has_skill != has_mastery) {
            return std::nullopt;
        }

        OperatorDevelopmentTarget target { .name = *name };
        if (has_elite) {
            const auto level = read_operator_development_integer(value, Elite);
            if (!level || *level < 1 || *level > 2) {
                return std::nullopt;
            }
            target.action = OperatorDevelopmentAction::Elite;
            target.target = *level;
        }
        else if (has_skills) {
            const auto level = read_operator_development_integer(value, Skills);
            if (!level || *level < 2 || *level > 7) {
                return std::nullopt;
            }
            target.action = OperatorDevelopmentAction::Skills;
            target.target = *level;
        }
        else {
            const auto skill = read_operator_development_integer(value, Skill);
            const auto level = read_operator_development_integer(value, SkillMaster);
            if (!skill || !level || *skill < 1 || *skill > 3 || *level < 1 || *level > 3) {
                return std::nullopt;
            }
            target.action = OperatorDevelopmentAction::Mastery;
            target.skill = *skill;
            target.target = *level;
        }
        result.emplace_back(std::move(target));
    }
    return result;
}
} // namespace asst
