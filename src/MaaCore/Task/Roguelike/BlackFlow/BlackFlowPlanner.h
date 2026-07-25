#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>

#include "BlackFlowPolicy.h"
#include "BlackFlowSafetyPlanner.h"
#include "BlackFlowStateSpace.h"

namespace asst::blackflow
{
struct RouteSearchOptions
{
    int time_budget_ms = 1000;
    std::size_t total_expansions = 4096;
    std::size_t expansions_per_root = 128;
    int greedy_preview_depth = 2;
    bool safety_resource_dominance = true;
};

struct BlackFlowPlanRequest
{
    const MapSnapshot* map = nullptr;
    const RunState* run = nullptr;
    const ResolvedPolicy* policy = nullptr;
    const FactStore* facts = nullptr;
    const MissionState* mission = nullptr;
    std::unordered_set<NodeId> strategy_terminal_nodes;
    const std::unordered_set<std::string>* forbidden_actions = nullptr;
    std::optional<NodeId> probe_target;
    std::size_t maximum_states = 2'000'000;
    RouteSearchOptions route_search;
};

struct PreviewSafetyVerification
{
    bool safe = false;
    int action_points_after = 0;
    int required_action_points_after = UnreachableActionPointRequirement;
    std::optional<std::size_t> proof_depth;
    std::string error;
};

struct BlackFlowPlan
{
    SafetyAssessment safety;
    SafetyAssessment relaxed_safety;
    PolicyDecision decision;
    std::optional<MoveCandidate> escape_first_action;
    std::uint64_t map_revision = 0;
    std::uint64_t cost_revision = 0;
    std::size_t confirmed_state_count = 0;
    std::size_t relaxed_state_count = 0;
    std::size_t route_search_expansions = 0;
    bool route_search_time_exhausted = false;
    bool route_search_expansions_exhausted = false;
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return error.empty() && decision.selected.has_value(); }
};

class BlackFlowPlanner
{
public:
    [[nodiscard]] BlackFlowPlan plan(const BlackFlowPlanRequest& request) const;
    [[nodiscard]] PreviewSafetyVerification verify_previewed_move(
        const BlackFlowPlanRequest& request,
        const MoveCandidate& move,
        int exact_action_point_cost) const;

private:
    [[nodiscard]] FactStore candidate_facts(
        const MapSnapshot& map,
        const ExpandedSafetyProblem& expanded,
        const SafetySolution& solution,
        const SafetyAction& root_action,
        const RunState& run,
        int current_action_points) const;
};
} // namespace asst::blackflow
