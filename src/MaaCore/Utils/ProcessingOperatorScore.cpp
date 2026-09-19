#include "ProcessingOperatorScore.h"

#include <algorithm>
#include <array>
#include <utility>

namespace asst::infrast
{
namespace
{
bool excluded_skills(const std::unordered_set<std::string>& skills)
{
    // Exclude these even when the skill icon is known but operator identity is unavailable.
    return skills.contains("bskill_ws_bonus1") || skills.contains("bskill_ws_bonus2") ||
           skills.contains("bskill_ws_frost") || skills.contains("bskill_ws_evolve_dorm1") ||
           skills.contains("bskill_ws_evolve_dorm2") || skills.contains("bskill_ws_evolve_dorm3") ||
           skills.contains("bskill_ws_cost&dorm");
}

ProcessingOperatorScore general_score(
    const std::unordered_set<std::string>& skills,
    int original_mood,
    std::optional<bool> stainless_in_dorm)
{
    static constexpr std::array<std::pair<std::string_view, int>, 7> ProbabilitySkills = { {
        { "bskill_ws_p1", 40 },
        { "bskill_ws_p2", 50 },
        { "bskill_ws_p3", 60 },
        { "bskill_ws_p4", 65 },
        { "bskill_ws_p5", 70 },
        { "bskill_ws_p7", 50 },
        { "bskill_ws_p_kalts2", 80 },
    } };
    ProcessingOperatorScore result;
    result.mood_cost = original_mood;
    for (const auto& [skill, bonus] : ProbabilitySkills) {
        if (skills.contains(std::string(skill))) {
            // Upgrade variants replace each other; they do not stack.
            result.bonus_percent = std::max(result.bonus_percent, bonus);
        }
    }
    if (skills.contains("bskill_ws_p7")) {
        result.probability_is_lower_bound = !stainless_in_dorm.has_value();
        if (stainless_in_dorm.value_or(false)) {
            result.bonus_percent = std::max(result.bonus_percent, 60);
        }
    }
    result.refunds_mood = skills.contains("bskill_ws_recovery");
    return result;
}
}

bool is_excluded_processing_operator(std::string_view operator_id)
{
    return operator_id == "char_4019_ncdeer" || operator_id == "char_271_spikes" || operator_id == "char_4072_ironmn" ||
           operator_id == "char_458_rfrost";
}

std::optional<ProcessingOperatorScore> score_skill_summary(
    const std::unordered_set<std::string>& skills,
    std::string_view operator_id,
    std::string_view item_id,
    std::optional<bool> stainless_in_dorm)
{
    if ((item_id != "3302" && item_id != "3303") || is_excluded_processing_operator(operator_id) ||
        excluded_skills(skills)) {
        return std::nullopt;
    }

    static constexpr std::array<std::pair<std::string_view, int>, 3> ProbabilitySkills = { {
        { "bskill_ws_skill1", 70 },
        { "bskill_ws_skill2", 75 },
        { "bskill_ws_skill3", 80 },
    } };

    const int original_mood = item_id == "3302" ? 1 : 2;
    auto result = general_score(skills, original_mood, stainless_in_dorm);
    for (const auto& [skill, bonus] : ProbabilitySkills) {
        if (skills.contains(std::string(skill))) {
            // Upgrade variants are alternatives, not additive probability bonuses.
            result.bonus_percent = std::max(result.bonus_percent, bonus);
        }
    }
    if (original_mood == 2 && (skills.contains("bskill_ws_skill_cost1") || skills.contains("bskill_ws_skill_cost2") ||
                               skills.contains("bskill_ws_all_cost2"))) {
        result.mood_cost = 1;
    }
    if (result.bonus_percent == 0 && result.mood_cost == original_mood) {
        return std::nullopt;
    }
    return result;
}

std::optional<ProcessingOperatorScore> score_processing_operator(
    const std::unordered_set<std::string>& skills,
    std::string_view operator_id,
    const MaterialFormula& formula,
    std::optional<bool> stainless_in_dorm)
{
    if (formula.facility != "Processing" || is_excluded_processing_operator(operator_id) || excluded_skills(skills)) {
        return std::nullopt;
    }
    if (formula.is_skill_summary()) {
        return score_skill_summary(skills, operator_id, formula.item_id, stainless_in_dorm);
    }
    // Do not infer a recipe's category or round a fractional mood cost.
    constexpr int MoodUnit = 360'000;
    if (formula.buff_type != "W_EVOLVE" || formula.ap_cost <= 0 || formula.ap_cost % MoodUnit != 0) {
        return std::nullopt;
    }
    const int original_mood = formula.ap_cost / MoodUnit;
    const auto& item = formula.item_id;
    auto has = [&](const char* skill) {
        return skills.contains(skill);
    };
    auto result = general_score(skills, original_mood, stainless_in_dorm);
    static constexpr std::array<std::pair<std::string_view, int>, 7> ProbabilitySkills = { {
        { "bskill_ws_evolve1", 70 },
        { "bskill_ws_evolve2", 75 },
        { "bskill_ws_evolve3", 80 },
        { "bskill_ws_evolve4", 100 },
        { "bskill_ws_cost_blemishine", 40 },
        { "bskill_ws_cost_ju", 60 },
        { "bskill_ws_cost_ju2", 80 },
    } };
    for (const auto& [skill, bonus] : ProbabilitySkills) {
        if (skills.contains(std::string(skill))) {
            result.bonus_percent = std::max(result.bonus_percent, bonus);
        }
    }

    const bool crystal = item == "31034" || item == "30145";
    const bool specialized90 =
        (has("bskill_ws_orirock") && (item == "30012" || item == "30013" || item == "30014")) ||
        (has("bskill_ws_device") && (item == "30062" || item == "30063" || item == "30064")) ||
        (has("bskill_ws_polyester") && (item == "30032" || item == "30033" || item == "30034")) ||
        // Toddifons and Bough share this icon. Only Toddifons covers the entire iron family.
        (has("bskill_ws_oriron") &&
         (item == "30043" || (operator_id == "char_363_toddi" && (item == "30042" || item == "30044"))));
    if (specialized90) {
        result.bonus_percent = std::max(result.bonus_percent, 90);
    }
    if ((has("bskill_ws_ketone") && (item == "30052" || item == "30053" || item == "30054")) ||
        (has("bskill_ws_crystalline") && crystal)) {
        result.bonus_percent = std::max(result.bonus_percent, 80);
    }
    if (has("bskill_ws_alloyblock") && item == "31024") {
        result.bonus_percent = std::max(result.bonus_percent, 100);
    }
    if (has("bskill_ws_lolxh") && original_mood == 8) {
        result.bonus_percent = std::max(result.bonus_percent, 50);
    }
    if (has("bskill_ws_rub") && original_mood == 2) {
        // Humus explicitly adds this bonus to his general probability skill.
        result.bonus_percent += 40;
    }

    // Evaluate every condition against the original cost. Alternative reductions do not stack.
    int reduction = 0;
    if (original_mood == 2 &&
        (has("bskill_ws_all_cost2") || has("bskill_ws_evolve_cost") || (has("bskill_ws_p8") && item == "30043"))) {
        reduction = 1;
    }
    if (original_mood == 4 && (has("bskill_ws_all_cost1") || has("bskill_ws_evolve_cost1"))) {
        reduction = std::max(reduction, 1);
    }
    if ((original_mood == 4 && has("bskill_ws_evolve_cost2")) ||
        (original_mood >= 4 &&
         (has("bskill_ws_cost") || has("bskill_ws_cost_magallan") || has("bskill_ws_cost_ju2"))) ||
        (original_mood >= 8 && has("bskill_ws_cost_ju"))) {
        reduction = std::max(reduction, 2);
    }
    if ((original_mood == 8 && has("bskill_ws_evolve_cost3")) ||
        (original_mood >= 8 && has("bskill_ws_cost_blemishine"))) {
        reduction = std::max(reduction, 4);
    }
    if (crystal && has("bskill_ws_evolve_cost4")) {
        reduction = std::max(reduction, 1);
    }
    result.mood_cost -= reduction;
    if (original_mood >= 4 && has("bskill_ws_cost_lolxh")) {
        if (original_mood % 4 != 0) {
            return std::nullopt;
        }
        result.mood_cost = original_mood / 4;
    }
    if (has("bskill_ws_constant")) {
        result.mood_cost = 2;
    }
    else if (has("bskill_ws_constant3")) {
        result.mood_cost = 3;
    }
    else if (has("bskill_ws_constant2")) {
        result.mood_cost = 4;
    }
    if (has("bskill_ws_nian")) {
        result.mood_cost += 2;
    }
    if (result.bonus_percent == 0 && result.mood_cost >= original_mood) {
        return std::nullopt;
    }
    return result;
}
}
