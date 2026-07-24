#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

namespace asst::algorithm::flow
{
static constexpr int64_t kInfCost = std::numeric_limits<int64_t>::max() / 2;

struct FlowEdge
{
    int to = 0;
    int rev = 0;
    int64_t cap = 0;
    int64_t cost = 0;
};

struct FlowGraph
{
    std::vector<std::vector<FlowEdge>> graph;
    int src = 0;
    int sink = 0;

    void add_edge(int from, int to, int64_t cap, int64_t cost)
    {
        graph[from].push_back({ to, static_cast<int>(graph[to].size()), cap, cost });
        graph[to].push_back({ from, static_cast<int>(graph[from].size()) - 1, 0, -cost });
    }
};

inline bool dijkstra(const FlowGraph& fg, const std::vector<int64_t>& potential, std::vector<int64_t>& dist)
{
    std::ranges::fill(dist, kInfCost);
    dist[fg.src] = 0;

    using State = std::pair<int64_t, int>;
    std::priority_queue<State, std::vector<State>, std::greater<>> pq;
    pq.emplace(0, fg.src);

    while (!pq.empty()) {
        auto [d, v] = pq.top();
        pq.pop();
        if (dist[v] < d) {
            continue;
        }
        for (auto& e : fg.graph[v]) {
            if (e.cap <= 0) {
                continue;
            }
            int64_t nd = d + e.cost + potential[v] - potential[e.to];
            if (dist[e.to] > nd) {
                dist[e.to] = nd;
                pq.emplace(nd, e.to);
            }
        }
    }
    return dist[fg.sink] < kInfCost;
}

inline bool build_level_graph(const FlowGraph& fg, const std::vector<int64_t>& potential, std::vector<int>& level)
{
    std::ranges::fill(level, -1);
    level[fg.src] = 0;
    std::queue<int> q;
    q.push(fg.src);

    while (!q.empty()) {
        int v = q.front();
        q.pop();
        for (auto& e : fg.graph[v]) {
            if (e.cap <= 0) {
                continue;
            }
            if (e.cost + potential[v] - potential[e.to] != 0) {
                continue;
            }
            if (level[e.to] >= 0) {
                continue;
            }
            level[e.to] = level[v] + 1;
            q.push(e.to);
        }
    }
    return level[fg.sink] >= 0;
}

inline int64_t dinic_dfs(
    FlowGraph& fg,
    int v,
    const std::vector<int>& level,
    const std::vector<int64_t>& potential,
    std::vector<int>& iter,
    int64_t f)
{
    if (v == fg.sink) {
        return f;
    }
    int64_t total = 0;
    for (int& i = iter[v]; i < static_cast<int>(fg.graph[v].size()); ++i) {
        auto& e = fg.graph[v][i];
        if (e.cap <= 0 || e.cost + potential[v] - potential[e.to] != 0) {
            continue;
        }
        if (level[e.to] != level[v] + 1) {
            continue;
        }
        int64_t pushed = dinic_dfs(fg, e.to, level, potential, iter, std::min(f, e.cap));
        if (pushed == 0) {
            continue;
        }
        e.cap -= pushed;
        fg.graph[e.to][e.rev].cap += pushed;
        total += pushed;
        f -= pushed;
        if (f == 0) {
            break;
        }
    }
    return total;
}

inline void min_cost_max_flow(FlowGraph& fg)
{
    int n = static_cast<int>(fg.graph.size());
    std::vector<int64_t> potential(n, 0);
    std::vector<int64_t> dist(n);
    std::vector<int> level(n), iter(n);

    while (true) {
        if (!dijkstra(fg, potential, dist)) {
            break;
        }
        for (int v = 0; v < n; ++v) {
            if (dist[v] < kInfCost) {
                potential[v] += dist[v];
            }
        }
        while (build_level_graph(fg, potential, level)) {
            std::ranges::fill(iter, 0);
            while (true) {
                int64_t pushed = dinic_dfs(fg, fg.src, level, potential, iter, kInfCost);
                if (pushed == 0) {
                    break;
                }
            }
        }
    }
}
} // namespace asst::algorithm::flow
