#include "BlackFlowModel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <queue>
#include <set>
#include <tuple>
#include <utility>

namespace asst::blackflow
{
namespace
{
const std::vector<NodeType> AllTargetTypes = {
    NodeType::Unknown,    NodeType::BattleElite, NodeType::BattleNormal, NodeType::BattleSavage,  NodeType::Door,
    NodeType::Employ,     NodeType::Expedition,  NodeType::HideBattle,   NodeType::HideInvisible, NodeType::Incident,
    NodeType::Light,      NodeType::Portal,      NodeType::Rest,         NodeType::Sacrifice,     NodeType::ScrapShop,
    NodeType::Shop,       NodeType::Wish,        NodeType::Empty,        NodeType::Evacuate,      NodeType::Final,
    NodeType::BattleBoss,
};

const std::vector<NodeType> NonCombatTargetTypes = {
    NodeType::Door,  NodeType::Employ, NodeType::Expedition, NodeType::HideInvisible, NodeType::Incident,
    NodeType::Light, NodeType::Portal, NodeType::Rest,       NodeType::Sacrifice,     NodeType::ScrapShop,
    NodeType::Shop,  NodeType::Wish,   NodeType::Empty,      NodeType::Evacuate,      NodeType::Final,
};

const std::vector<NodeType> ShopTargetTypes = {
    NodeType::ScrapShop,
    NodeType::Shop,
};

bool same_edge(const Edge& lhs, const Edge& rhs) noexcept
{
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

Edge normalized_edge(Edge edge) noexcept
{
    if (edge.second < edge.first) {
        std::swap(edge.first, edge.second);
    }
    return edge;
}

bool edge_visible_in_layer(const Edge& edge, GraphLayer layer) noexcept
{
    if (edge.knowledge == EdgeKnowledge::Absent) {
        return false;
    }
    if (layer == GraphLayer::Relaxed) {
        return true;
    }
    return edge.knowledge == EdgeKnowledge::Confirmed && !edge.evidence.forced_by_connectivity_constraint;
}

NodeProgress effective_progress(const Node& node, const RunState& state) noexcept
{
    const auto found = state.node_progress.find(node.id);
    return found == state.node_progress.end() ? node.progress : found->second;
}

NodeType effective_node_type(const Node& node, const RunState& state) noexcept
{
    if (effective_progress(node, state) == NodeProgress::Completed && !node.traversal.repeatable &&
        node.type != NodeType::Empty) {
        return NodeType::Empty;
    }
    return node.type;
}

bool is_targetable(const Node& node, const RunState& state) noexcept
{
    const NodeProgress progress = effective_progress(node, state);
    if (!node.traversal.enterable || progress == NodeProgress::Removed || node.type == NodeType::Empty) {
        return false;
    }
    return progress != NodeProgress::Completed || node.traversal.repeatable;
}

bool is_walk_transparent(const Node& node, const RunState& state, GraphLayer layer) noexcept
{
    return !node.traversal.blocks_walk || state.visited_nodes.contains(node.id) ||
           effective_progress(node, state) == NodeProgress::Completed ||
           (layer == GraphLayer::Relaxed && node.identity_state == NodeIdentityState::Unclassified);
}

int predicted_node_gain(const Node& node, const RunState& state) noexcept
{
    return node.type == NodeType::Light && !state.consumed_one_time_nodes.contains(node.id) ? 1 : 0;
}

std::uint32_t encode_grid_component(int value, bool* ok) noexcept
{
    constexpr int Minimum = -4'194'304;
    constexpr int Maximum = 4'194'303;
    if (value < Minimum || value > Maximum) {
        *ok = false;
        return 0;
    }
    const std::int64_t wide = value;
    return static_cast<std::uint32_t>((wide << 1) ^ (wide >> 63));
}
} // namespace

std::size_t GridPositionHash::operator()(const GridPosition& position) const noexcept
{
    std::size_t seed = std::hash<int> {}(position.row);
    seed ^= std::hash<int> {}(position.column) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
}

std::optional<NodeId> make_stable_node_id(int floor, GridPosition position) noexcept
{
    if (floor < 0 || floor > 65534) {
        return std::nullopt;
    }
    bool valid = true;
    const std::uint32_t row = encode_grid_component(position.row, &valid);
    const std::uint32_t column = encode_grid_component(position.column, &valid);
    if (!valid || row >= (1U << 24U) || column >= (1U << 24U)) {
        return std::nullopt;
    }
    return (static_cast<NodeId>(floor) << 48U) | (static_cast<NodeId>(row) << 24U) | column;
}

bool MapSnapshot::upsert_node(Node node)
{
    if (node.id == InvalidNodeId) {
        return false;
    }
    const auto expected = make_stable_node_id(node.floor, node.position);
    if (!expected.has_value() || *expected != node.id) {
        return false;
    }
    const auto existing = m_nodes.find(node.id);
    if (existing != m_nodes.end() && existing->second == node) {
        return true;
    }
    m_nodes.insert_or_assign(node.id, std::move(node));
    ++revision;
    return true;
}

bool MapSnapshot::remove_node(NodeId id)
{
    if (m_nodes.erase(id) == 0) {
        return false;
    }
    std::erase_if(m_edges, [id](const Edge& edge) { return edge.first == id || edge.second == id; });
    for (auto& [other_id, node] : m_nodes) {
        (void)other_id;
        if (node.transfer_target == id) {
            node.transfer_target.reset();
        }
    }
    ++revision;
    return true;
}

bool MapSnapshot::upsert_edge(Edge edge)
{
    edge = normalized_edge(edge);
    if (edge.first == InvalidNodeId || edge.second == InvalidNodeId || edge.first == edge.second ||
        !m_nodes.contains(edge.first) || !m_nodes.contains(edge.second)) {
        return false;
    }
    if (auto iter = std::ranges::find_if(m_edges, [&](const Edge& current) { return same_edge(current, edge); });
        iter != m_edges.end()) {
        if (iter->knowledge != edge.knowledge || iter->evidence.probability != edge.evidence.probability ||
            iter->evidence.cnn_connected != edge.evidence.cnn_connected ||
            iter->evidence.forced_by_connectivity_constraint != edge.evidence.forced_by_connectivity_constraint ||
            iter->evidence.decision_source != edge.evidence.decision_source) {
            *iter = std::move(edge);
            ++revision;
        }
    }
    else {
        m_edges.emplace_back(edge);
        ++revision;
    }
    return true;
}

const Node* MapSnapshot::find_node(NodeId id) const noexcept
{
    const auto iter = m_nodes.find(id);
    return iter == m_nodes.end() ? nullptr : &iter->second;
}

const Node* MapSnapshot::find_node(int floor, GridPosition position) const noexcept
{
    const auto id = make_stable_node_id(floor, position);
    return id.has_value() ? find_node(*id) : nullptr;
}

EdgeKnowledge MapSnapshot::edge_knowledge(NodeId first, NodeId second) const noexcept
{
    const Edge* edge = find_edge(first, second);
    return edge == nullptr ? EdgeKnowledge::Unknown : edge->knowledge;
}

const Edge* MapSnapshot::find_edge(NodeId first, NodeId second) const noexcept
{
    const Edge key = normalized_edge({ first, second, EdgeKnowledge::Unknown, {} });
    const auto iter = std::ranges::find_if(m_edges, [&](const Edge& edge) { return same_edge(edge, key); });
    return iter == m_edges.end() ? nullptr : &*iter;
}

std::vector<NodeId> MapSnapshot::neighbors(NodeId id, GraphLayer layer) const
{
    std::vector<NodeId> result;
    for (const auto& edge : m_edges) {
        if (!edge_visible_in_layer(edge, layer)) {
            continue;
        }
        if (edge.first == id) {
            result.emplace_back(edge.second);
        }
        else if (edge.second == id) {
            result.emplace_back(edge.first);
        }
    }
    std::ranges::sort(result);
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

std::unordered_set<NodeId> MapSnapshot::reveal_through_transparent_nodes(NodeId origin) const
{
    std::unordered_set<NodeId> revealed;
    if (!m_nodes.contains(origin)) {
        return revealed;
    }
    std::queue<NodeId> pending;
    pending.emplace(origin);
    revealed.emplace(origin);
    while (!pending.empty()) {
        const NodeId current = pending.front();
        pending.pop();
        for (const NodeId neighbor : neighbors(current)) {
            const Node* node = find_node(neighbor);
            if (node == nullptr || node->progress == NodeProgress::Removed) {
                continue;
            }
            const bool inserted = revealed.emplace(neighbor).second;
            if (inserted && !node->traversal.blocks_vision) {
                pending.emplace(neighbor);
            }
        }
    }
    return revealed;
}

std::unordered_set<NodeId> MapSnapshot::nodes_within_manhattan(NodeId origin, int distance) const
{
    std::unordered_set<NodeId> result;
    const Node* center = find_node(origin);
    if (center == nullptr || distance < 0) {
        return result;
    }
    for (const auto& [id, node] : m_nodes) {
        if (node.progress == NodeProgress::Removed || node.floor != center->floor) {
            continue;
        }
        const int manhattan = std::abs(node.position.row - center->position.row) +
                              std::abs(node.position.column - center->position.column);
        if (manhattan <= distance) {
            result.emplace(id);
        }
    }
    return result;
}

bool MapSnapshot::has_valid_transfer_pair(NodeId id) const noexcept
{
    const Node* first = find_node(id);
    if (first == nullptr || !is_transfer_node(first->type) || !first->transfer_target.has_value() ||
        *first->transfer_target == id) {
        return false;
    }
    const Node* second = find_node(*first->transfer_target);
    return second != nullptr && is_transfer_node(second->type) && second->transfer_target == id;
}

bool MapSnapshot::validate(std::string* error) const
{
    std::set<std::tuple<int, int, int>> positions;
    for (const auto& [id, node] : m_nodes) {
        const auto expected = make_stable_node_id(node.floor, node.position);
        if (id == InvalidNodeId || node.id != id || !expected.has_value() || *expected != id) {
            if (error != nullptr) {
                *error = "node id is invalid or does not match floor and grid position";
            }
            return false;
        }
        if (!positions.emplace(node.floor, node.position.row, node.position.column).second) {
            if (error != nullptr) {
                *error = "multiple nodes occupy the same floor and grid position";
            }
            return false;
        }
        if (node.transfer_target == id) {
            if (error != nullptr) {
                *error = "transfer pairing must be non-self";
            }
            return false;
        }
    }
    std::set<std::pair<NodeId, NodeId>> edge_keys;
    for (const auto& edge_value : m_edges) {
        const Edge edge = normalized_edge(edge_value);
        if (edge.first == edge.second || !m_nodes.contains(edge.first) || !m_nodes.contains(edge.second)) {
            if (error != nullptr) {
                *error = "edge has an invalid endpoint";
            }
            return false;
        }
        if (!edge_keys.emplace(edge.first, edge.second).second) {
            if (error != nullptr) {
                *error = "duplicate edge";
            }
            return false;
        }
        if (edge.knowledge == EdgeKnowledge::Confirmed && edge.evidence.forced_by_connectivity_constraint) {
            if (error != nullptr) {
                *error = "connectivity-constraint edge cannot be confirmed";
            }
            return false;
        }
    }
    return true;
}

bool NormalizedMap::merge(const MapObservationBatch& batch, std::string* error)
{
    if (batch.floor < 1) {
        if (error != nullptr) {
            *error = "observation floor must be positive";
        }
        return false;
    }
    NormalizedMap working = *this;
    if (working.m_floor != 0 && working.m_floor != batch.floor) {
        working.reset();
    }
    working.m_floor = batch.floor;

    std::unordered_set<GridPosition, GridPositionHash> observed_positions;
    for (const auto& observed : batch.nodes) {
        const auto id = make_stable_node_id(batch.floor, observed.position);
        if (!id.has_value() || !observed_positions.emplace(observed.position).second) {
            if (error != nullptr) {
                *error = "observation contains an invalid or duplicate grid position";
            }
            return false;
        }

        const Node* current = working.m_snapshot.find_node(*id);
        Node node = current == nullptr ? Node {} : *current;
        if (current == nullptr) {
            node.id = *id;
            node.floor = batch.floor;
            node.position = observed.position;
            node.traversal = default_traversal_for(NodeType::Unknown);
        }
        if (observed.type.has_value()) {
            const bool newly_classified = node.type == NodeType::Unknown && *observed.type != NodeType::Unknown;
            node.type = *observed.type;
            if (!observed.traversal.has_value() &&
                (current == nullptr || newly_classified || current->type != *observed.type)) {
                node.traversal = default_traversal_for(node.type);
            }
        }
        if (observed.name.has_value()) {
            node.name = *observed.name;
        }
        if (observed.progress.has_value()) {
            node.progress = *observed.progress;
        }
        if (observed.traversal.has_value()) {
            node.traversal = *observed.traversal;
        }
        if (observed.identity_state.has_value()) {
            node.identity_state = *observed.identity_state;
        }
        if (observed.identity_revealed.has_value()) {
            node.identity_revealed = *observed.identity_revealed;
        }
        if (observed.badged.has_value()) {
            node.badged = *observed.badged;
        }
        if (observed.transfer_target.has_value()) {
            if (observed.transfer_target->has_value()) {
                node.transfer_target = make_stable_node_id(batch.floor, **observed.transfer_target);
                if (!node.transfer_target.has_value()) {
                    if (error != nullptr) {
                        *error = "observation contains an invalid transfer target";
                    }
                    return false;
                }
            }
            else {
                node.transfer_target.reset();
            }
        }
        if (!working.m_snapshot.upsert_node(std::move(node))) {
            if (error != nullptr) {
                *error = "failed to merge observed node";
            }
            return false;
        }
    }

    std::unordered_set<GridPosition, GridPositionHash> covered(
        batch.covered_positions.begin(),
        batch.covered_positions.end());
    if (batch.coverage == ObservationCoverage::FullMap && covered.empty()) {
        for (const auto& [id, node] : working.m_snapshot.nodes()) {
            (void)id;
            if (node.floor == batch.floor) {
                covered.emplace(node.position);
            }
        }
    }
    for (const GridPosition& position : covered) {
        if (observed_positions.contains(position)) {
            continue;
        }
        const auto id = make_stable_node_id(batch.floor, position);
        if (!id.has_value()) {
            if (error != nullptr) {
                *error = "covered observation contains an invalid grid position";
            }
            return false;
        }
        const Node* current = working.m_snapshot.find_node(*id);
        if (current == nullptr) {
            continue;
        }
        Node empty = *current;
        empty.type = NodeType::Empty;
        empty.name.clear();
        empty.progress = NodeProgress::Active;
        empty.traversal = default_traversal_for(NodeType::Empty);
        empty.identity_state = NodeIdentityState::Classified;
        empty.identity_revealed = true;
        empty.badged = false;
        empty.transfer_target.reset();
        working.m_snapshot.upsert_node(std::move(empty));
    }

    std::set<std::pair<NodeId, NodeId>> observed_edges;
    for (const auto& observed : batch.edges) {
        const auto first = make_stable_node_id(batch.floor, observed.first);
        const auto second = make_stable_node_id(batch.floor, observed.second);
        if (!first.has_value() || !second.has_value()) {
            if (error != nullptr) {
                *error = "observation edge contains an invalid grid position";
            }
            return false;
        }
        const Edge normalized = normalized_edge({ *first, *second, observed.knowledge, observed.evidence });
        if (!observed_edges.emplace(normalized.first, normalized.second).second ||
            !working.m_snapshot.upsert_edge(normalized)) {
            if (error != nullptr) {
                *error = "observation edge endpoints must reference merged nodes and be unique";
            }
            return false;
        }
    }

    std::vector<Edge> missing_edges;
    for (const auto& edge : working.m_snapshot.edges()) {
        if (observed_edges.contains({ edge.first, edge.second })) {
            continue;
        }
        const Node* first = working.m_snapshot.find_node(edge.first);
        const Node* second = working.m_snapshot.find_node(edge.second);
        const bool edge_covered = batch.coverage == ObservationCoverage::FullMap ||
                                  (first != nullptr && second != nullptr && covered.contains(first->position) &&
                                   covered.contains(second->position));
        if (edge_covered && edge.knowledge != EdgeKnowledge::Absent) {
            Edge absent = edge;
            absent.knowledge = EdgeKnowledge::Absent;
            missing_edges.emplace_back(std::move(absent));
        }
    }
    for (const Edge& edge : missing_edges) {
        working.m_snapshot.upsert_edge(edge);
    }
    *this = std::move(working);
    return true;
}

void NormalizedMap::reset()
{
    m_floor = 0;
    m_snapshot = MapSnapshot {};
}

void ViewportObservation::replace(
    std::vector<NodeObservation> observations,
    std::uint64_t map_revision,
    std::uint64_t viewport_revision)
{
    m_nodes.clear();
    for (auto& observation : observations) {
        if (observation.node != InvalidNodeId && !observation.icon_rect.empty()) {
            m_nodes.insert_or_assign(observation.node, std::move(observation));
        }
    }
    m_map_revision = map_revision;
    m_viewport_revision = viewport_revision;
}

void ViewportObservation::clear(std::uint64_t map_revision, std::uint64_t viewport_revision)
{
    m_nodes.clear();
    m_map_revision = map_revision;
    m_viewport_revision = viewport_revision;
}

const NodeObservation* ViewportObservation::find(NodeId node) const noexcept
{
    const auto iter = m_nodes.find(node);
    return iter == m_nodes.end() ? nullptr : &iter->second;
}

std::optional<Rect> ViewportObservation::clickable_rect(
    NodeId node,
    std::uint64_t expected_map_revision,
    std::uint64_t expected_viewport_revision) const
{
    if (expected_map_revision != m_map_revision || expected_viewport_revision != m_viewport_revision) {
        return std::nullopt;
    }
    const NodeObservation* observation = find(node);
    return observation == nullptr ? std::nullopt : std::optional<Rect>(observation->icon_rect);
}

NodeTraversal default_traversal_for(NodeType type) noexcept
{
    switch (type) {
    case NodeType::Empty:
        return { false, false, false, false };
    case NodeType::Door:
        return { false, false, true, true };
    case NodeType::Shop:
    case NodeType::ScrapShop:
        return { true, true, true, true };
    default:
        return {};
    }
}

bool is_transfer_node(NodeType type) noexcept
{
    return type == NodeType::Door;
}

bool is_combat_node_type(NodeType type) noexcept
{
    return type == NodeType::BattleNormal || type == NodeType::BattleElite || type == NodeType::BattleSavage ||
           type == NodeType::HideBattle || type == NodeType::BattleBoss;
}

std::optional<NodeType> node_type_from_string(std::string_view value) noexcept
{
    static const std::unordered_map<std::string_view, NodeType> Mapping = {
        { "unclassified", NodeType::Unknown },
        { "battle_elite", NodeType::BattleElite },
        { "battle_normal", NodeType::BattleNormal },
        { "battle_savage", NodeType::BattleSavage },
        { "door", NodeType::Door },
        { "employ", NodeType::Employ },
        { "expedition", NodeType::Expedition },
        { "hide_battle", NodeType::HideBattle },
        { "hide_invisible", NodeType::HideInvisible },
        { "incident", NodeType::Incident },
        { "light", NodeType::Light },
        { "portal", NodeType::Portal },
        { "rest", NodeType::Rest },
        { "sacrifice", NodeType::Sacrifice },
        { "scrap_shop", NodeType::ScrapShop },
        { "shop", NodeType::Shop },
        { "wish", NodeType::Wish },
        { "empty", NodeType::Empty },
        { "evacuate", NodeType::Evacuate },
        { "final", NodeType::Final },
        { "battle_boss", NodeType::BattleBoss },
    };
    const auto found = Mapping.find(value);
    return found == Mapping.end() ? std::nullopt : std::optional<NodeType>(found->second);
}

bool node_type_allowed(const MovementSpec& movement, NodeType type) noexcept
{
    return std::ranges::find(movement.target_types, type) != movement.target_types.end();
}

const std::vector<MovementSpec>& movement_specs()
{
    static const std::vector<MovementSpec> Specs = {
        { MovementKind::Walk, "walk", "徒步跋涉", MovementRange::WalkEdges, AllTargetTypes, 1, -1, false, false, {} },
        { MovementKind::M01,
          "rogue_6_scrap_M_01",
          "报废轮子",
          MovementRange::OrthogonalTwo,
          AllTargetTypes,
          1,
          1,
          false,
          false,
          {} },
        { MovementKind::M02,
          "rogue_6_scrap_M_02",
          "报废假肢",
          MovementRange::SurroundingEight,
          AllTargetTypes,
          1,
          1,
          false,
          false,
          {} },
        { MovementKind::M03,
          "rogue_6_scrap_M_03",
          "标准引擎",
          MovementRange::SurroundingEight,
          AllTargetTypes,
          1,
          3,
          false,
          false,
          {} },
        { MovementKind::M04,
          "rogue_6_scrap_M_04",
          "重弹簧",
          MovementRange::ManhattanTwo,
          AllTargetTypes,
          0,
          1,
          false,
          true,
          {} },
        { MovementKind::M05,
          "rogue_6_scrap_M_05",
          "气垫底座",
          MovementRange::ManhattanTwo,
          AllTargetTypes,
          1,
          2,
          false,
          false,
          {} },
        { MovementKind::M06,
          "rogue_6_scrap_M_06",
          "试作外骨骼",
          MovementRange::OrthogonalThree,
          AllTargetTypes,
          1,
          2,
          false,
          false,
          {} },
        { MovementKind::M07,
          "rogue_6_scrap_M_07",
          "小八界",
          MovementRange::FullMap,
          NonCombatTargetTypes,
          0,
          1,
          true,
          true,
          {} },
        { MovementKind::M08,
          "rogue_6_scrap_M_08",
          "一次性喷气背包",
          MovementRange::FullMap,
          AllTargetTypes,
          0,
          1,
          false,
          false,
          {} },
        { MovementKind::M09,
          "rogue_6_scrap_M_09",
          "老妈妈的融雪",
          MovementRange::FullMap,
          NonCombatTargetTypes,
          1,
          1,
          false,
          false,
          { 0, 2, 0 } },
        { MovementKind::M10,
          "rogue_6_scrap_M_10",
          "坎诺特的触须",
          MovementRange::FullMap,
          ShopTargetTypes,
          1,
          2,
          false,
          false,
          { 0, 0, 4 } },
        { MovementKind::M11,
          "rogue_6_scrap_M_11",
          "结构性原理",
          MovementRange::FullMap,
          AllTargetTypes,
          1,
          3,
          false,
          false,
          {} },
        { MovementKind::M12,
          "rogue_6_scrap_M_12",
          "简易遥控器",
          MovementRange::SurroundingEight,
          AllTargetTypes,
          0,
          1,
          false,
          false,
          { 3, 0, 0 } },
    };
    return Specs;
}

int action_points_after(int current, int cost, int gain) noexcept
{
    const std::int64_t result = static_cast<std::int64_t>(current) - cost + gain;
    return static_cast<int>(
        std::clamp<std::int64_t>(result, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

const MovementSpec* find_movement_spec(MovementKind kind) noexcept
{
    const auto& specs = movement_specs();
    const auto iter = std::ranges::find_if(specs, [&](const MovementSpec& spec) { return spec.kind == kind; });
    return iter == specs.end() ? nullptr : &*iter;
}

int DynamicCostModel::movement_cost(const MovementSpec& movement, std::size_t walked_edges) const noexcept
{
    if (movement.kind == MovementKind::Walk) {
        const std::int64_t cost = static_cast<std::int64_t>(walk_cost_per_edge) * walked_edges;
        return static_cast<int>(std::clamp<std::int64_t>(cost, 0, std::numeric_limits<int>::max()));
    }
    const auto override_value = movement_cost_overrides.find(movement.kind);
    return override_value == movement_cost_overrides.end() ? movement.action_point_cost : override_value->second;
}

int DynamicCostModel::action_cost(std::string_view action_id, int fallback) const noexcept
{
    const auto found = action_cost_overrides.find(std::string(action_id));
    return found == action_cost_overrides.end() ? fallback : found->second;
}

bool DynamicCostModel::clear_action_cost_overrides() noexcept
{
    if (action_cost_overrides.empty()) {
        return false;
    }
    action_cost_overrides.clear();
    ++revision;
    return true;
}

bool DynamicCostModel::validate(std::string* error) const
{
    if (walk_cost_per_edge < 0) {
        if (error != nullptr) {
            *error = "walk edge cost must be non-negative";
        }
        return false;
    }
    for (const auto& [movement, cost] : movement_cost_overrides) {
        if (movement == MovementKind::Walk || find_movement_spec(movement) == nullptr || cost < 0) {
            if (error != nullptr) {
                *error = "movement cost override is invalid";
            }
            return false;
        }
    }
    for (const auto& [action_id, cost] : action_cost_overrides) {
        if (action_id.empty() || cost < 0) {
            if (error != nullptr) {
                *error = "action cost override is invalid";
            }
            return false;
        }
    }
    return true;
}

bool is_in_geometric_range(
    const MapSnapshot& map,
    NodeId source,
    NodeId target,
    const MovementSpec& movement,
    GraphLayer layer)
{
    if (source == target) {
        return false;
    }
    const Node* source_node = map.find_node(source);
    const Node* target_node = map.find_node(target);
    if (source_node == nullptr || target_node == nullptr || target_node->progress == NodeProgress::Removed ||
        source_node->floor != target_node->floor) {
        return false;
    }
    const int row_delta = std::abs(target_node->position.row - source_node->position.row);
    const int column_delta = std::abs(target_node->position.column - source_node->position.column);
    switch (movement.range) {
    case MovementRange::WalkEdges: {
        const Edge* edge = map.find_edge(source, target);
        return edge != nullptr && edge_visible_in_layer(*edge, layer);
    }
    case MovementRange::OrthogonalTwo:
        return (row_delta == 0 || column_delta == 0) && row_delta + column_delta <= 2;
    case MovementRange::SurroundingEight:
        return row_delta <= 1 && column_delta <= 1;
    case MovementRange::ManhattanTwo:
        return row_delta + column_delta <= 2;
    case MovementRange::OrthogonalThree:
        return (row_delta == 0 || column_delta == 0) && row_delta + column_delta <= 3;
    case MovementRange::FullMap:
        return true;
    }
    return false;
}

std::vector<NodeId>
    enumerate_geometric_targets(const MapSnapshot& map, NodeId source, const MovementSpec& movement, GraphLayer layer)
{
    const RunState empty_state;
    std::vector<NodeId> result;
    for (const auto& [id, node] : map.nodes()) {
        if (id != source && is_targetable(node, empty_state) &&
            is_in_geometric_range(map, source, id, movement, layer)) {
            result.emplace_back(id);
        }
    }
    std::ranges::sort(result);
    return result;
}

NodeId resolve_landing(const MapSnapshot& map, NodeId target) noexcept
{
    const Node* node = map.find_node(target);
    if (node == nullptr || !is_transfer_node(node->type)) {
        return target;
    }
    return map.has_valid_transfer_pair(target) ? *node->transfer_target : InvalidNodeId;
}

std::vector<MoveAction> enumerate_move_actions(const MapSnapshot& map, const RunState& state, GraphLayer layer)
{
    std::vector<MoveAction> result;
    if (state.resources.action_points < 1 || map.find_node(state.current_node) == nullptr) {
        return result;
    }

    const MovementSpec* walk = find_movement_spec(MovementKind::Walk);
    if (walk != nullptr) {
        struct WalkFrontier
        {
            NodeId node = InvalidNodeId;
            std::vector<NodeId> path;
        };

        std::deque<WalkFrontier> queue;
        std::unordered_map<NodeId, std::size_t> walk_action_indices;
        std::unordered_set<NodeId> expanded;
        queue.push_back({ state.current_node, {} });
        expanded.emplace(state.current_node);
        while (!queue.empty()) {
            WalkFrontier current = std::move(queue.front());
            queue.pop_front();
            for (const NodeId neighbor : map.neighbors(current.node, layer)) {
                if (std::ranges::find(current.path, neighbor) != current.path.end() || neighbor == state.current_node) {
                    continue;
                }
                const Node* node = map.find_node(neighbor);
                if (node == nullptr || effective_progress(*node, state) == NodeProgress::Removed) {
                    continue;
                }
                auto path = current.path;
                path.emplace_back(neighbor);
                if (is_targetable(*node, state) && node_type_allowed(*walk, node->type)) {
                    MoveAction action;
                    action.candidate.action_id =
                        "walk:" + std::to_string(state.current_node) + ":" + std::to_string(neighbor);
                    action.candidate.movement = MovementKind::Walk;
                    action.candidate.source = state.current_node;
                    action.candidate.target = neighbor;
                    action.candidate.landing = resolve_landing(map, neighbor);
                    action.candidate.graph_layer = layer;
                    if (action.candidate.landing != InvalidNodeId) {
                        action.candidate.path = path;
                        NodeId previous = state.current_node;
                        for (const NodeId step : path) {
                            const Edge* path_edge = map.find_edge(previous, step);
                            if (path_edge != nullptr) {
                                action.candidate.uses_unconfirmed_edge =
                                    action.candidate.uses_unconfirmed_edge ||
                                    path_edge->knowledge != EdgeKnowledge::Confirmed ||
                                    path_edge->evidence.forced_by_connectivity_constraint;
                                action.candidate.uses_inferred_edge =
                                    action.candidate.uses_inferred_edge ||
                                    path_edge->evidence.forced_by_connectivity_constraint;
                            }
                            const Node* path_node = map.find_node(step);
                            if (!action.candidate.first_unclassified.has_value() && path_node != nullptr &&
                                path_node->identity_state == NodeIdentityState::Unclassified) {
                                action.candidate.first_unclassified = step;
                            }
                            previous = step;
                        }
                        action.candidate.predicted_action_point_cost = state.costs.action_cost(
                            action.candidate.action_id,
                            state.costs.movement_cost(*walk, path.size()));
                        action.candidate.predicted_action_point_gain = predicted_node_gain(*node, state);
                        action.candidate.possible_landings.emplace_back(action.candidate.landing);
                        action.candidate.landing_action_point_gains.emplace(
                            action.candidate.landing,
                            action.candidate.predicted_action_point_gain);
                        action.candidate.terminal_on_completion = node->type == NodeType::Final;
                        action.possible_landings.emplace_back(action.candidate.landing);
                        const auto existing_action = walk_action_indices.find(neighbor);
                        if (existing_action == walk_action_indices.end()) {
                            walk_action_indices.emplace(neighbor, result.size());
                            result.emplace_back(std::move(action));
                        }
                        else if (action.candidate.path.size() < result[existing_action->second].candidate.path.size()) {
                            result[existing_action->second] = std::move(action);
                        }
                    }
                }
                if (is_walk_transparent(*node, state, layer) && expanded.emplace(neighbor).second) {
                    queue.push_back({ neighbor, std::move(path) });
                }
            }
        }
    }

    for (const auto& movement : movement_specs()) {
        if (movement.kind == MovementKind::Walk) {
            continue;
        }
        const auto charge = state.resources.movement_charges.find(movement.kind);
        if (charge == state.resources.movement_charges.end() || charge->second <= 0 ||
            state.cross_floor_expired.contains(movement.kind)) {
            continue;
        }

        std::vector<NodeId> targets;
        for (const auto& [id, node] : map.nodes()) {
            const NodeType target_type = effective_node_type(node, state);
            const bool targetable = target_type == NodeType::Empty
                                        ? effective_progress(node, state) != NodeProgress::Removed
                                        : is_targetable(node, state);
            if (id == state.current_node || !targetable || !node_type_allowed(movement, target_type) ||
                !is_in_geometric_range(map, state.current_node, id, movement, GraphLayer::Confirmed)) {
                continue;
            }
            targets.emplace_back(id);
        }
        std::ranges::sort(targets);
        if (targets.empty()) {
            continue;
        }

        if (movement.random_target) {
            MoveAction action;
            action.candidate.action_id = std::string(movement.id) + ":random:" + std::to_string(state.current_node);
            action.candidate.movement = movement.kind;
            action.candidate.source = state.current_node;
            action.candidate.predicted_action_point_cost =
                state.costs.action_cost(action.candidate.action_id, state.costs.movement_cost(movement));
            action.candidate.predicted_action_point_gain = movement.effect.action_point_gain;
            action.candidate.controllable = false;
            for (const NodeId target : targets) {
                const NodeId landing = resolve_landing(map, target);
                const Node* target_node = map.find_node(target);
                if (landing != InvalidNodeId && target_node != nullptr) {
                    action.possible_landings.emplace_back(landing);
                    action.candidate.landing_action_point_gains.insert_or_assign(
                        landing,
                        movement.effect.action_point_gain + predicted_node_gain(*target_node, state));
                }
            }
            std::ranges::sort(action.possible_landings);
            action.possible_landings.erase(
                std::unique(action.possible_landings.begin(), action.possible_landings.end()),
                action.possible_landings.end());
            if (!action.possible_landings.empty()) {
                action.candidate.possible_landings = action.possible_landings;
                result.emplace_back(std::move(action));
            }
            continue;
        }

        for (const NodeId target : targets) {
            const Node* target_node = map.find_node(target);
            if (target_node == nullptr) {
                continue;
            }
            const NodeId landing = resolve_landing(map, target);
            if (landing == InvalidNodeId) {
                continue;
            }
            MoveAction action;
            action.candidate.action_id = std::string(movement.id) + ":" + std::to_string(state.current_node) + ":" +
                                         std::to_string(target) + ":" + std::to_string(landing);
            action.candidate.movement = movement.kind;
            action.candidate.source = state.current_node;
            action.candidate.target = target;
            action.candidate.landing = landing;
            action.candidate.path = { target };
            action.candidate.predicted_action_point_cost =
                state.costs.action_cost(action.candidate.action_id, state.costs.movement_cost(movement));
            action.candidate.predicted_action_point_gain =
                movement.effect.action_point_gain + predicted_node_gain(*target_node, state);
            action.candidate.possible_landings.emplace_back(landing);
            action.candidate.landing_action_point_gains.emplace(landing, action.candidate.predicted_action_point_gain);
            action.candidate.controllable = true;
            action.candidate.terminal_on_completion = target_node->type == NodeType::Final;
            action.possible_landings.emplace_back(landing);
            result.emplace_back(std::move(action));
        }
    }
    return result;
}

std::optional<MoveTransaction> MoveTransaction::propose(
    MoveCandidate proposal,
    const MapSnapshot& map,
    const ViewportObservation& viewport,
    std::string* error)
{
    if (proposal.source == InvalidNodeId || map.find_node(proposal.source) == nullptr) {
        if (error != nullptr) {
            *error = "move proposal references an invalid source node";
        }
        return std::nullopt;
    }
    if (proposal.controllable && (proposal.target == InvalidNodeId || proposal.landing == InvalidNodeId ||
                                  map.find_node(proposal.target) == nullptr)) {
        if (error != nullptr) {
            *error = "controllable move proposal references an invalid target node";
        }
        return std::nullopt;
    }
    if (!proposal.controllable && proposal.possible_landings.empty()) {
        if (error != nullptr) {
            *error = "uncontrollable move proposal has no possible landing";
        }
        return std::nullopt;
    }
    if (proposal.controllable &&
        !viewport.clickable_rect(proposal.target, map.revision, viewport.viewport_revision()).has_value()) {
        if (error != nullptr) {
            *error = "move proposal has no current viewport coordinate";
        }
        return std::nullopt;
    }
    MoveTransaction transaction;
    transaction.m_source_floor = map.find_node(proposal.source)->floor;
    if (proposal.controllable) {
        const NodeId destination = proposal.landing == InvalidNodeId ? proposal.target : proposal.landing;
        if (const Node* target = map.find_node(destination); target != nullptr) {
            transaction.m_target_type = target->type;
        }
    }
    transaction.m_proposal = std::move(proposal);
    transaction.m_map_revision = map.revision;
    transaction.m_viewport_revision = viewport.viewport_revision();
    return transaction;
}

bool MoveTransaction::record_preview(MovePreview preview, std::string* error)
{
    if (m_stage != MoveTransactionStage::Proposed || preview.exact_action_point_cost < 0) {
        if (error != nullptr) {
            *error = "preview is invalid for the current transaction stage";
        }
        return false;
    }
    if (preview.reachability == PreviewReachability::Unknown) {
        if (error != nullptr) {
            *error = "preview did not determine reachability";
        }
        return false;
    }
    m_preview = std::move(preview);
    if (m_preview->reachability != PreviewReachability::Reachable) {
        m_stage = MoveTransactionStage::Cancelled;
        return true;
    }
    m_stage = MoveTransactionStage::Previewed;
    return true;
}

bool MoveTransaction::commit(
    std::uint64_t current_map_revision,
    std::uint64_t current_viewport_revision,
    std::string* error)
{
    if (m_stage != MoveTransactionStage::Previewed || !m_preview.has_value() ||
        m_preview->reachability != PreviewReachability::Reachable) {
        if (error != nullptr) {
            *error = "only a reachable previewed transaction can be committed";
        }
        return false;
    }
    if (current_map_revision != m_map_revision || current_viewport_revision != m_viewport_revision) {
        m_stage = MoveTransactionStage::Invalidated;
        if (error != nullptr) {
            *error = "map or viewport revision changed before commit";
        }
        return false;
    }
    m_stage = MoveTransactionStage::Committed;
    return true;
}

bool MoveTransaction::mark_page_resolved(std::string* error)
{
    if (m_stage == MoveTransactionStage::PageResolved) {
        return true;
    }
    if (m_stage != MoveTransactionStage::Committed) {
        if (error != nullptr) {
            *error = "node page can only resolve a committed transaction";
        }
        return false;
    }
    m_stage = MoveTransactionStage::PageResolved;
    return true;
}

bool MoveTransaction::observe(MoveObservation observation, std::string* error)
{
    const bool returned_to_same_floor = observation.floor == m_source_floor;
    const bool landing_matches = m_proposal.controllable
                                     ? observation.current_node == m_proposal.landing
                                     : std::ranges::find(m_proposal.possible_landings, observation.current_node) !=
                                           m_proposal.possible_landings.end();
    const bool advanced_after_final = observation.floor == m_source_floor + 1 && m_target_type == NodeType::Final;
    const bool stage_accepts_observation =
        m_stage == MoveTransactionStage::Committed || m_stage == MoveTransactionStage::PageResolved;
    if (!stage_accepts_observation || observation.viewport_revision <= m_viewport_revision ||
        !((returned_to_same_floor && landing_matches) || advanced_after_final)) {
        if (error != nullptr) {
            *error = "next map observation does not match the committed move";
        }
        return false;
    }
    m_observation = std::move(observation);
    m_stage = MoveTransactionStage::Observed;
    return true;
}

int MoveTransaction::authoritative_cost() const noexcept
{
    return m_preview.has_value() && m_preview->reachability == PreviewReachability::Reachable
               ? m_preview->exact_action_point_cost
               : m_proposal.predicted_action_point_cost;
}

bool MoveTransaction::apply(RunState& state, std::string* error)
{
    const RunResources resources_before = state.resources;
    if (m_stage != MoveTransactionStage::Observed || !m_observation.has_value() ||
        state.current_node != m_proposal.source || state.resources.action_points < 1) {
        if (error != nullptr) {
            *error = "transaction cannot be applied to the current run state";
        }
        return false;
    }
    const int cost = authoritative_cost();
    if (cost < 0 || state.resources.action_points < cost) {
        if (error != nullptr) {
            *error = "authoritative move cost exceeds current action points";
        }
        return false;
    }
    const MovementSpec* movement = find_movement_spec(m_proposal.movement);
    if (movement == nullptr) {
        if (error != nullptr) {
            *error = "transaction references an unknown movement";
        }
        return false;
    }
    int action_point_gain = m_proposal.predicted_action_point_gain;
    if (!m_proposal.controllable) {
        const auto gain = m_proposal.landing_action_point_gains.find(m_observation->current_node);
        if (gain == m_proposal.landing_action_point_gains.end()) {
            if (error != nullptr) {
                *error = "uncontrollable move observation has no matching action-point outcome";
            }
            return false;
        }
        action_point_gain = gain->second;
    }
    const int expected_action_points = action_points_after(state.resources.action_points, cost, action_point_gain);
    const bool floor_advanced = m_observation->floor > m_source_floor;
    if (!floor_advanced && m_observation->action_points != expected_action_points) {
        if (error != nullptr) {
            *error = "observed action points do not match authoritative move accounting";
        }
        return false;
    }

    if (m_proposal.movement != MovementKind::Walk) {
        auto charge = state.resources.movement_charges.find(m_proposal.movement);
        if (charge == state.resources.movement_charges.end() || charge->second <= 0) {
            if (error != nullptr) {
                *error = "movement charge was exhausted before transaction application";
            }
            return false;
        }
        --charge->second;
    }
    state.resources.action_points = m_observation->action_points;
    state.resources.hope += movement->effect.hope_gain;
    state.resources.ingots += movement->effect.ingot_gain;
    state.current_node = m_observation->current_node;

    const bool processing_move = m_proposal.movement != MovementKind::Walk;
    if (processing_move && is_combat_node_type(m_observation->landed_type) && state.resources.white_model_birds > 0) {
        --state.resources.white_model_birds;
    }
    if (state.resources != resources_before) {
        ++state.resources_revision;
    }
    m_stage = MoveTransactionStage::Applied;
    return true;
}

void MoveTransaction::cancel() noexcept
{
    if (m_stage != MoveTransactionStage::Applied) {
        m_stage = MoveTransactionStage::Cancelled;
    }
}

void MoveTransaction::invalidate() noexcept
{
    if (m_stage != MoveTransactionStage::Applied) {
        m_stage = MoveTransactionStage::Invalidated;
    }
}

std::string_view to_string(NodeType type) noexcept
{
    switch (type) {
    case NodeType::Unknown:
        return "unclassified";
    case NodeType::BattleElite:
        return "battle_elite";
    case NodeType::BattleNormal:
        return "battle_normal";
    case NodeType::BattleSavage:
        return "battle_savage";
    case NodeType::Door:
        return "door";
    case NodeType::Employ:
        return "employ";
    case NodeType::Expedition:
        return "expedition";
    case NodeType::HideBattle:
        return "hide_battle";
    case NodeType::HideInvisible:
        return "hide_invisible";
    case NodeType::Incident:
        return "incident";
    case NodeType::Light:
        return "light";
    case NodeType::Portal:
        return "portal";
    case NodeType::Rest:
        return "rest";
    case NodeType::Sacrifice:
        return "sacrifice";
    case NodeType::ScrapShop:
        return "scrap_shop";
    case NodeType::Shop:
        return "shop";
    case NodeType::Wish:
        return "wish";
    case NodeType::Empty:
        return "empty";
    case NodeType::Evacuate:
        return "evacuate";
    case NodeType::Final:
        return "final";
    case NodeType::BattleBoss:
        return "battle_boss";
    }
    return "unclassified";
}

std::string_view to_string(MovementKind kind) noexcept
{
    const MovementSpec* spec = find_movement_spec(kind);
    return spec == nullptr ? std::string_view("unknown") : spec->id;
}
} // namespace asst::blackflow

