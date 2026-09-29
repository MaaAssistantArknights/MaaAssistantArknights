#include "BlackFlowMapTemplateConfig.h"

#include <algorithm>
#include <array>
#include <set>

#include "Utils/Logger.hpp"
#include "Vision/Roguelike/BlackFlow/BlackFlowFloor.h"

namespace asst
{
bool BlackFlowMapTemplateConfig::parse(const json::value& value)
{
    const auto entries = value.find<json::array>("templates");
    if (!entries.has_value() || entries->empty()) {
        LogError << __FUNCTION__ << "Map templates must be a nonempty array";
        return false;
    }
    std::vector<blackflow::MapTemplate> templates;
    std::set<std::string> ids;
    std::array<bool, 5> floors {};
    for (const auto& entry : *entries) {
        const auto id = entry.find<std::string>("id");
        const auto topology = id ? blackflow::map_topology_from_string(*id) : std::nullopt;
        const auto floor = entry.find<int>("floor");
        const auto rows = entry.find<int>("rows");
        const auto columns = entry.find<int>("columns");
        const auto start = entry.find<std::vector<int>>("start");
        const auto exits = entry.find<std::vector<std::vector<int>>>("terminals");
        const auto terminal_name = entry.find<std::string>("terminal_type");
        const auto terminal_type = terminal_name ? blackflow::node_type_from_string(*terminal_name) : std::nullopt;
        const auto edges = entry.find<std::vector<std::vector<std::vector<int>>>>("edges");
        if (!topology || !id || id->empty() || !ids.emplace(*id).second || !floor || *floor < 1 || *floor > 5 ||
            !rows || *rows < 1 || !columns || *columns < 1 || !start || !exits || exits->empty() || !edges ||
            edges->empty() ||
            (terminal_type != blackflow::NodeType::Final && terminal_type != blackflow::NodeType::BattleBoss)) {
            LogError << __FUNCTION__ << "Invalid map template fields";
            return false;
        }
        const auto profiles = blackflow::perception::floor_profiles(*floor);
        if (!std::ranges::any_of(profiles, [&](const auto& profile) {
                return profile.rows == *rows && profile.columns == *columns;
            })) {
            LogError << __FUNCTION__ << "Unsupported map template dimensions" << *id;
            return false;
        }
        const auto position = [&](const std::vector<int>& coordinates) -> std::optional<blackflow::GridPosition> {
            if (coordinates.size() != 2 || coordinates[0] < 0 || coordinates[0] >= *rows || coordinates[1] < 0 ||
                coordinates[1] >= *columns) {
                return std::nullopt;
            }
            return blackflow::GridPosition { coordinates[0], coordinates[1] };
        };
        const auto origin = position(*start);
        if (!origin) {
            LogError << __FUNCTION__ << "Invalid map template start" << *id;
            return false;
        }
        blackflow::MapTemplate item { *topology, *floor, *rows, *columns, *origin, *terminal_type, {}, {} };
        std::set<blackflow::GridPosition> terminals;
        std::set<blackflow::GridPosition> cells;
        std::set<std::pair<blackflow::GridPosition, blackflow::GridPosition>> connections;
        for (const auto& coordinates : *exits) {
            const auto terminal = position(coordinates);
            if (!terminal || *terminal == *origin || !terminals.emplace(*terminal).second) {
                LogError << __FUNCTION__ << "Invalid map template exit" << *id;
                return false;
            }
            item.terminals.emplace_back(*terminal);
        }
        for (const auto& coordinates : *edges) {
            if (coordinates.size() != 2) {
                LogError << __FUNCTION__ << "Invalid map template edge" << *id;
                return false;
            }
            const auto first = position(coordinates[0]);
            const auto second = position(coordinates[1]);
            if (!first || !second || *first == *second) {
                LogError << __FUNCTION__ << "Invalid map template edge endpoints" << *id;
                return false;
            }
            const auto endpoints = std::minmax(*first, *second);
            if (!connections.emplace(endpoints.first, endpoints.second).second) {
                LogError << __FUNCTION__ << "Duplicate map template edge" << *id;
                return false;
            }
            cells.insert(*first);
            cells.insert(*second);
            item.edges.emplace_back(*first, *second);
        }
        if (!cells.contains(*origin) ||
            !std::ranges::all_of(terminals, [&](const auto& cell) { return cells.contains(cell); })) {
            LogError << __FUNCTION__ << "Map template start or exit is absent from template edges" << *id;
            return false;
        }
        // 模板坐标与裁去外侧空行列后的识别坐标对齐，内部允许空位。
        const auto [top, bottom] = std::ranges::minmax(cells, {}, &blackflow::GridPosition::row);
        const auto [left, right] = std::ranges::minmax(cells, {}, &blackflow::GridPosition::column);
        if (top.row != 0 || bottom.row != *rows - 1 || left.column != 0 || right.column != *columns - 1) {
            LogError << __FUNCTION__ << "Map template bounds do not match its dimensions" << *id;
            return false;
        }
        // 这里只校验模板自身的全部点位连通性，不改变实际地图的识别结果。
        std::set<blackflow::GridPosition> visited { *origin };
        std::vector<blackflow::GridPosition> pending { *origin };
        for (std::size_t index = 0; index < pending.size(); ++index) {
            const auto current = pending[index];
            for (const auto& [first, second] : item.edges) {
                if (first != current && second != current) {
                    continue;
                }
                const auto next = first == current ? second : first;
                if (visited.emplace(next).second) {
                    pending.emplace_back(next);
                }
            }
        }
        if (visited.size() != cells.size()) {
            LogError << __FUNCTION__ << "Map template graph is disconnected" << *id;
            return false;
        }
        floors[static_cast<std::size_t>(*floor - 1)] = true;
        templates.emplace_back(std::move(item));
    }
    if (templates.size() != blackflow::MapTopologyNames.size() ||
        !std::ranges::all_of(floors, [](bool present) { return present; })) {
        LogError << __FUNCTION__ << "Map templates must cover floors one through five";
        return false;
    }
    m_templates = std::move(templates);
    return true;
}
} // namespace asst
