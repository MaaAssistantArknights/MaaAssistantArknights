#pragma once

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>

namespace asst::fight
{
inline std::optional<int> remaining_drop_quantity(
    const std::unordered_map<std::string, int>& targets,
    const std::unordered_map<std::string, int>& totals,
    bool recognized)
{
    if (!recognized || targets.empty()) {
        return std::nullopt;
    }
    int remaining = std::numeric_limits<int>::max();
    for (const auto& [item_id, target] : targets) {
        const auto found = totals.find(item_id);
        const int total = found == totals.end() ? 0 : found->second;
        if (item_id.empty() || target < 0 || total < 0) {
            return std::nullopt;
        }
        // Fight stops when any configured drop target is reached.
        remaining = std::min(remaining, total >= target ? 0 : target - total);
    }
    return remaining;
}

inline int series_limit(int remaining_fights, int maximum_series, int remaining_drops)
{
    return std::max(0, std::min({ remaining_fights, maximum_series, remaining_drops }));
}
}
