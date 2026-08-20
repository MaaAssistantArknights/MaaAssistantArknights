#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace asst::algorithm::flow
{
// 固定 256-bit 有符号整数，4 x uint64_t 小端压位（base 2^64），2-补码表示。
// 仅实现费用流算法所需的加减/移位/比较，等要复用的时候再移到独立的 Int256.hpp
class Int256
{
    std::array<uint64_t, 4> limbs {};

    constexpr Int256(uint64_t a, uint64_t b, uint64_t c, uint64_t d) :
        limbs { a, b, c, d }
    {
    }

public:
    constexpr Int256() = default;

    constexpr Int256(int64_t value)
    {
        const uint64_t v = static_cast<uint64_t>(value);
        limbs[0] = v;
        limbs[1] = limbs[2] = limbs[3] = (value < 0) ? UINT64_MAX : 0;
    }

    static constexpr Int256 max_value() { return Int256(UINT64_MAX, UINT64_MAX, UINT64_MAX, INT64_MAX); }

    constexpr Int256& operator+=(const Int256& rhs)
    {
        uint64_t carry = 0;
        for (int q = 0; q < 4; ++q) {
            const uint64_t a = limbs[q];
            const uint64_t b = rhs.limbs[q];
            const uint64_t sum = a + b;
            limbs[q] = sum + carry;
            carry = (sum < a) || (carry != 0 && sum == UINT64_MAX);
        }
        return *this;
    }

    constexpr Int256& operator-=(const Int256& rhs) { return *this += -rhs; }

    constexpr Int256 operator-() const
    {
        Int256 result;
        uint64_t carry = 1;
        for (int q = 0; q < 4; ++q) {
            const uint64_t inv = ~limbs[q];
            result.limbs[q] = inv + carry;
            carry = carry && inv == UINT64_MAX;
        }
        return result;
    }

    constexpr Int256 operator+(const Int256& rhs) const
    {
        Int256 result = *this;
        result += rhs;
        return result;
    }

    constexpr Int256 operator-(const Int256& rhs) const
    {
        Int256 result = *this;
        result -= rhs;
        return result;
    }

    constexpr Int256& operator<<=(uint64_t shift)
    {
        if (shift >= 256) {
            limbs = {};
            return *this;
        }
        const uint64_t whole = shift / 64;
        const uint64_t part = shift % 64;
        for (int q = 3; q >= 0; --q) {
            uint64_t value = 0;
            const int src = q - static_cast<int>(whole);
            if (src >= 0) {
                value = limbs[src] << part;
                if (part != 0 && src - 1 >= 0) {
                    value |= limbs[src - 1] >> (64 - part);
                }
            }
            limbs[q] = value;
        }
        return *this;
    }

    constexpr Int256 operator<<(uint64_t shift) const
    {
        Int256 result = *this;
        result <<= shift;
        return result;
    }

    friend constexpr bool operator==(const Int256& lhs, const Int256& rhs) { return lhs.limbs == rhs.limbs; }

    friend constexpr std::strong_ordering operator<=>(const Int256& lhs, const Int256& rhs)
    {
        const bool lhs_negative = (lhs.limbs[3] >> 63) != 0;
        const bool rhs_negative = (rhs.limbs[3] >> 63) != 0;
        if (lhs_negative != rhs_negative) {
            return lhs_negative ? std::strong_ordering::less : std::strong_ordering::greater;
        }
        for (int q = 3; q >= 0; --q) {
            if (lhs.limbs[q] != rhs.limbs[q]) {
                return lhs.limbs[q] < rhs.limbs[q] ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }
};

static constexpr Int256 kInfCost = Int256::max_value();
static constexpr int64_t kInfCap = std::numeric_limits<int64_t>::max() / 2;

struct FlowEdge
{
    int to = 0;
    int rev = 0;
    int64_t cap = 0;
    Int256 cost = 0;
};

struct FlowGraph
{
    std::vector<std::vector<FlowEdge>> graph;
    int src = 0;
    int sink = 0;

    void add_edge(int from, int to, int64_t cap, Int256 cost)
    {
        graph[from].push_back({ to, static_cast<int>(graph[to].size()), cap, cost });
        graph[to].push_back({ from, static_cast<int>(graph[from].size()) - 1, 0, -cost });
    }
};

inline bool dijkstra(const FlowGraph& fg, const std::vector<Int256>& potential, std::vector<Int256>& dist)
{
    std::ranges::fill(dist, kInfCost);
    dist[fg.src] = 0;

    using State = std::pair<Int256, int>;
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
            Int256 nd = d + e.cost + potential[v] - potential[e.to];
            if (dist[e.to] > nd) {
                dist[e.to] = nd;
                pq.emplace(nd, e.to);
            }
        }
    }
    return dist[fg.sink] < kInfCost;
}

inline bool build_level_graph(const FlowGraph& fg, const std::vector<Int256>& potential, std::vector<int>& level)
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
            if (e.cost + potential[v] != potential[e.to]) {
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
    const std::vector<Int256>& potential,
    std::vector<int>& iter,
    int64_t f)
{
    if (v == fg.sink) {
        return f;
    }
    int64_t total = 0;
    for (int& i = iter[v]; i < static_cast<int>(fg.graph[v].size()); ++i) {
        auto& e = fg.graph[v][i];
        if (e.cap <= 0 || e.cost + potential[v] != potential[e.to]) {
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
    std::vector<Int256> potential(n, 0);
    std::vector<Int256> dist(n);
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
                int64_t pushed = dinic_dfs(fg, fg.src, level, potential, iter, kInfCap);
                if (pushed == 0) {
                    break;
                }
            }
        }
    }
}
} // namespace asst::algorithm::flow
