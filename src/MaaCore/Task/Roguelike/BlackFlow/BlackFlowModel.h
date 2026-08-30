#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst::blackflow
{
using NodeId = std::uint64_t;

inline constexpr NodeId InvalidNodeId = std::numeric_limits<NodeId>::max();

struct GridPosition
{
    int row = 0;
    int column = 0;

    bool operator==(const GridPosition&) const noexcept = default;
    auto operator<=>(const GridPosition&) const noexcept = default;
};

struct GridPositionHash
{
    std::size_t operator()(const GridPosition& position) const noexcept;
};

[[nodiscard]] std::optional<NodeId> make_stable_node_id(int floor, GridPosition position) noexcept;

enum class NodeType
{
    Unknown,
    BattleElite,
    BattleNormal,
    BattleSavage,
    Duel,
    Door,
    Employ,
    Expedition,
    HideBattle,
    HideInvisible,
    Incident,
    Light,
    Portal,
    Rest,
    Sacrifice,
    ScrapShop,
    Shop,
    Wish,
    Empty,
    Evacuate,
    Final,
    BattleBoss,
};

enum class NodeProgress
{
    Active,
    Completed,
    Removed,
};

enum class NodeIdentityState
{
    Classified,
    Hidden,
    Unclassified,
};

enum class EdgeKnowledge
{
    Unknown,
    Confirmed,
    Absent,
};

enum class GraphLayer
{
    Confirmed,
    Relaxed,
};

struct NodeTraversal
{
    bool blocks_walk = true;
    bool blocks_vision = true;
    bool repeatable = false;
    bool enterable = true;

    bool operator==(const NodeTraversal&) const noexcept = default;
};

struct Node
{
    NodeId id = InvalidNodeId;
    int floor = 0;
    GridPosition position;
    NodeType type = NodeType::Unknown;
    std::string name;
    NodeProgress progress = NodeProgress::Active;
    NodeTraversal traversal;
    NodeIdentityState identity_state = NodeIdentityState::Unclassified;
    bool identity_revealed = false;
    std::string marker_type;
    std::string marker_display_name;
    double marker_score = 0.0;
    bool badged = false;
    std::optional<NodeId> transfer_target;

    bool operator==(const Node&) const noexcept = default;
};

struct EdgeEvidence
{
    double probability = 0.0;
    bool cnn_connected = false;
    bool forced_by_connectivity_constraint = false;
    std::string decision_source;
};

struct Edge
{
    NodeId first = InvalidNodeId;
    NodeId second = InvalidNodeId;
    EdgeKnowledge knowledge = EdgeKnowledge::Unknown;
    EdgeEvidence evidence;
};

class MapSnapshot
{
public:
    bool upsert_node(Node node);
    bool remove_node(NodeId id);
    bool upsert_edge(Edge edge);

    [[nodiscard]] const Node* find_node(NodeId id) const noexcept;
    [[nodiscard]] const Node* find_node(int floor, GridPosition position) const noexcept;
    [[nodiscard]] EdgeKnowledge edge_knowledge(NodeId first, NodeId second) const noexcept;
    [[nodiscard]] const Edge* find_edge(NodeId first, NodeId second) const noexcept;
    [[nodiscard]] std::vector<NodeId> neighbors(NodeId id, GraphLayer layer = GraphLayer::Confirmed) const;
    [[nodiscard]] std::unordered_set<NodeId> reveal_through_transparent_nodes(NodeId origin) const;
    [[nodiscard]] std::unordered_set<NodeId> nodes_within_manhattan(NodeId origin, int distance) const;
    [[nodiscard]] bool has_valid_transfer_pair(NodeId node) const noexcept;
    [[nodiscard]] bool validate(std::string* error = nullptr) const;

    [[nodiscard]] const auto& nodes() const noexcept { return m_nodes; }

    [[nodiscard]] const auto& edges() const noexcept { return m_edges; }

    std::uint64_t revision = 0;

private:
    std::unordered_map<NodeId, Node> m_nodes;
    std::vector<Edge> m_edges;
};

enum class ObservationCoverage
{
    PartialViewport,
    FullMap,
};

struct ObservedNode
{
    GridPosition position;
    std::optional<NodeType> type;
    std::optional<std::string> name;
    std::optional<NodeProgress> progress;
    std::optional<NodeTraversal> traversal;
    std::optional<NodeIdentityState> identity_state;
    std::optional<bool> identity_revealed;
    std::optional<std::string> marker_type;
    std::optional<std::string> marker_display_name;
    std::optional<double> marker_score;
    std::optional<bool> badged;
    std::optional<std::optional<GridPosition>> transfer_target;
};

struct ObservedEdge
{
    GridPosition first;
    GridPosition second;
    EdgeKnowledge knowledge = EdgeKnowledge::Unknown;
    EdgeEvidence evidence;
};

struct MapObservationBatch
{
    int floor = 0;
    ObservationCoverage coverage = ObservationCoverage::PartialViewport;
    std::vector<GridPosition> covered_positions;
    std::vector<ObservedNode> nodes;
    std::vector<ObservedEdge> edges;
};

class NormalizedMap
{
public:
    [[nodiscard]] bool merge(const MapObservationBatch& batch, std::string* error = nullptr);
    void reset();

    [[nodiscard]] const MapSnapshot& snapshot() const noexcept { return m_snapshot; }

    [[nodiscard]] MapSnapshot& snapshot() noexcept { return m_snapshot; }

    [[nodiscard]] int floor() const noexcept { return m_floor; }

private:
    int m_floor = 0;
    MapSnapshot m_snapshot;
};

struct NodeObservation
{
    NodeId node = InvalidNodeId;
    Rect icon_rect;
    std::optional<Rect> text_rect;
    double icon_confidence = 0.0;
    double text_confidence = 0.0;
};

class ViewportObservation
{
public:
    void
        replace(std::vector<NodeObservation> observations, std::uint64_t map_revision, std::uint64_t viewport_revision);
    void clear(std::uint64_t map_revision, std::uint64_t viewport_revision);
    [[nodiscard]] const NodeObservation* find(NodeId node) const noexcept;
    [[nodiscard]] std::optional<Rect> clickable_rect(
        NodeId node,
        std::uint64_t expected_map_revision,
        std::uint64_t expected_viewport_revision) const;

    [[nodiscard]] const auto& nodes() const noexcept { return m_nodes; }

    [[nodiscard]] std::uint64_t map_revision() const noexcept { return m_map_revision; }

    [[nodiscard]] std::uint64_t viewport_revision() const noexcept { return m_viewport_revision; }

private:
    std::unordered_map<NodeId, NodeObservation> m_nodes;
    std::uint64_t m_map_revision = 0;
    std::uint64_t m_viewport_revision = 0;
};

[[nodiscard]] NodeTraversal default_traversal_for(NodeType type) noexcept;
[[nodiscard]] bool is_transfer_node(NodeType type) noexcept;
[[nodiscard]] bool is_combat_node_type(NodeType type) noexcept;
[[nodiscard]] bool is_exit_node_type(NodeType type) noexcept;
[[nodiscard]] std::optional<NodeType> node_type_from_string(std::string_view value) noexcept;

enum class MovementKind
{
    Walk,
    M01,
    M02,
    M03,
    M04,
    M05,
    M06,
    M07,
    M08,
    M09,
    M10,
    M11,
    M12,
};

enum class MovementRange
{
    WalkEdges,
    OrthogonalTwo,
    SurroundingEight,
    ManhattanTwo,
    OrthogonalThree,
    FullMap,
};

struct MovementEffect
{
    int action_point_gain = 0;
    int hope_gain = 0;
    int ingot_gain = 0;
};

struct MovementSpec
{
    MovementKind kind = MovementKind::Walk;
    std::string_view id;
    std::string_view name;
    MovementRange range = MovementRange::WalkEdges;
    std::vector<NodeType> target_types;
    int action_point_cost = 1;
    int initial_charges = 0;
    bool random_target = false;
    bool expires_on_floor_end = false;
    MovementEffect effect;
};

[[nodiscard]] bool node_type_allowed(const MovementSpec& movement, NodeType type) noexcept;

// 加工品移动可以作为落点的全部节点类型。按落点能力划分资源时需要逐个遍历，取用同一份表
// 可以保证资源定义与候选生成用的白名单不会分叉。
[[nodiscard]] const std::vector<NodeType>& all_target_node_types() noexcept;

struct DynamicCostModel
{
    int walk_cost_per_edge = 1;
    std::unordered_map<MovementKind, int> movement_cost_overrides;
    std::unordered_map<std::string, int> action_cost_overrides;
    std::uint64_t revision = 0;

    [[nodiscard]] int movement_cost(const MovementSpec& movement, std::size_t walked_edges = 0) const noexcept;
    [[nodiscard]] int action_cost(std::string_view action_id, int fallback) const noexcept;
    bool clear_action_cost_overrides() noexcept;
    [[nodiscard]] bool validate(std::string* error = nullptr) const;
};

[[nodiscard]] int action_points_after(int current, int cost, int gain) noexcept;

[[nodiscard]] const std::vector<MovementSpec>& movement_specs();
[[nodiscard]] const MovementSpec* find_movement_spec(MovementKind kind) noexcept;
[[nodiscard]] bool is_in_geometric_range(
    const MapSnapshot& map,
    NodeId source,
    NodeId target,
    const MovementSpec& movement,
    GraphLayer layer = GraphLayer::Confirmed);
[[nodiscard]] std::vector<NodeId> enumerate_geometric_targets(
    const MapSnapshot& map,
    NodeId source,
    const MovementSpec& movement,
    GraphLayer layer = GraphLayer::Confirmed);

struct RunResources
{
    int action_points = 0;
    int hope = 0;
    int ingots = 0;
    int seeds = 0;
    int sellable_scraps = 0;
    int white_model_birds = 0;
    bool painted_liberi = false;
    std::unordered_map<MovementKind, int> movement_charges;
    std::unordered_map<MovementKind, int> movement_pieces;

    bool operator==(const RunResources&) const noexcept = default;
};

struct RunState
{
    int floor = 0;
    NodeId current_node = InvalidNodeId;
    RunResources resources;
    std::optional<MovementKind> active_movement;
    DynamicCostModel costs;
    std::uint64_t resources_revision = 0;
    std::unordered_set<NodeId> visited_nodes;
    std::unordered_set<NodeId> consumed_one_time_nodes;
    std::unordered_set<NodeId> revealed_nodes;
    std::unordered_set<MovementKind> cross_floor_expired;
    std::unordered_map<NodeId, NodeProgress> node_progress;
    bool strategy_terminal = false;
};

struct MoveCandidate
{
    std::string action_id;
    MovementKind movement = MovementKind::Walk;
    NodeId source = InvalidNodeId;
    NodeId target = InvalidNodeId;
    NodeId landing = InvalidNodeId;
    std::vector<NodeId> path;
    std::vector<NodeId> possible_landings;
    std::unordered_map<NodeId, int> landing_action_point_gains;
    int predicted_action_point_cost = 0;
    int predicted_action_point_gain = 0;
    int action_point_requirement = std::numeric_limits<int>::max() / 4;
    bool controllable = true;
    bool terminal_on_completion = false;
    bool requires_preview_verification = false;
    GraphLayer graph_layer = GraphLayer::Confirmed;
    bool uses_unconfirmed_edge = false;
    bool uses_inferred_edge = false;
    std::optional<NodeId> first_unclassified;
};

struct MoveAction
{
    MoveCandidate candidate;
    std::vector<NodeId> possible_landings;
};

[[nodiscard]] NodeId resolve_landing(const MapSnapshot& map, NodeId target) noexcept;
[[nodiscard]] std::vector<MoveAction>
    enumerate_move_actions(const MapSnapshot& map, const RunState& state, GraphLayer layer = GraphLayer::Confirmed);

enum class PreviewReachability
{
    Unknown,
    Reachable,
    Blocked,
    InsufficientActionPoints,
    TargetStateChanged,
};

struct MovePreview
{
    PreviewReachability reachability = PreviewReachability::Unknown;
    int exact_action_point_cost = 0;
    NodeType displayed_type = NodeType::Unknown;
    std::string displayed_name;
    bool identity_revealed = false;
};

enum class MoveTransactionStage
{
    Proposed,
    Previewed,
    Committed,
    PageResolved,
    Observed,
    Applied,
    Cancelled,
    Invalidated,
};

struct MoveObservation
{
    NodeId current_node = InvalidNodeId;
    int floor = 0;
    int action_points = 0;
    NodeProgress target_progress = NodeProgress::Active;
    NodeType landed_type = NodeType::Unknown;
    std::uint64_t map_revision = 0;
    std::uint64_t viewport_revision = 0;
};

class MoveTransaction
{
public:
    static std::optional<MoveTransaction> propose(
        MoveCandidate proposal,
        const MapSnapshot& map,
        const ViewportObservation& viewport,
        std::string* error = nullptr);

    [[nodiscard]] bool record_preview(MovePreview preview, std::string* error = nullptr);
    [[nodiscard]] bool commit(
        std::uint64_t current_map_revision,
        std::uint64_t current_viewport_revision,
        std::string* error = nullptr);
    [[nodiscard]] bool mark_page_resolved(std::string* error = nullptr);
    [[nodiscard]] bool observe(MoveObservation observation, std::string* error = nullptr);
    [[nodiscard]] bool apply(RunState& state, std::string* error = nullptr);
    void cancel() noexcept;
    void invalidate() noexcept;

    [[nodiscard]] const MoveCandidate& proposal() const noexcept { return m_proposal; }

    [[nodiscard]] const std::optional<MovePreview>& preview() const noexcept { return m_preview; }

    [[nodiscard]] MoveTransactionStage stage() const noexcept { return m_stage; }

    [[nodiscard]] int authoritative_cost() const noexcept;

    [[nodiscard]] std::uint64_t map_revision() const noexcept { return m_map_revision; }

    [[nodiscard]] std::uint64_t viewport_revision() const noexcept { return m_viewport_revision; }

private:
    MoveCandidate m_proposal;
    std::optional<MovePreview> m_preview;
    std::optional<MoveObservation> m_observation;
    MoveTransactionStage m_stage = MoveTransactionStage::Proposed;
    int m_source_floor = 0;
    NodeType m_target_type = NodeType::Unknown;
    std::uint64_t m_map_revision = 0;
    std::uint64_t m_viewport_revision = 0;
};

[[nodiscard]] std::string_view to_string(NodeType type) noexcept;
[[nodiscard]] std::string_view to_string(MovementKind kind) noexcept;
} // namespace asst::blackflow

