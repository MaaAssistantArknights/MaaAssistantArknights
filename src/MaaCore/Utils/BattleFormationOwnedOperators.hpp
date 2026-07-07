#pragma once

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace asst::algorithm
{
template <typename Oper, typename Status>
inline size_t mark_unowned_formation_candidates_missing(
    std::vector<std::pair<std::string, std::vector<Oper>>>& groups,
    const std::unordered_set<std::string>& owned_opers,
    Status selected,
    Status unchecked,
    Status missing)
{
    if (owned_opers.empty()) {
        return 0;
    }

    size_t changed = 0;
    for (auto& group : groups) {
        bool has_selected = false;
        for (const auto& oper : group.second) {
            if (oper.status == selected) {
                has_selected = true;
                break;
            }
        }
        if (has_selected) {
            continue;
        }

        for (auto& oper : group.second) {
            if (oper.status == unchecked && !owned_opers.contains(oper.name)) {
                oper.status = missing;
                ++changed;
            }
        }
    }

    return changed;
}
} // namespace asst::algorithm
