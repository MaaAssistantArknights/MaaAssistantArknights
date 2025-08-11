#include "RoguelikeBoskyPassageMap.h"
#include "Utils/Logger.hpp"

#include <array>

namespace asst
{

RoguelikeBoskyPassageMap::RoguelikeBoskyPassageMap()
{
    reset();
}

std::optional<size_t> RoguelikeBoskyPassageMap::create_and_insert_node(int x, int y, RoguelikeNodeType type)
{
    if (!in_bounds(x, y)) {
        Log.warn(__FUNCTION__, "| Coordinates ({}, {}) out of bounds", x, y);
        return std::nullopt;
    }

    const auto idx = static_cast<size_t>(y * WIDTH + x);
    Node& n = m_nodes[idx]; // 直接获取节点引用，而不是通过 get_valid_node

    if (!n.exists) {
        n.exists = true;
        n.is_open = true;
        n.visited = false;
        n.type = type;
        ++m_existing_count;

        // 临时措施，先关闭祸乱节点
        if (type == RoguelikeNodeType::Disaster) {
            n.is_open = false;
        }
        return idx;
    }
    else {
        Log.warn(__FUNCTION__, "| Node already exists at ({}, {})", x, y);
        return std::nullopt;
    }
}

RoguelikeBoskyPassageMap::Node* RoguelikeBoskyPassageMap::get_valid_node(size_t index)
{
    if (index >= m_nodes.size()) {
        return nullptr;
    }
    Node& n = m_nodes[index];
    return &n;
}

void RoguelikeBoskyPassageMap::set_visited(size_t index)
{
    Node* n = get_valid_node(index);
    if (!n || !n->exists) {
        return;
    }
    n->visited = true;
}

void RoguelikeBoskyPassageMap::set_node_type(size_t index, RoguelikeNodeType type)
{
    Node* n = get_valid_node(index);
    if (!n || !n->exists) {
        return;
    }
    n->type = type;
}

void RoguelikeBoskyPassageMap::reset()
{
    for (auto& n : m_nodes) {
        n = Node {}; // reset
    }

    // 创建中心节点
    if (auto center = create_and_insert_node(CENTER_X, CENTER_Y, RoguelikeNodeType::Init); center.has_value()) {
        Node& c = m_nodes[center.value()];
        c.is_open = false;
    }
    m_existing_count = 0;
}

std::vector<size_t> RoguelikeBoskyPassageMap::get_open_unvisited_nodes() const
{
    std::vector<size_t> res;
    res.reserve(m_existing_count);
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const Node& n = m_nodes[i];
        if (n.exists && n.is_open && !n.visited) {
            res.push_back(i);
        }
    }
    return res;
}

std::vector<size_t> RoguelikeBoskyPassageMap::get_open_nodes() const
{
    std::vector<size_t> res;
    res.reserve(m_existing_count);
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const Node& n = m_nodes[i];
        if (n.exists && n.is_open) {
            res.push_back(i);
        }
    }
    return res;
}

RoguelikeNodeType RoguelikeBoskyPassageMap::get_node_type(size_t index) const
{
    if (index >= m_nodes.size()) {
        return RoguelikeNodeType::Unknown;
    }
    return m_nodes[index].type;
}

bool RoguelikeBoskyPassageMap::get_node_exists(size_t index) const
{
    return index < m_nodes.size() && m_nodes[index].exists;
}

bool RoguelikeBoskyPassageMap::get_node_open(size_t index) const
{
    return index < m_nodes.size() && m_nodes[index].exists && m_nodes[index].is_open;
}

bool RoguelikeBoskyPassageMap::get_node_visited(size_t index) const
{
    return index < m_nodes.size() && m_nodes[index].exists && m_nodes[index].visited;
}

int RoguelikeBoskyPassageMap::get_node_x(size_t index) const
{
    return static_cast<int>(index % WIDTH);
}

int RoguelikeBoskyPassageMap::get_node_y(size_t index) const
{
    return static_cast<int>(index / WIDTH);
}

