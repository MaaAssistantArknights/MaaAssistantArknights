#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "BlackFlowStateSpace.h"

namespace asst::blackflow
{
struct CompactNeighbor
{
    std::uint8_t index = 0;
    bool uses_unconfirmed_edge = false;
    bool uses_inferred_edge = false;
};

struct CompactActionOutcome
{
    PlannerState successor;
    int action_point_gain = 0;
};

struct CompactMoveAction
{
    MoveCandidate candidate;
    int action_point_cost = 0;
    int minimum_action_points_to_start = 1;
    std::vector<CompactActionOutcome> outcomes;
};

class BlackFlowCompactStateSpace
{
public:
    [[nodiscard]] bool initialize(
        const MapSnapshot& map,
        const RunState& run,
        StateExpansionOptions options,
        std::string* error = nullptr);

    [[nodiscard]] const PlannerState& initial_state() const noexcept { return m_initial_state; }

    [[nodiscard]] bool is_terminal(const PlannerState& state) const noexcept;
    [[nodiscard]] bool mask_contains(PlannerNodeMask mask, NodeId node) const noexcept;
    [[nodiscard]] std::optional<std::uint8_t> node_index(NodeId node) const noexcept;

    [[nodiscard]] const std::vector<NodeId>& indexed_nodes() const noexcept { return m_indexed_nodes; }

    [[nodiscard]] const std::vector<CompactNeighbor>& adjacency(GraphLayer layer, std::uint8_t source) const;
    [[nodiscard]] const std::vector<std::uint8_t>& geometric_targets(MovementKind movement, std::uint8_t source) const;

    [[nodiscard]] PlannerNodeMask blocking_nodes() const noexcept { return m_blocking_nodes; }

    [[nodiscard]] PlannerNodeMask transfer_nodes() const noexcept { return m_transfer_nodes; }

    [[nodiscard]] PlannerNodeMask nodes_of_type(NodeType type) const noexcept;

    [[nodiscard]] std::optional<std::vector<CompactMoveAction>>
        actions(const PlannerState& source, std::string* error = nullptr) const;

private:
    struct StaticNode
    {
        NodeId id = InvalidNodeId;
        GridPosition position;
        NodeType type = NodeType::Unknown;
        NodeProgress progress = NodeProgress::Active;
        NodeTraversal traversal;
        NodeIdentityState identity_state = NodeIdentityState::Unclassified;
        std::optional<std::uint8_t> transfer_landing;
    };

    [[nodiscard]] std::optional<PlannerNodeMask> bit(NodeId node) const noexcept;
    [[nodiscard]] NodeProgress effective_progress(const PlannerState& state, std::uint8_t node) const noexcept;
    [[nodiscard]] NodeType effective_type(const PlannerState& state, std::uint8_t node) const noexcept;
    [[nodiscard]] bool targetable_for_walk(const PlannerState& state, std::uint8_t node) const noexcept;
    [[nodiscard]] bool walk_transparent(const PlannerState& state, std::uint8_t node, GraphLayer layer) const noexcept;
    [[nodiscard]] int node_action_point_gain(const PlannerState& state, std::uint8_t node) const noexcept;
    [[nodiscard]] NodeId resolve_landing(std::uint8_t target) const noexcept;
    [[nodiscard]] PlannerState
        transition(const PlannerState& source, const MoveCandidate& candidate, NodeId landing) const;
    [[nodiscard]] int
        outcome_gain(const PlannerState& source, const MoveCandidate& candidate, NodeId landing) const noexcept;
    [[nodiscard]] bool is_endpoint(const PlannerState& state) const noexcept;
    [[nodiscard]] bool unavailable_target(const PlannerState& source, NodeId target) const noexcept;
    void precompute_adjacency(const MapSnapshot& map);
    void precompute_geometry();

    const RunState* m_run = nullptr;
    StateExpansionOptions m_options;
    PlannerState m_initial_state;
    std::vector<NodeId> m_indexed_nodes;
    std::unordered_map<NodeId, std::uint8_t> m_node_indices;
    std::vector<StaticNode> m_nodes;
    std::vector<std::vector<CompactNeighbor>> m_confirmed_adjacency;
    std::vector<std::vector<CompactNeighbor>> m_relaxed_adjacency;
    std::array<std::vector<std::vector<std::uint8_t>>, 13> m_geometric_targets;
    std::array<PlannerNodeMask, 21> m_type_nodes {};
    PlannerNodeMask m_blocking_nodes = 0;
    PlannerNodeMask m_transfer_nodes = 0;
};
} // namespace asst::blackflow
