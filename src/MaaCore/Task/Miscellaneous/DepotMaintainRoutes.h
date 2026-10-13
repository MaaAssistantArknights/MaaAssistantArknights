#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace asst::depot_maintain
{
enum class RouteEvidence
{
    ListedDrop,
    FixedResourceReward,
    ItemNotListed,
    StageUnknown,
};

inline bool known_natural_stage(const std::string& stage_id, int ap_cost)
{
    return !stage_id.empty() && ap_cost > 0;
}

// Missing Penguin drop data is NOT proof that an item can never drop.
// Missing evidence nevertheless cannot authorize spending a reserve budget.
// CE/AP fixed rewards are absent from some exported stages.json dropInfos:
// use their exact code + native stage-ID pair, not a loose prefix match.
inline RouteEvidence route_evidence(
    const std::string& code,
    const std::string& stage_id,
    int ap_cost,
    const std::vector<std::string>& listed_items,
    const std::string& item)
{
    if (!known_natural_stage(stage_id, ap_cost)) {
        return RouteEvidence::StageUnknown;
    }
    if (!item.empty() && std::ranges::find(listed_items, item) != listed_items.end()) {
        return RouteEvidence::ListedDrop;
    }
    if (item == "4001") {
        for (int level = 1; level <= 6; ++level) {
            if (code == "CE-" + std::to_string(level) && stage_id == "wk_melee_" + std::to_string(level)) {
                return RouteEvidence::FixedResourceReward;
            }
        }
    }
    if (item == "4006") {
        for (int level = 1; level <= 5; ++level) {
            if (code == "AP-" + std::to_string(level) && stage_id == "wk_toxic_" + std::to_string(level)) {
                return RouteEvidence::FixedResourceReward;
            }
        }
    }
    return RouteEvidence::ItemNotListed;
}

inline bool route_supported(RouteEvidence evidence)
{
    return evidence == RouteEvidence::ListedDrop || evidence == RouteEvidence::FixedResourceReward;
}
}
