#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BlackFlowObservation.h"
#include "BlackFlowPlanner.h"
#include "BlackFlowTelemetry.h"

#include "Common/AsstMsg.h"

namespace cv
{
class Mat;
}

namespace asst
{
class Assistant;
}

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
    int floor = 0;
    int attempt_count = 1;
    std::int64_t capture_us = 0;
};

struct EnteredPageObservation
{
    std::vector<std::string> matched_texts;
    std::optional<NodeType> classified_type;
    bool classification_conflict = false;
};

enum class MoveConfirmationStatus
{
    Succeeded,
    NeedsDismiss,
    Failed,
};

[[nodiscard]] EnteredPageObservation classify_entered_page_texts(std::vector<std::string> matched_texts);

class IBlackFlowMapObservationSource
{
public:
    virtual ~IBlackFlowMapObservationSource() = default;

    virtual bool recognize(
        const cv::Mat& image,
        const BlackFlowObservationRequest& request,
        BlackFlowMapObservation& observation,
        FactStore& observed_facts,
        std::string* error) = 0;

    virtual void configure_diagnostics(const DiagnosticSettings&) {}

    virtual bool persist_diagnostics(const DiagnosticArtifactRequest& request, std::string* error) = 0;
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
    virtual MoveConfirmationStatus
        confirm(const MoveTransaction& transaction, EnteredPageObservation& entered_page, std::string* error) = 0;

    virtual void reset_run() {}

    virtual void configure_diagnostics(const DiagnosticSettings&) {}

    virtual bool persist_diagnostics(const DiagnosticArtifactRequest&, std::string*) { return false; }
};

class BlackFlowTaskPort final : public IBlackFlowTaskPort
{
public:
    BlackFlowTaskPort(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        std::shared_ptr<IBlackFlowMapObservationSource> map_source);
    ~BlackFlowTaskPort() override;

    bool refresh(const BlackFlowObservationRequest& request, BlackFlowPerceptionSnapshot& snapshot, std::string* error)
        override;
    bool preview(
        const MoveCandidate& candidate,
        const ViewportObservation& viewport,
        MovePreview& preview,
        bool& panel_open,
        std::string* error) override;
    MoveConfirmationStatus
        confirm(const MoveTransaction& transaction, EnteredPageObservation& entered_page, std::string* error) override;

    void configure_diagnostics(const DiagnosticSettings& settings) override;
    bool persist_diagnostics(const DiagnosticArtifactRequest& request, std::string* error) override;

private:
    class ProcessTaskContext;

    [[nodiscard]] std::optional<int> recognize_action_points(const cv::Mat& image) const;
    bool classify_entered_page(const cv::Mat& image, EnteredPageObservation& observation, std::string* error) const;

    std::unique_ptr<ProcessTaskContext> m_task_context;
    std::shared_ptr<IBlackFlowMapObservationSource> m_map_source;
};

enum class PreviewDisposition
{
    ReadyToCommit,
    ReplanAfterDismiss,
    Failed,
};
} // namespace asst::blackflow
