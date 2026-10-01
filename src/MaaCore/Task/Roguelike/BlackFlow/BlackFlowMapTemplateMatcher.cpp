#include "BlackFlowMapTemplateMatcher.h"

#include <algorithm>
#include <set>

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
const MapTemplate* match_map_template(const MapObservationBatch& observation, std::span<const MapTemplate> templates)
{
    using MapEdges = std::set<std::pair<GridPosition, GridPosition>>;
    MapEdges observed;
    // 推断边与直接识别的边同等有效；未识别到的模板边不影响匹配。
    for (const auto& edge : observation.edges) {
        if (edge.knowledge != EdgeKnowledge::Absent) {
            const auto endpoints = std::minmax(edge.first, edge.second);
            observed.emplace(endpoints.first, endpoints.second);
        }
    }
    const MapTemplate* selected = nullptr;
    for (const auto& definition : templates) {
        if (definition.floor != observation.floor) {
            continue;
        }
        MapEdges expected;
        for (const auto& [first, second] : definition.edges) {
            const auto endpoints = std::minmax(first, second);
            expected.emplace(endpoints.first, endpoints.second);
        }
        if (!std::ranges::includes(expected, observed)) {
            continue;
        }
        // 匹配有歧义时放弃补全，不影响实际地图。
        if (selected != nullptr) {
            return nullptr;
        }
        selected = &definition;
    }
    return selected;
}

void supplement_map_exits(MapObservationBatch& observation, std::span<const GridPosition> exits)
{
    for (const auto& position : exits) {
        auto node = std::ranges::find(observation.nodes, position, &ObservedNode::position);
        if (node == observation.nodes.end()) {
            // 仅补全已识别节点的身份，避免创建没有点击坐标的移动目标。
            continue;
        }
        node->type = NodeType::Final;
        node->name = "险路尽头";
        node->identity_state = NodeIdentityState::Classified;
        node->identity_revealed = true;
    }
}
} // namespace asst::blackflow
