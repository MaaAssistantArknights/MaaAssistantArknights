#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include "MaterialCraftPlanner.h"

namespace asst::infrast
{
struct ProcessingOperatorScore
{
    int bonus_percent = 0;
    int mood_cost = 0;
    bool refunds_mood = false;
    bool probability_is_lower_bound = false;

    double byproduct_probability() const noexcept { return 0.1 * (1.0 + bonus_percent / 100.0); }

    bool better_than(const ProcessingOperatorScore& other) const noexcept
    {
        return bonus_percent > other.bonus_percent ||
               (bonus_percent == other.bonus_percent && mood_cost < other.mood_cost);
    }
};

bool is_excluded_processing_operator(std::string_view operator_id);

// Only the currently recognized skills are input; operator identity never supplies an unlocked skill.
// nullopt means unsupported recipe, excluded operator, or no effect on this recipe.
std::optional<ProcessingOperatorScore> score_skill_summary(
    const std::unordered_set<std::string>& skills,
    std::string_view operator_id,
    std::string_view item_id,
    std::optional<bool> stainless_in_dorm = std::nullopt);

// Elite material conditions use the recipe's original ap_cost, before any skill adjustments.
std::optional<ProcessingOperatorScore> score_processing_operator(
    const std::unordered_set<std::string>& skills,
    std::string_view operator_id,
    const MaterialFormula& formula,
    std::optional<bool> stainless_in_dorm = std::nullopt);
}
