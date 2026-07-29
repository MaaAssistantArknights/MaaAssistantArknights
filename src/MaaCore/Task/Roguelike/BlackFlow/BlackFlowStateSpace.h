#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BlackFlowModel.h"
#include "BlackFlowSafetyGoal.h"
#include "BlackFlowSafetyPlanner.h"

namespace asst::blackflow
{
class BlackFlowCompactStateSpace;
using PlannerNodeMask = std::uint64_t;

struct PlannerState
{
    NodeId node = InvalidNodeId;
    std::array<std::uint8_t, 13> movement_charges {};
    PlannerNodeMask completed_nodes = 0;
    PlannerNodeMask opened_blockers = 0;
    PlannerNodeMask consumed_lights = 0;
    SafetyGoalProgressId goal_progress_id = InvalidSafetyGoalProgressId;
    bool terminal = false;

    bool operator==(const PlannerState&) const noexcept = default;
};

struct PlannerStateHash
{
    std::size_t operator()(const PlannerState& state) const noexcept;
};

struct StateExpansionOptions
{
    std::unordered_set<NodeId> strategy_goal_nodes;
    std::unordered_set<std::string> forbidden_action_ids;
    GraphLayer graph_layer = GraphLayer::Confirmed;
    bool final_is_terminal = true;
    SafetyGoalProgram* safety_goal = nullptr;
    const FactStore* safety_goal_facts = nullptr;
    SafetyGoalProgressId initial_goal_progress_id = InvalidSafetyGoalProgressId;
    bool use_compact_actions = true;
    std::size_t maximum_states = 2'000'000;
};

struct OnDemandSafetyOutcome
{
    SafetyStateId successor = 0;
    int action_point_gain = 0;
};

struct OnDemandSafetyAction
{
    MoveCandidate candidate;
    int action_point_cost = 0;
    int minimum_action_points_to_start = 1;
    std::vector<OnDemandSafetyOutcome> outcomes;
};

class OnDemandStateGraph
{
public:
    OnDemandStateGraph();
    ~OnDemandStateGraph();
    OnDemandStateGraph(const OnDemandStateGraph&) = delete;
    OnDemandStateGraph& operator=(const OnDemandStateGraph&) = delete;
    [[nodiscard]] bool initialize(
        const MapSnapshot& map,
        const RunState& run,
        StateExpansionOptions options,
        std::string* error = nullptr);

    [[nodiscard]] SafetyStateId initial_state() const noexcept { return m_initial_state; }

    [[nodiscard]] const PlannerState& state(SafetyStateId id) const { return m_states.at(id); }

    [[nodiscard]] bool is_terminal(SafetyStateId id) const noexcept;
    [[nodiscard]] bool is_terminal_node(NodeId node) const noexcept;
    [[nodiscard]] bool is_completed(SafetyStateId id, NodeId node) const noexcept;
    [[nodiscard]] bool is_light_consumed(SafetyStateId id, NodeId node) const noexcept;
    [[nodiscard]] const std::vector<OnDemandSafetyAction>* actions(SafetyStateId id, std::string* error = nullptr);

    [[nodiscard]] std::size_t state_count() const noexcept { return m_states.size(); }

    [[nodiscard]] const MapSnapshot& map() const noexcept { return *m_map; }

    [[nodiscard]] const RunState& source_run() const noexcept { return *m_run; }

private:
    [[nodiscard]] std::optional<SafetyStateId> intern(PlannerState state, std::string* error);
    [[nodiscard]] RunState materialize(const PlannerState& state) const;
    [[nodiscard]] bool state_is_endpoint(const PlannerState& state) const noexcept;
    [[nodiscard]] bool state_is_goal(const PlannerState& state) const noexcept;
    [[nodiscard]] std::optional<PlannerNodeMask> bit(NodeId node) const noexcept;

    const MapSnapshot* m_map = nullptr;
    const RunState* m_run = nullptr;
    StateExpansionOptions m_options;
    SafetyStateId m_initial_state = 0;
    std::vector<NodeId> m_indexed_nodes;
    std::unordered_map<NodeId, std::uint8_t> m_node_indices;
    std::vector<PlannerState> m_states;
    std::unordered_map<PlannerState, SafetyStateId, PlannerStateHash> m_ids;
    std::deque<std::optional<std::vector<OnDemandSafetyAction>>> m_actions;
    std::unique_ptr<BlackFlowCompactStateSpace> m_compact;
};

struct ProjectedMoveOutcome
{
    RunState run;
    int action_point_gain = 0;
};

[[nodiscard]] std::optional<ProjectedMoveOutcome> project_move_outcome(
    const MapSnapshot& map,
    const RunState& run,
    const MoveCandidate& move,
    int exact_action_point_cost,
    std::string* error = nullptr);

} // namespace asst::blackflow
