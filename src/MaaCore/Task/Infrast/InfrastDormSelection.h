#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace asst::infrast
{
struct DormSelectionCandidate
{
    std::string name;
    std::string operator_id;
    double mood_ratio = 0;
    bool selected = false;
    bool available = true;
};

std::vector<std::string> normalize_fiammetta_targets(const std::vector<std::string>& configured);

std::string_view fiammetta_target_id(std::string_view name) noexcept;

std::optional<size_t> find_fiammetta_target(
    const std::vector<DormSelectionCandidate>& first_page,
    const std::vector<std::string>& fiammetta_targets,
    double mood_threshold);

std::optional<size_t> find_full_mood_fiammetta(const std::vector<DormSelectionCandidate>& first_page);

std::vector<size_t> find_low_mood_candidates(
    const std::vector<DormSelectionCandidate>& candidates,
    double mood_threshold,
    size_t limit);
} // namespace asst::infrast
