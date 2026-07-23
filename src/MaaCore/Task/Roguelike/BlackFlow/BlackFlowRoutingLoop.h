#pragma once

#include <string>
#include <utility>

#include "BlackFlowTaskPort.h"

namespace asst::blackflow
{
enum class RoutingCycleStatus
{
    MoveCommitted,
    PreviewNeedsDismiss,
    SessionTerminated,
    NeedsPageRecovery,
    Failed,
};

struct RoutingCycleOutcome
{
    RoutingCycleStatus status = RoutingCycleStatus::Failed;
    std::string failure_code;
    std::string error;
};

template <typename Session>
bool refresh_with_retries(Session& session, IBlackFlowTaskPort& port, std::string* error)
{
    std::string latest_error;
    const BlackFlowObservationRequest request { session.expected_observation_floor() };
    for (int attempt = 0; attempt < 2; ++attempt) {
        BlackFlowPerceptionSnapshot snapshot;
        std::string current_error;
        if (port.refresh(request, snapshot, &current_error) && session.update(snapshot, &current_error)) {
            return true;
        }
        if (!current_error.empty()) {
            latest_error = std::move(current_error);
        }
    }
    if (error != nullptr) {
        *error = latest_error.empty() ? "map rebuild failed twice" : std::move(latest_error);
    }
    return false;
}

template <typename Session>
RoutingCycleOutcome execute_routing_cycle(Session& session, IBlackFlowTaskPort& port)
{
    std::string error;
    if (!refresh_with_retries(session, port, &error)) {
        return { RoutingCycleStatus::NeedsPageRecovery, "map_rebuild_failed", std::move(error) };
    }
    if (session.terminated()) {
        return { RoutingCycleStatus::SessionTerminated, {}, {} };
    }

    BlackFlowPlan plan = session.plan(&error);
    if (!plan) {
        return { RoutingCycleStatus::Failed, "planning_failed", std::move(error) };
    }
    if (!session.begin_transaction(*plan.decision.selected, &error)) {
        return { RoutingCycleStatus::Failed, "transaction_proposal_failed", std::move(error) };
    }

    MovePreview preview;
    bool panel_open = false;
    if (!port.preview(*plan.decision.selected, session.viewport(), preview, panel_open, &error)) {
        session.cancel_transaction();
        if (panel_open) {
            return { RoutingCycleStatus::PreviewNeedsDismiss, "move_preview_failed", std::move(error) };
        }
        return { RoutingCycleStatus::Failed, "move_preview_failed", std::move(error) };
    }
    const PreviewDisposition disposition = session.accept_preview(std::move(preview), &error);
    if (disposition == PreviewDisposition::ReplanAfterDismiss) {
        return { RoutingCycleStatus::PreviewNeedsDismiss, {}, {} };
    }
    if (disposition == PreviewDisposition::Failed || session.transaction() == nullptr) {
        return { RoutingCycleStatus::Failed, "move_preview_rejected", std::move(error) };
    }
    if (!port.confirm(*session.transaction(), &error) || !session.commit(&error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_failed", std::move(error) };
    }
    return { RoutingCycleStatus::MoveCommitted, {}, {} };
}
} // namespace asst::blackflow