std::pair<int, int> RoguelikeBoskyPassageMap::get_node_pixel(
    size_t index,
    int origin_x,
    int origin_y,
    int column_offset,
    int row_offset) const
{
    if (index >= m_nodes.size() || !m_nodes[index].exists) {
        Log.warn(__FUNCTION__, "| Invalid node index: {}", index);
        return { -1, -1 };
    }
    const int x = get_node_x(index);
    const int y = get_node_y(index);
    return { origin_x + x * column_offset, origin_y + y * row_offset };
}

std::vector<size_t> RoguelikeBoskyPassageMap::get_adjacent_existing_nodes(size_t index) const
{
    std::vector<size_t> res;
    if (index >= m_nodes.size()) {
        return res;
    }
    const int x = get_node_x(index);
    const int y = get_node_y(index);
    const std::array<int, 4> dx { 1, -1, 0, 0 };
    const std::array<int, 4> dy { 0, 0, 1, -1 };
    for (auto k = 0; k < 4; ++k) {
        int nx = x + dx[k];
        int ny = y + dy[k];
        if (!in_bounds(nx, ny)) {
            continue;
        }
        const auto nidx = static_cast<size_t>(ny * WIDTH + nx);
        if (m_nodes[nidx].exists) {
            res.push_back(nidx);
        }
    }
    return res;
}

std::optional<size_t> RoguelikeBoskyPassageMap::coord_to_index(int x, int y) const
{
    if (!in_bounds(x, y)) {
        return std::nullopt;
    }
    const auto idx = static_cast<size_t>(y * WIDTH + x);
    if (!m_nodes[idx].exists) {
        return std::nullopt;
    }
    return idx;
}

namespace
{
// 像素坐标到网格坐标的转换辅助函数
std::pair<int, int> pixel_to_grid_coords(int px, int py, int origin_x, int origin_y, int column_offset, int row_offset)
{
    const double dx = static_cast<double>(px - origin_x) / column_offset;
    const double dy = static_cast<double>(py - origin_y) / row_offset;
    const int gx = static_cast<int>(dx + (dx >= 0 ? 0.5 : -0.5));
    const int gy = static_cast<int>(dy + (dy >= 0 ? 0.5 : -0.5));
    return { gx, gy };
}
}

std::optional<size_t> RoguelikeBoskyPassageMap::pixel_to_index(
    int px,
    int py,
    int origin_x,
    int origin_y,
    int column_offset,
    int row_offset,
    int node_width,
    int node_height) const
{
    if (node_width <= 0 || node_height <= 0 || column_offset <= 0 || row_offset <= 0) {
        return std::nullopt;
    }

    auto [gx, gy] = pixel_to_grid_coords(px, py, origin_x, origin_y, column_offset, row_offset);
    return coord_to_index(gx, gy);
}

std::optional<size_t> RoguelikeBoskyPassageMap::ensure_node_from_pixel(
    int px,
    int py,
    int origin_x,
    int origin_y,
    int column_offset,
    int row_offset,
    int node_width,
    int node_height,
    bool is_open,
    RoguelikeNodeType type)
{
    if (node_width <= 0 || node_height <= 0 || column_offset <= 0 || row_offset <= 0) {
        Log.warn(
            __FUNCTION__,
            "| Invalid parameters: node_width={}, node_height={}, column_offset={}, row_offset={}",
            node_width,
            node_height,
            column_offset,
            row_offset);
        return std::nullopt;
    }

    auto [gx, gy] = pixel_to_grid_coords(px, py, origin_x, origin_y, column_offset, row_offset);
    Log.info(__FUNCTION__, "| analyzing node ({}, {}) -> ({}, {})", px, py, gx, gy);

    if (!in_bounds(gx, gy)) {
        Log.warn(__FUNCTION__, "| Grid coordinates ({}, {}) out of bounds", gx, gy);
        return std::nullopt;
    }

    if (auto idx = coord_to_index(gx, gy); idx.has_value()) {
        // 节点已存在，只更新状态
        if (Node* n = get_valid_node(idx.value())) {
            n->is_open = is_open;
            n->type = type;
        }
        return idx;
    }
    // 节点不存在，创建新节点
    return create_and_insert_node(gx, gy, type);
}
} // namespace asst
