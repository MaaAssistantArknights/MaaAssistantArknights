#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include <meojson/json.hpp>

#include "BlackFlowTaskPort.h"

namespace asst::blackflow
{
enum class RoutingCycleStatus
{
    MovedToNode,
    SessionTerminated,
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
    for (int attempt = 0; attempt < 2; ++attempt) {
        BlackFlowPerceptionSnapshot snapshot;
        std::string current_error;
        if (port.refresh(snapshot, &current_error) && session.update(snapshot, &current_error)) {
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
        std::string recovery_error;
        if (!port.recover_map(&recovery_error)) {
            return {
                RoutingCycleStatus::Failed,
                "page_recovery_failed",
                recovery_error.empty() ? std::move(error) : std::move(recovery_error),
            };
        }
        if (!refresh_with_retries(session, port, &error)) {
            return { RoutingCycleStatus::Failed, "map_rebuild_failed", std::move(error) };
        }
    }
    if (session.terminated()) {
        return { RoutingCycleStatus::SessionTerminated, {}, {} };
    }

    for (std::size_t attempt = 0; attempt < 256; ++attempt) {
        BlackFlowPlan plan = session.plan(&error);
        if (!plan) {
            return { RoutingCycleStatus::Failed, "planning_failed", std::move(error) };
        }
        if (!session.begin_transaction(*plan.decision.selected, &error)) {
            return { RoutingCycleStatus::Failed, "transaction_proposal_failed", std::move(error) };
        }

        MovePreview preview;
        if (!port.preview(*plan.decision.selected, session.viewport(), preview, &error)) {
            return { RoutingCycleStatus::Failed, "move_preview_failed", std::move(error) };
        }
        const PreviewDisposition disposition = session.accept_preview(std::move(preview), &error);
        if (disposition == PreviewDisposition::Replan) {
            continue;
        }
        if (disposition == PreviewDisposition::Failed || session.transaction() == nullptr) {
            return { RoutingCycleStatus::Failed, "move_preview_rejected", std::move(error) };
        }
        if (!port.confirm(*session.transaction(), &error) || !session.commit(&error)) {
            return { RoutingCycleStatus::Failed, "move_confirmation_failed", std::move(error) };
        }

        BlackFlowPostMoveSnapshot post_move;
        if (!port.observe_after_commit(post_move, &error) || !session.apply_post_move(post_move, &error) ||
            !session.bind_page_route(&error)) {
            return { RoutingCycleStatus::Failed, "post_move_validation_failed", std::move(error) };
        }
        return { RoutingCycleStatus::MovedToNode, {}, {} };
    }
    return {
        RoutingCycleStatus::Failed,
        "planning_retry_exhausted",
        "preview replanning exceeded the finite candidate limit",
    };
}

template <typename Session>
bool execute_routing_task_event(Session& session, const json::value& callback_details, std::string* error)
{
    return session.apply_task_event(callback_details, error);
}
} // namespace asst::blackflow
