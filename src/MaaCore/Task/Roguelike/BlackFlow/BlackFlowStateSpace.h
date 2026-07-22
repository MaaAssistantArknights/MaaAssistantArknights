#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BlackFlowModel.h"
#include "BlackFlowSafetyPlanner.h"

namespace asst::blackflow
{
struct PlannerState
{
    NodeId node = InvalidNodeId;
    std::array<std::uint8_t, 13> movement_charges {};
    std::array<bool, 13> cross_floor_expired {};
    std::vector<NodeId> completed_nodes;
    std::vector<NodeId> visited_nodes;
    std::vector<NodeId> consumed_one_time_nodes;
    std::vector<NodeId> revealed_nodes;
    std::uint64_t dynamic_cost_revision = 0;
    bool terminal = false;

    bool operator==(const PlannerState&) const noexcept = default;
};

struct PlannerStateHash
{
    std::size_t operator()(const PlannerState& state) const noexcept;
};

struct StateExpansionOptions
{
    MapKnowledgeMode knowledge = MapKnowledgeMode::Confirmed;
    std::unordered_set<NodeId> strategy_terminal_nodes;
    std::unordered_set<std::string> forbidden_action_ids;
    bool final_is_terminal = true;
    bool fate_is_terminal = false;
    std::size_t maximum_states = 200000;
};

struct ExpandedSafetyProblem
{
    SafetyProblem problem;
    SafetyStateId initial_state = 0;
    std::vector<PlannerState> planner_states;
    std::unordered_map<std::string, MoveCandidate> action_candidates;
};

class BlackFlowStateExpander
{
public:
    [[nodiscard]] std::optional<ExpandedSafetyProblem> build(
        const MapSnapshot& map,
        const RunState& run,
        const StateExpansionOptions& options,
        std::string* error = nullptr) const;

private:
    [[nodiscard]] bool is_terminal(
        const MapSnapshot& map,
        const PlannerState& state,
        const StateExpansionOptions& options) const noexcept;
};
} // namespace asst::blackflow

