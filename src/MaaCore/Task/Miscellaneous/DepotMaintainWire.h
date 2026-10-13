#pragma once

#include "DepotMaintainPolicy.h"
#include <charconv>
#include <climits>
#include <meojson/json.hpp>
#include <string_view>

namespace asst::depot_maintain
{
inline bool natural_budget_blocked(const json::value& details)
{
    if (details.get("what", "") != "ExceededLimit" || details.get("details", "max_times", -1) != 0) {
        return false;
    }
    auto task = details.get("details", "task", std::string());
    if (const auto prefix = task.rfind('@'); prefix != std::string::npos) {
        task = task.substr(prefix + 1);
    }
    return task == "MedicineConfirm" || task == "ExpiringMedicineConfirm" || task == "StoneConfirm";
}

inline bool recovery_budget_blocked(const json::value& details, int medicine, int stone, int expire_days)
{
    if (details.get("what", "") != "ExceededLimit") {
        return false;
    }
    auto task = details.get("details", "task", std::string());
    if (const auto prefix = task.rfind('@'); prefix != std::string::npos) {
        task = task.substr(prefix + 1);
    }
    const auto* data = details.find_value("details");
    if (!data || !data->is_object()) {
        return false;
    }
    const auto limit = data->find<int>("max_times");
    if (!limit) {
        return false;
    }
    return (task == "MedicineConfirm" && *limit == medicine) || (task == "StoneConfirm" && *limit == stone) ||
           (task == "ExpiringMedicineConfirm" && *limit == (expire_days == 0 ? 0 : 9999));
}

inline std::optional<int64_t> recognized_count(const json::value& value)
{
    if (!value.is_number()) {
        return std::nullopt;
    }
    const auto raw = value.to_string();
    int64_t count = -1;
    const auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), count);
    if (error != std::errc {} || end != raw.data() + raw.size() || count < 0) {
        return std::nullopt;
    }
    return count;
}

// A single refill limit is not the end of the native recovery flow: medicine
// may fall through to stone. Only its completed close-and-stop route is a
// terminal signal. A timed-out stone confirmation remains unverified.
struct RecoveryStopEvidence
{
    bool terminal_close = false;
    bool confirmation_uncertain = false;

    void observe(const json::value& details, bool completed, bool extra_info)
    {
        if (details.get("subtask", "") != "ProcessTask") {
            return;
        }
        auto task = details.get("details", "task", std::string());
        if (const auto prefix = task.rfind('@'); prefix != std::string::npos) {
            task = task.substr(prefix + 1);
        }
        if (completed && task == "CloseStonePageExceeded") {
            terminal_close = true;
        }
        if (extra_info && details.get("what", "") == "ExceededLimit" && task == "StoneConfirmWait") {
            confirmation_uncertain = true;
        }
    }

    bool verified_stop() const { return terminal_close && !confirmation_uncertain; }
};

inline std::optional<Inventory::Counts> depot_counts(const std::string& data)
{
    auto parsed = json::parse(data);
    if (!parsed || !parsed->is_object()) {
        return std::nullopt;
    }
    Inventory::Counts counts;
    for (const auto& [id, value] : parsed->as_object()) {
        auto count = recognized_count(value);
        if (id.empty() || !count) {
            return std::nullopt;
        }
        counts.emplace(id, *count);
    }
    return counts;
}

// Treat one result screen as a transaction. A malformed later entry must not
// leave an earlier quantity applied to the running inventory.
inline bool apply_actual_drops(Inventory& inventory, const json::value& details)
{
    const auto* data = details.find_value("details");
    const auto* drops = data ? data->find_value("drops") : nullptr;
    if (!drops || !drops->is_array()) {
        return false;
    }
    auto updated = inventory;
    for (const auto& drop : drops->as_array()) {
        const auto* item = drop.find_value("itemId");
        const auto* quantity = drop.find_value("quantity");
        if (!item || !item->is_string() || !quantity) {
            return false;
        }
        const auto count = recognized_count(*quantity);
        if (!count || !updated.add_actual_drop(item->as_string(), *count)) {
            return false;
        }
    }
    inventory = std::move(updated);
    return true;
}

// A valid quantity from a different/unknown result screen is not a valid
// receipt for the currently selected route. Check before any stock mutation.
inline bool apply_actual_stage_drops(
    Inventory& inventory,
    const json::value& details,
    const std::string& expected_code,
    const std::string& expected_id)
{
    if (expected_code.empty() || expected_id.empty()) {
        return false;
    }
    const auto* data = details.find_value("details");
    const auto* stage = data ? data->find_value("stage") : nullptr;
    const auto* code = stage ? stage->find_value("stageCode") : nullptr;
    const auto* id = stage ? stage->find_value("stageId") : nullptr;
    if (!code || !id || !code->is_string() || !id->is_string() || code->as_string() != expected_code ||
        id->as_string() != expected_id) {
        return false;
    }
    return apply_actual_drops(inventory, details);
}

inline bool is_drop_recognition_task(const json::value& details)
{
    // AbstractTask::basic_info derives this exact subtask name from
    // StageDropsTaskPlugin. Network reporting errors are not drop failures.
    return details.get("subtask", "") == "StageDropsTask";
}

inline bool fallback_budget_verified(bool drops_valid, bool natural_limit_seen)
{
    // No target quantity can stop the fallback: its completion needs a fresh
    // natural-budget signal, not the limit reached by an earlier goal.
    return drops_valid && natural_limit_seen;
}

inline bool valid_params(const json::value& params)
{
    if (!params.is_object()) {
        return false;
    }
    for (const auto* key : { "enable", "fallback_open", "only_first", "dependency_preview" }) {
        if (auto value = params.find_value(key); value && !value->is_boolean()) {
            return false;
        }
    }
    if (auto value = params.find_value("fallback_stage"); value && !value->is_string()) {
        return false;
    }
    if (auto value = params.find_value("fight_options"); value && !value->is_object()) {
        return false;
    }
    auto plans = params.find<json::array>("plans");
    if (!plans || plans->size() > 256) {
        return false;
    }
    for (const auto& entry : *plans) {
        if (!entry.is_object()) {
            return false;
        }
        for (const auto* key : { "item_id", "stage" }) {
            if (auto value = entry.find_value(key); value && !value->is_string()) {
                return false;
            }
        }
        for (const auto* key : { "enabled", "stage_open" }) {
            if (auto value = entry.find_value(key); value && !value->is_boolean()) {
                return false;
            }
        }
        auto target = recognized_count(entry.get("target", json::value(0)));
        if (!target || *target > INT_MAX) {
            return false;
        }
        for (const auto* key : { "medicine", "stone", "medicine_expire_days" }) {
            auto count = recognized_count(entry.get(key, json::value(0)));
            if (!count || *count > (std::string_view(key) == "medicine_expire_days" ? 2 : 9999)) {
                return false;
            }
        }
        if (entry.get("enabled", true) &&
            (entry.get("item_id", "").empty() || *target == 0 ||
             (!params.get("dependency_preview", false) && entry.get("stage", "").empty()))) {
            return false;
        }
    }
    return true;
}
}
