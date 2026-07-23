#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "BlackFlowObservation.h"
#include "BlackFlowPlanner.h"
#include "BlackFlowTelemetry.h"

namespace asst::blackflow
{
struct MovementPanelObservation
{
    MovementKind target = MovementKind::Walk;
    int completed_swipes = 0;
    bool complete = false;
    bool target_found = false;
    bool unique_record = false;
    std::optional<int> remaining_charges;
};

[[nodiscard]] inline bool
    movement_panel_observation_is_structurally_valid(const MovementPanelObservation& observation) noexcept
{
    return observation.completed_swipes >= 0 && observation.completed_swipes <= 3 &&
           (!observation.complete || observation.completed_swipes == 3) &&
           (!observation.remaining_charges.has_value() || *observation.remaining_charges >= 0) &&
           (observation.target_found || !observation.remaining_charges.has_value()) &&
           (observation.target != MovementKind::Walk || !observation.remaining_charges.has_value()) &&
           (!observation.unique_record || observation.remaining_charges.has_value());
}

[[nodiscard]] inline bool movement_panel_has_reliable_count(const MovementPanelObservation& observation) noexcept
{
    return observation.target != MovementKind::Walk && observation.target_found && observation.unique_record &&
           observation.remaining_charges.has_value();
}

[[nodiscard]] inline bool movement_panel_confirms_absent(const MovementPanelObservation& observation) noexcept
{
    return observation.complete && observation.completed_swipes == 3 && !observation.target_found;
}

struct RunObservation
{
    std::optional<int> action_points;
    std::optional<int> hope;
    std::optional<int> ingots;
    std::optional<int> seeds;
    std::optional<int> sellable_scraps;
    std::optional<int> white_model_birds;
    std::optional<bool> painted_liberi;
    std::optional<MovementKind> active_movement;
    std::optional<std::unordered_map<MovementKind, int>> movement_charges;
    std::optional<MovementPanelObservation> movement_panel;
    std::optional<std::unordered_set<MovementKind>> cross_floor_expired;
    std::optional<DynamicCostModel> costs;
};

struct BlackFlowPerceptionSnapshot
{
    BlackFlowMapObservation observation;
    RunObservation run;
    FactStore observed_facts;
};

struct BlackFlowObservationRequest
{
    int expected_floor = 1;
};

class IBlackFlowTaskPort
{
public:
    virtual ~IBlackFlowTaskPort() = default;

    virtual bool refresh(
        const BlackFlowObservationRequest& request,
        BlackFlowPerceptionSnapshot& snapshot,
        std::string* error) = 0;
    virtual bool preview(
        const MoveCandidate& candidate,
        const ViewportObservation& viewport,
        MovePreview& preview,
        bool& panel_open,
        std::string* error) = 0;
    virtual bool confirm(const MoveTransaction& transaction, std::string* error) = 0;

    virtual void reset_run() {}

    virtual void configure_diagnostics(const DiagnosticSettings&) {}

    virtual bool persist_diagnostics(const DiagnosticArtifactRequest&, std::string*) { return true; }
};

enum class PreviewDisposition
{
    ReadyToCommit,
    ReplanAfterDismiss,
    Failed,
};
} // namespace asst::blackflow
