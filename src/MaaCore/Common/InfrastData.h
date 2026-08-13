#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace asst::infrast
{
using OperatorIds = std::unordered_set<std::string>;

enum class FacilityPlanMode
{
    Default,
    Custom,
    Rotation,
};

enum class FacilityStep
{
    DormPrepare,
    DormFill,
    MfgInspect,
    Mfg,
    Trade,
    Power,
    Office,
    ControlForce,
    ControlVacancy,
    Reception,
    Processing,
    Training,
};

inline std::optional<std::vector<FacilityStep>>
    build_facility_plan(FacilityPlanMode mode, const std::vector<std::string>& facilities)
{
    const auto to_step = [](const std::string& facility) -> std::optional<FacilityStep> {
        if (facility == "Dorm") {
            return FacilityStep::DormPrepare;
        }
        if (facility == "Mfg") {
            return FacilityStep::Mfg;
        }
        if (facility == "Trade") {
            return FacilityStep::Trade;
        }
        if (facility == "Power") {
            return FacilityStep::Power;
        }
        if (facility == "Office") {
            return FacilityStep::Office;
        }
        if (facility == "Control") {
            return FacilityStep::ControlForce;
        }
        if (facility == "Reception") {
            return FacilityStep::Reception;
        }
        if (facility == "Processing") {
            return FacilityStep::Processing;
        }
        if (facility == "Training") {
            return FacilityStep::Training;
        }
        return std::nullopt;
    };

    for (const auto& facility : facilities) {
        if (!to_step(facility)) {
            return std::nullopt;
        }
    }

    if (mode != FacilityPlanMode::Default) {
        const std::unordered_set<std::string> rotation_skips = { "Dorm", "Power", "Office", "Control" };
        std::vector<FacilityStep> result;
        result.reserve(facilities.size());
        for (const auto& facility : facilities) {
            if (mode == FacilityPlanMode::Rotation && rotation_skips.contains(facility)) {
                continue;
            }
            result.emplace_back(*to_step(facility));
        }
        return result;
    }

    // 常规模式中的 facility 是启用集合，数组顺序与重复项不参与调度。
    const std::unordered_set<std::string> enabled(facilities.begin(), facilities.end());
    std::vector<FacilityStep> result;
    if (enabled.contains("Trade") && !enabled.contains("Mfg")) {
        result.emplace_back(FacilityStep::MfgInspect);
    }
    if (enabled.contains("Dorm")) {
        result.emplace_back(FacilityStep::DormPrepare);
    }
    if (enabled.contains("Power")) {
        result.emplace_back(FacilityStep::Power);
    }
    if (enabled.contains("Office")) {
        result.emplace_back(FacilityStep::Office);
    }
    if (enabled.contains("Control")) {
        result.emplace_back(FacilityStep::ControlForce);
    }
    if (enabled.contains("Mfg")) {
        result.emplace_back(FacilityStep::Mfg);
    }
    if (enabled.contains("Trade")) {
        result.emplace_back(FacilityStep::Trade);
    }
    if (enabled.contains("Control")) {
        result.emplace_back(FacilityStep::ControlVacancy);
    }
    if (enabled.contains("Reception")) {
        result.emplace_back(FacilityStep::Reception);
    }
    if (enabled.contains("Dorm")) {
        result.emplace_back(FacilityStep::DormFill);
    }
    if (enabled.contains("Processing")) {
        result.emplace_back(FacilityStep::Processing);
    }
    if (enabled.contains("Training")) {
        result.emplace_back(FacilityStep::Training);
    }
    return result;
}

inline OperatorIds intersect_operator_ids(const std::vector<OperatorIds>& candidates)
{
    auto iter = std::ranges::find_if(candidates, [](const OperatorIds& ids) { return !ids.empty(); });
    if (iter == candidates.end()) {
        return { };
    }

    OperatorIds result = *iter;
    for (++iter; iter != candidates.end(); ++iter) {
        if (iter->empty()) {
            continue;
        }
        std::erase_if(result, [&](const std::string& id) { return !iter->contains(id); });
    }
    return result;
}

inline bool operator_id_matches_candidates(const OperatorIds& candidates, std::string_view recognized_id)
{
    return !recognized_id.empty() &&
           (candidates.empty() || candidates.contains(std::string(recognized_id)));
}

struct OperatorSelection
{
    OperatorIds operator_ids;
    OperatorIds pending_operator_ids;

    void commit_pending()
    {
        operator_ids.insert(pending_operator_ids.begin(), pending_operator_ids.end());
        pending_operator_ids.clear();
    }

    void discard_pending() { pending_operator_ids.clear(); }

    void clear_operator_selection()
    {
        operator_ids.clear();
        pending_operator_ids.clear();
    }
};
} // namespace asst::infrast
