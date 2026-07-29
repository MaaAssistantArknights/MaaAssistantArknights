#pragma once

#include <string>
#include <utility>

#include "BlackFlowTaskPort.h"

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
enum class RoutingCycleStatus
{
    MoveCommitted,
    MoveCommittedToMap,
    MovementSelectionRequired,
    MovementInventoryObservationRequired,
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
    const std::optional<int> current_floor = session.current_floor();
    if (!current_floor.has_value()) {
        if (error != nullptr) {
            *error = "current floor has not been recognized by NextLevel";
        }
        return false;
    }

    std::string latest_error;
    for (int attempt = 0; attempt < 2; ++attempt) {
        BlackFlowObservationRequest request;
        request.floor = *current_floor;
        request.attempt_count = attempt + 1;
        BlackFlowPerceptionSnapshot snapshot;
        std::string current_error;
        if (port.refresh(request, snapshot, &current_error) && session.update(snapshot, &current_error)) {
            return true;
        }
        const auto* failure = current_error.empty() ? "unknown" : current_error.c_str();
        if (attempt == 0) {
            Log.info(
                "BlackFlow map rebuild attempt failed",
                "floor",
                request.floor,
                "attempt",
                attempt + 1,
                "of",
                2,
                "error",
                failure);
        }
        else {
            Log.debug(
                "BlackFlow map rebuild attempt failed",
                "floor",
                request.floor,
                "attempt",
                attempt + 1,
                "of",
                2,
                "error",
                failure);
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
bool session_requires_movement_inventory_observation(const Session& session)
{
    if constexpr (requires { session.movement_inventory_refresh_required(); }) {
        return session.movement_inventory_refresh_required();
    }
    else {
        return false;
    }
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
bool session_requires_page_dispatch(const Session& session)
{
    if constexpr (requires { session.page_dispatch_required(); }) {
        return session.page_dispatch_required();
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
        session.cancel_transaction();
        return { RoutingCycleStatus::PreviewNeedsDismiss, "move_preview_rejected", std::move(error) };
    }
    if (!validate_session_commit(session, &error)) {
        session.cancel_transaction();
        return { RoutingCycleStatus::PreviewNeedsDismiss, "move_confirmation_invalidated", std::move(error) };
    }
    EnteredPageObservation entered_page;
    if (!port.confirm(*session.transaction(), entered_page, &error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_failed", std::move(error) };
    }
    if (!session.commit(std::move(entered_page), &error)) {
        return { RoutingCycleStatus::Failed, "move_confirmation_state_failed", std::move(error) };
    }
    return { session_requires_page_dispatch(session) ? RoutingCycleStatus::MoveCommitted
                                                     : RoutingCycleStatus::MoveCommittedToMap,
             {},
             {} };
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
    // 终止判定先于地图重建：楼层识别就可能已经把这一局判完，那就不该再为一张用不上的地图折腾。
    if (session.terminated()) {
        return { RoutingCycleStatus::SessionTerminated, {}, {} };
    }
    if (!refresh_with_retries(session, port, &error)) {
        return { RoutingCycleStatus::NeedsPageRecovery, "map_rebuild_failed", std::move(error) };
    }
    if (session_requires_movement_inventory_observation(session)) {
        return { RoutingCycleStatus::MovementInventoryObservationRequired, {}, {} };
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
