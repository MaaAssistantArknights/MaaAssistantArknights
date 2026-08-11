#include "InfrastDormSelection.h"

#include <algorithm>
#include <array>
#include <ranges>

namespace
{
using TargetEntry = std::pair<std::string_view, std::string_view>;

constexpr std::array<TargetEntry, 6> FiammettaTargets = {
    TargetEntry { "清流", "char_385_finlpp" },
    TargetEntry { "可露希尔", "char_4228_closur" },
    TargetEntry { "但书", "char_4032_provs" },
    TargetEntry { "巫恋", "char_254_vodfox" },
    TargetEntry { "龙舌兰", "char_486_takila" },
    TargetEntry { "歌蕾蒂娅", "char_474_glady" },
};

constexpr std::array<std::string_view, 3> DefaultFiammettaTargets = { "清流", "可露希尔", "但书" };
constexpr std::string_view FiammettaId = "char_300_phenxi";
constexpr double FullMoodThreshold = 0.99;
constexpr size_t MaxConfiguredTargets = 3;
} // namespace

std::vector<std::string> asst::infrast::normalize_fiammetta_targets(const std::vector<std::string>& configured)
{
    std::vector<std::string> result;
    result.reserve(MaxConfiguredTargets);
    auto append = [&](const auto& source) {
        for (const std::string_view name : source) {
            if (fiammetta_target_id(name).empty() || std::ranges::find(result, name) != result.end()) {
                continue;
            }
            result.emplace_back(name);
            if (result.size() == MaxConfiguredTargets) {
                break;
            }
        }
    };
    if (configured.empty()) {
        append(DefaultFiammettaTargets);
    }
    else {
        append(configured);
    }
    if (result.empty()) {
        append(DefaultFiammettaTargets);
    }
    return result;
}

std::string_view asst::infrast::fiammetta_target_id(std::string_view name) noexcept
{
    const auto iter = std::ranges::find(FiammettaTargets, name, &TargetEntry::first);
    return iter == FiammettaTargets.end() ? std::string_view {} : iter->second;
}

std::optional<size_t> asst::infrast::find_fiammetta_target(
    const std::vector<DormSelectionCandidate>& first_page,
    const std::vector<std::string>& fiammetta_targets,
    double mood_threshold)
{
    const auto normalized_targets = normalize_fiammetta_targets(fiammetta_targets);
    std::optional<size_t> result;
    for (size_t index = 0; index < first_page.size(); ++index) {
        const auto& candidate = first_page[index];
        if (candidate.selected || !candidate.available || candidate.mood_ratio >= mood_threshold ||
            std::ranges::find(normalized_targets, candidate.name) == normalized_targets.end()) {
            continue;
        }
        if (!result || candidate.mood_ratio < first_page[*result].mood_ratio) {
            result = index;
        }
    }
    return result;
}

std::optional<size_t>
asst::infrast::find_full_mood_fiammetta(const std::vector<DormSelectionCandidate>& first_page)
{
    const auto iter = std::ranges::find_if(first_page, [](const DormSelectionCandidate& candidate) {
        return !candidate.selected && candidate.available && candidate.operator_id == FiammettaId &&
            candidate.mood_ratio >= FullMoodThreshold;
    });
    return iter == first_page.end()
        ? std::nullopt
        : std::optional<size_t> { static_cast<size_t>(std::distance(first_page.begin(), iter)) };
}

std::vector<size_t> asst::infrast::find_low_mood_candidates(
    const std::vector<DormSelectionCandidate>& candidates,
    double mood_threshold,
    size_t limit)
{
    std::vector<size_t> result;
    for (size_t index = 0; index < candidates.size(); ++index) {
        const auto& candidate = candidates[index];
        if (!candidate.selected && candidate.available && candidate.mood_ratio < mood_threshold) {
            result.emplace_back(index);
        }
    }
    std::ranges::stable_sort(result, {}, [&](size_t index) { return candidates[index].mood_ratio; });
    if (result.size() > limit) {
        result.resize(limit);
    }
    return result;
}
