#include "BlackFlowObservation.h"

#include <algorithm>
#include <unordered_set>

#include "Vision/Roguelike/BlackFlow/BlackFlowFloor.h"

namespace asst::blackflow
{
namespace
{
NodeIdentityState identity_state_for(std::string_view type) noexcept
{
    if (type == "unclassified") {
        return NodeIdentityState::Unclassified;
    }
    if (type == "hide_battle" || type == "hide_invisible") {
        return NodeIdentityState::Hidden;
    }
    return NodeIdentityState::Classified;
}

bool identity_revealed_for(std::string_view type) noexcept
{
    return type != "unclassified" && type != "hide_battle" && type != "hide_invisible";
}

const PerceptionNodeObservation* infer_first_floor_shop(const BlackFlowMapObservation& source) noexcept
{
    if (source.floor != 1) {
        return nullptr;
    }

    const PerceptionNodeObservation* result = nullptr;
    for (const auto& node : source.nodes) {
        if (!node.exists || node.position.row != 1) {
            continue;
        }
        // 第一层第二行的固定商店揭示后直接沿用，避免中途进入时将剩余的未知节点重新推断为商店。
        if (node.type == "shop") {
            return &node;
        }
        if (node.type != "hide_invisible") {
            continue;
        }
        if (result == nullptr || node.position.column > result->position.column) {
            result = &node;
        }
    }
    return result;
}
} // namespace

std::optional<NodeType> BlackFlowObservationAdapter::map_node_type(std::string_view type) noexcept
{
    return node_type_from_string(type);
}

std::vector<GridPosition> BlackFlowObservationAdapter::expected_grid_positions(int floor)
{
    std::vector<GridPosition> result;
    const auto profile = perception::floor_profile(floor);
    if (!profile.has_value()) {
        return result;
    }
    result.reserve(static_cast<std::size_t>(profile->rows * profile->columns));
    for (int row = 0; row < profile->rows; ++row) {
        for (int column = 0; column < profile->columns; ++column) {
            result.emplace_back(GridPosition { row, column });
        }
    }
    return result;
}

std::optional<NormalizedPerceptionObservation>
    BlackFlowObservationAdapter::normalize(const BlackFlowMapObservation& source, std::string* error) const
{
    if (!source.recognition_ok || !source.graph_connected) {
        if (error != nullptr) {
            *error = "perception result failed the recognition or connectivity gate";
        }
        return std::nullopt;
    }
    if (source.floor < 1 || source.current_marker_temporary_id < 0) {
        if (error != nullptr) {
            *error = "perception result has no valid observed floor or current marker";
        }
        return std::nullopt;
    }

    NormalizedPerceptionObservation result;
    result.map.floor = source.floor;
    result.map.coverage = source.coverage;
    result.map.covered_positions = source.covered_positions;
    if (result.map.coverage == ObservationCoverage::FullMap && result.map.covered_positions.empty()) {
        result.map.covered_positions = expected_grid_positions(source.floor);
    }
    result.hud_action_points = source.hud_action_points;
    result.viewport_revision = source.viewport_revision;
    result.summary.observation_id = source.observation_id;
    result.summary.floor = source.floor;
    result.summary.floor_from_ocr = source.floor_from_ocr;
    result.summary.screenshot_us = source.screenshot_us;
    result.summary.recognition_us = source.recognition_us;
    result.summary.attempt_count = source.attempt_count;
    result.summary.retry_count = source.retry_count;

    const PerceptionNodeObservation* inferred_shop = infer_first_floor_shop(source);
    std::unordered_map<int, const PerceptionNodeObservation*> by_temporary_id;
    std::unordered_map<int, NodeId> stable_ids;
    for (const auto& source_node : source.nodes) {
        if (!source_node.exists) {
            continue;
        }
        const auto type = map_node_type(source_node.type);
        const auto stable = make_stable_node_id(source.floor, source_node.position);
        if (!type.has_value() || !stable.has_value() || source_node.temporary_id < 0 ||
            !by_temporary_id.emplace(source_node.temporary_id, &source_node).second) {
            if (error != nullptr) {
                *error = "perception result contains an unknown node type or invalid duplicate node";
            }
            return std::nullopt;
        }
        stable_ids.emplace(source_node.temporary_id, *stable);

        const bool savage_override =
            source_node.marker_type == "savage" &&
            (*type == NodeType::Empty || *type == NodeType::BattleNormal || *type == NodeType::BattleElite ||
             *type == NodeType::HideInvisible || *type == NodeType::HideBattle);
        const bool is_inferred_shop = inferred_shop == &source_node;
        ObservedNode node;
        node.position = source_node.position;
        node.type = is_inferred_shop ? NodeType::Shop : savage_override ? NodeType::BattleNormal : *type;
        node.name = source_node.displayed_name;

        node.identity_state =
            is_inferred_shop || savage_override ? NodeIdentityState::Classified : identity_state_for(source_node.type);
        node.identity_revealed = is_inferred_shop || savage_override || identity_revealed_for(source_node.type);
        node.marker_type = source_node.marker_type;
        node.marker_display_name = source_node.marker_display_name;
        node.marker_score = source_node.marker_score;
        node.badged = source_node.badged;
        if (source_node.transfer_target.has_value()) {
            node.transfer_target = source_node.transfer_target;
        }
        result.map.nodes.emplace_back(std::move(node));

        if (!source_node.icon_rect.empty()) {
            result.viewport.emplace_back(
                NodeObservation {
                    *stable,
                    source_node.icon_rect,
                    source_node.text_rect,
                    source_node.confidence,
                    source_node.confidence,
                });
        }
        ++result.summary.node_count;
        if (source_node.type == "unclassified") {
            ++result.summary.unclassified_count;
        }
    }

    const auto current = stable_ids.find(source.current_marker_temporary_id);
    if (current == stable_ids.end()) {
        if (error != nullptr) {
            *error = "current marker does not reference an existing imported node";
        }
        return std::nullopt;
    }
    result.current_node = current->second;
    result.summary.current_node = current->second;

    for (const auto& source_edge : source.edges) {
        if (!source_edge.connected) {
            continue;
        }
        const auto first = by_temporary_id.find(source_edge.temporary_first);
        const auto second = by_temporary_id.find(source_edge.temporary_second);
        if (first == by_temporary_id.end() || second == by_temporary_id.end() || first == second) {
            if (error != nullptr) {
                *error = "connected perception edge references an absent or identical endpoint";
            }
            return std::nullopt;
        }
        const EdgeKnowledge knowledge =
            source_edge.forced_by_connectivity_constraint ? EdgeKnowledge::Unknown : EdgeKnowledge::Confirmed;
        result.map.edges.emplace_back(
            ObservedEdge {
                first->second->position,
                second->second->position,
                knowledge,
                EdgeEvidence {
                    source_edge.probability,
                    source_edge.cnn_connected,
                    source_edge.forced_by_connectivity_constraint,
                    source_edge.decision_source,
                },
            });
        if (knowledge == EdgeKnowledge::Confirmed) {
            ++result.summary.confirmed_edge_count;
        }
        if (source_edge.forced_by_connectivity_constraint) {
            ++result.summary.forced_edge_count;
        }
    }
    return result;
}
} // namespace asst::blackflow
