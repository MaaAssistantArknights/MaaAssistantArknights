#include "BlackFlowObservation.h"

#include <algorithm>
#include <unordered_set>

namespace asst::blackflow
{
namespace
{
std::optional<std::pair<int, int>> grid_shape(int floor) noexcept
{
    switch (floor) {
    case 1:
        return std::pair { 3, 5 };
    case 2:
        return std::pair { 4, 5 };
    case 3:
        return std::pair { 5, 7 };
    case 4:
        return std::pair { 5, 8 };
    case 5:
        return std::pair { 5, 10 };
    default:
        return std::nullopt;
    }
}

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
} // namespace

std::optional<NodeType> BlackFlowObservationAdapter::map_node_type(std::string_view type) noexcept
{
    static const std::unordered_map<std::string_view, NodeType> Mapping = {
        { "battle_normal", NodeType::Combat },
        { "battle_elite", NodeType::EmergencyCombat },
        { "battle_savage", NodeType::FaceOff },
        { "incident", NodeType::Encounter },
        { "shop", NodeType::BattleShop },
        { "scrap_shop", NodeType::ScrapShop },
        { "expedition", NodeType::Scout },
        { "employ", NodeType::EmergencyAid },
        { "rest", NodeType::Rest },
        { "door", NodeType::WindingPassage },
        { "portal", NodeType::BoskyPassage },
        { "light", NodeType::FeatherPoint },
        { "sacrifice", NodeType::Sacrifice },
        { "wish", NodeType::Wish },
        { "hide_battle", NodeType::FerociousPresage },
        { "hide_invisible", NodeType::MysteriousPresage },
        { "battle_boss", NodeType::Boss },
        { "final", NodeType::Final },
        { "evacuate", NodeType::Evacuate },
        { "empty", NodeType::Empty },
        { "unclassified", NodeType::Unknown },
    };
    const auto found = Mapping.find(type);
    return found == Mapping.end() ? std::nullopt : std::optional<NodeType>(found->second);
}

std::vector<GridPosition> BlackFlowObservationAdapter::expected_grid_positions(int floor)
{
    std::vector<GridPosition> result;
    const auto shape = grid_shape(floor);
    if (!shape.has_value()) {
        return result;
    }
    result.reserve(static_cast<std::size_t>(shape->first * shape->second));
    for (int row = 0; row < shape->first; ++row) {
        for (int column = 0; column < shape->second; ++column) {
            result.emplace_back(GridPosition { row, column });
        }
    }
    return result;
}

std::optional<NormalizedPerceptionObservation>
    BlackFlowObservationAdapter::normalize(const BlackFlowMapObservation& source, std::string* error) const
{
    if (!source.recognition_ok || !source.map_valid || !source.graph_connected) {
        if (error != nullptr) {
            *error = "perception result failed the recognition, map, or connectivity validity gate";
        }
        return std::nullopt;
    }
    if (source.state_machine_floor < 1 || source.current_marker_temporary_id < 0) {
        if (error != nullptr) {
            *error = "perception result has no valid state-machine floor or current marker";
        }
        return std::nullopt;
    }
    if (source.hud_floor.has_value() && *source.hud_floor != source.state_machine_floor) {
        if (error != nullptr) {
            *error = "HUD floor conflicts with the state-machine floor";
        }
        return std::nullopt;
    }

    NormalizedPerceptionObservation result;
    result.map.floor = source.state_machine_floor;
    result.map.coverage = source.coverage;
    result.map.covered_positions = source.covered_positions;
    if (result.map.coverage == ObservationCoverage::FullMap && result.map.covered_positions.empty()) {
        result.map.covered_positions = expected_grid_positions(source.state_machine_floor);
    }
    result.hud_action_points = source.hud_action_points;
    result.viewport_revision = source.viewport_revision;
    result.summary.observation_id = source.observation_id;
    result.summary.floor = source.state_machine_floor;
    result.summary.screenshot_us = source.screenshot_us;
    result.summary.recognition_us = source.recognition_us;
    result.summary.attempt_count = source.attempt_count;
    result.summary.retry_count = source.retry_count;

    std::unordered_map<int, const PerceptionNodeObservation*> by_temporary_id;
    std::unordered_map<int, NodeId> stable_ids;
    for (const auto& source_node : source.nodes) {
        if (!source_node.exists) {
            continue;
        }
        const auto type = map_node_type(source_node.type);
        const auto stable = make_stable_node_id(source.state_machine_floor, source_node.position);
        if (!type.has_value() || !stable.has_value() || source_node.temporary_id < 0 ||
            !by_temporary_id.emplace(source_node.temporary_id, &source_node).second) {
            if (error != nullptr) {
                *error = "perception result contains an unknown node type or invalid duplicate node";
            }
            return std::nullopt;
        }
        stable_ids.emplace(source_node.temporary_id, *stable);

        ObservedNode node;
        node.position = source_node.position;
        node.type = *type;
        node.event_mask = event_mask_for(*type);
        node.name = source_node.displayed_name;

        node.identity_state = identity_state_for(source_node.type);
        node.identity_revealed = identity_revealed_for(source_node.type);
        node.badged = source_node.badged;
        if (source_node.transfer_target.has_value()) {
            node.teleport_target = source_node.transfer_target;
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
        result.map.edges.emplace_back(
            ObservedEdge {
                first->second->position,
                second->second->position,
                EdgeKnowledge::Confirmed,
                EdgeEvidence {
                    source_edge.probability,
                    source_edge.cnn_connected,
                    source_edge.forced_by_connectivity_constraint,
                    source_edge.decision_source,
                },
            });
        ++result.summary.confirmed_edge_count;
        if (source_edge.forced_by_connectivity_constraint) {
            ++result.summary.forced_edge_count;
        }
    }
    return result;
}
} // namespace asst::blackflow
