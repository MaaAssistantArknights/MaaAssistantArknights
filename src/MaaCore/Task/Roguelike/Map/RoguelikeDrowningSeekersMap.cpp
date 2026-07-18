#include "RoguelikeDrowningSeekersMap.h"

#include <algorithm>
#include <limits>
#include <queue>

#include "Utils/Logger.hpp"

using namespace asst;

void RoguelikeDrowningSeekersMap::reset()
{
    m_cols = 0;
    m_rows = 0;
    m_cells.clear();
    m_adjacency.clear();
    m_player = { -1, -1 };
}

void RoguelikeDrowningSeekersMap::set_dimensions(int cols, int rows)
{
    m_cols = std::max(0, cols);
    m_rows = std::max(0, rows);
    m_cells.assign(static_cast<size_t>(m_cols) * m_rows, Cell {});
    m_adjacency.clear();
}

void RoguelikeDrowningSeekersMap::set_cell(int col, int row, const Cell& cell)
{
    if (!in_bounds(col, row)) {
        Log.error(__FUNCTION__, "| cell out of bounds", col, row);
        return;
    }
    m_cells[index(col, row)] = cell;
}

void RoguelikeDrowningSeekersMap::add_edge(int c1, int r1, int c2, int r2)
{
    if (!in_bounds(c1, r1) || !in_bounds(c2, r2)) {
        Log.error(__FUNCTION__, "| edge endpoint out of bounds");
        return;
    }
    const size_t a = index(c1, r1);
    const size_t b = index(c2, r2);
    auto& va = m_adjacency[a];
    if (std::ranges::find(va, b) == va.end()) {
        va.emplace_back(b);
    }
    auto& vb = m_adjacency[b];
    if (std::ranges::find(vb, a) == vb.end()) {
        vb.emplace_back(a);
    }
}

const RoguelikeDrowningSeekersMap::Cell& RoguelikeDrowningSeekersMap::cell(int col, int row) const
{
    if (!in_bounds(col, row)) {
        return m_none_cell;
    }
    return m_cells[index(col, row)];
}

bool RoguelikeDrowningSeekersMap::has_node(int col, int row) const
{
    return in_bounds(col, row) && m_cells[index(col, row)].kind != CellKind::None;
}

std::vector<std::pair<int, int>> RoguelikeDrowningSeekersMap::neighbors(int col, int row) const
{
    std::vector<std::pair<int, int>> result;
    if (!in_bounds(col, row)) {
        return result;
    }
    auto it = m_adjacency.find(index(col, row));
    if (it == m_adjacency.end()) {
        return result;
    }
    for (const size_t nb : it->second) {
        const int nc = static_cast<int>(nb % m_cols);
        const int nr = static_cast<int>(nb / m_cols);
        if (has_node(nc, nr)) {
            result.emplace_back(nc, nr);
        }
    }
    return result;
}

std::vector<std::pair<int, int>> RoguelikeDrowningSeekersMap::all_nodes() const
{
    std::vector<std::pair<int, int>> result;
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            if (m_cells[index(c, r)].kind != CellKind::None) {
                result.emplace_back(c, r);
            }
        }
    }
    return result;
}

std::vector<std::pair<int, int>>
    RoguelikeDrowningSeekersMap::find_nodes(RoguelikeNodeType type, bool exclude_visited) const
{
    std::vector<std::pair<int, int>> result;
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            const Cell& cl = m_cells[index(c, r)];
            if (cl.kind == CellKind::None || cl.type != type) {
                continue;
            }
            if (exclude_visited && cl.visited) {
                continue;
            }
            result.emplace_back(c, r);
        }
    }
    return result;
}

std::vector<std::pair<int, int>>
    RoguelikeDrowningSeekersMap::shortest_path(int target_col, int target_row, const CostFun& cost_fun) const
{
    if (m_player.first < 0 || !in_bounds(target_col, target_row) || !has_node(target_col, target_row)) {
        return {};
    }
    const size_t start = index(m_player.first, m_player.second);
    const size_t goal = index(target_col, target_row);

    // Dijkstra（进入 target 格也计入代价；起点玩家格代价为 0）
    const size_t n = m_cells.size();
    std::vector<int> dist(n, std::numeric_limits<int>::max());
    std::vector<size_t> prev(n, n);
    using QItem = std::pair<int, size_t>; // (dist, index)
    std::priority_queue<QItem, std::vector<QItem>, std::greater<>> pq;
    dist[start] = 0;
    pq.emplace(0, start);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) {
            continue;
        }
        if (u == goal) {
            break;
        }
        auto it = m_adjacency.find(u);
        if (it == m_adjacency.end()) {
            continue;
        }
        for (const size_t v : it->second) {
            const int vc = static_cast<int>(v % m_cols);
            const int vr = static_cast<int>(v / m_cols);
            if (!has_node(vc, vr)) {
                continue;
            }
            const int step_cost = cost_fun(vc, vr, m_cells[v]);
            if (step_cost < 0) {
                continue; // 负代价视为不可通行
            }
            const int nd = dist[u] + step_cost;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.emplace(nd, v);
            }
        }
    }

    if (dist[goal] == std::numeric_limits<int>::max()) {
        return {};
    }

    std::vector<std::pair<int, int>> path;
    for (size_t cur = goal; cur != n; cur = prev[cur]) {
        path.emplace_back(static_cast<int>(cur % m_cols), static_cast<int>(cur / m_cols));
        if (cur == start) {
            break;
        }
    }
    std::ranges::reverse(path);
    return path;
}

int RoguelikeDrowningSeekersMap::path_cost(int target_col, int target_row, const CostFun& cost_fun) const
{
    const auto path = shortest_path(target_col, target_row, cost_fun);
    if (path.empty()) {
        return std::numeric_limits<int>::max();
    }
    int total = 0;
    // path[0] 为玩家格，不计入进入代价
    for (size_t i = 1; i < path.size(); ++i) {
        total += cost_fun(path[i].first, path[i].second, m_cells[index(path[i].first, path[i].second)]);
    }
    return total;
}
