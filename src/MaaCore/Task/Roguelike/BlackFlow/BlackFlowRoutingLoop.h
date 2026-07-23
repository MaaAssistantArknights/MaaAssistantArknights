#pragma once

#include <string>
#include <utility>

#include "BlackFlowTaskPort.h"

namespace asst::blackflow
{
enum class RoutingCycleStatus
{
    MoveCommitted,
    MovementSelectionRequired,
    ReplanRequired,
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
bool validate_session_commit(Session& session, std::string* error)
{
    if constexpr (requires { session.validate_commit(error); }) {
        return session.validate_commit(error);
    }
    else {
        return true;
    }
}

template <typename Session>
RoutingCycleOutcome execute_preview_cycle(Session& session, IBlackFlowTaskPort& port)
{
    std::string error;
    if (session.transaction() == nullptr) {
        return { RoutingCycleStatus::Failed,
                 "transaction_proposal_failed",
                 "move preview has no proposed transaction" };
    }

    MovePreview preview;
    bool panel_open = false;
    const MoveCandidate candidate = session.transaction()->proposal();
    if (!port.preview(candidate, session.viewport(), preview, panel_open, &error)) {
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
    if (!validate_session_commit(session, &error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_invalidated", std::move(error) };
    }
    if (!port.confirm(*session.transaction(), &error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_failed", std::move(error) };
    }
    if (!session.commit(&error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_state_failed", std::move(error) };
    }
    return { RoutingCycleStatus::MoveCommitted, {}, {} };
}

template <typename Session>
RoutingCycleOutcome execute_pending_routing_cycle(Session& session, IBlackFlowTaskPort& port)
{
    std::string error;
    if (!session.begin_pending_transaction(&error)) {
        session.clear_pending_candidate();
        return { RoutingCycleStatus::ReplanRequired, "pending_movement_invalidated", std::move(error) };
    }
    return execute_preview_cycle(session, port);
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
    const MoveCandidate candidate = *plan.decision.selected;
    if constexpr (requires {
                      session.run().active_movement;
                      session.save_pending_candidate(candidate, &error);
                  }) {
        if (!session.run().active_movement.has_value() || *session.run().active_movement != candidate.movement) {
            if (!session.save_pending_candidate(candidate, &error)) {
                return { RoutingCycleStatus::Failed, "movement_selection_proposal_failed", std::move(error) };
            }
            return { RoutingCycleStatus::MovementSelectionRequired, {}, {} };
        }
    }
    if (!session.begin_transaction(candidate, &error)) {
        return { RoutingCycleStatus::Failed, "transaction_proposal_failed", std::move(error) };
    }
    return execute_preview_cycle(session, port);
}
} // namespace asst::blackflow
