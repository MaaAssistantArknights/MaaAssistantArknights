#pragma once

#include "DepotMaintainWire.h"

namespace asst::depot_maintain
{
// DepotInfo contains a complete cumulative snapshot, not a page delta.
// A later scan can withdraw a previously exact quantity after conflicting OCR.
inline bool consume_depot_snapshot(Inventory::Counts& recognized, bool& done, const json::value& details)
{
    auto parsed = depot_counts(details.get("details", "verified_data", ""));
    const auto* data = details.find_value("details");
    const auto* completed = data ? data->find_value("done") : nullptr;
    if (!parsed || !completed || !completed->is_boolean()) {
        recognized.clear();
        done = false;
        return false;
    }
    recognized = std::move(*parsed);
    done = completed->as_boolean();
    return true;
}

// Return a complete post-drop stock receipt only after the current ledger
// accepts this stage result. An earlier invalid result remains disqualifying.
inline std::optional<json::object> accept_stage_inventory(
    Inventory& inventory,
    const json::value& details,
    const std::string& expected_code,
    const std::string& expected_id,
    bool& invalid)
{
    invalid |= !apply_actual_stage_drops(inventory, details, expected_code, expected_id);
    if (invalid) {
        return std::nullopt;
    }
    json::object snapshot;
    for (const auto& [item, count] : inventory.exact_counts()) {
        snapshot.emplace(item, count);
    }
    return snapshot;
}

enum class ShortageFightDecision
{
    TargetMet,
    NaturalBudgetStop,
    UnverifiedStop,
};

inline ShortageFightDecision shortage_fight_decision(int64_t remaining, bool natural_limit_seen)
{
    if (remaining < 0) {
        return ShortageFightDecision::UnverifiedStop;
    }
    if (remaining == 0) {
        return ShortageFightDecision::TargetMet;
    }
    return natural_limit_seen ? ShortageFightDecision::NaturalBudgetStop : ShortageFightDecision::UnverifiedStop;
}
}
