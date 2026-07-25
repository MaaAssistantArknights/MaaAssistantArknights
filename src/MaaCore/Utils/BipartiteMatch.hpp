#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "Utils/MinCostFlow.hpp"

namespace asst::algorithm::bipartite
{
struct BipartiteMatchResult
{
    size_t match_count = 0;
    std::vector<std::pair<size_t, size_t>> matched;
    std::vector<size_t> unmatched_left;

    [[nodiscard]] bool all_matched() const noexcept { return unmatched_left.empty(); }
};

inline flow::FlowGraph
    build_flow_graph(const std::vector<std::vector<size_t>>& adjacency, size_t left_count, size_t right_count)
{
    int src = 0;
    int sink = static_cast<int>(left_count + right_count + 1);
    flow::FlowGraph fg { .src = src, .sink = sink };
    fg.graph.resize(sink + 1);

    for (size_t i = 0; i < left_count; ++i) {
        fg.add_edge(src, static_cast<int>(i + 1), 1, 0);
    }
    for (size_t j = 0; j < right_count; ++j) {
        fg.add_edge(static_cast<int>(left_count + j + 1), sink, 1, 0);
    }
    for (size_t i = 0; i < left_count; ++i) {
        for (size_t j : adjacency[i]) {
            // magic: 第一项优先满足左边前面的。第二项是反四边形不等式，让前面的组尽可能匹配前面的干员
            int64_t cost = (i + 1) * 10000 - (i + 1) * (j + 1);
            fg.add_edge(static_cast<int>(i + 1), static_cast<int>(left_count + j + 1), 1, cost);
        }
    }
    return fg;
}

inline BipartiteMatchResult
    bipartite_max_match(const std::vector<std::vector<size_t>>& adjacency, size_t left_count, size_t right_count)
{
    auto fg = bipartite::build_flow_graph(adjacency, left_count, right_count);
    flow::min_cost_max_flow(fg);

    BipartiteMatchResult result;
    for (size_t i = 0; i < left_count; ++i) {
        bool matched = false;
        for (auto& e : fg.graph[i + 1]) {
            if (e.cap == 0 && e.to > static_cast<int>(left_count)) {
                result.matched.emplace_back(i, e.to - static_cast<int>(left_count) - 1);
                ++result.match_count;
                matched = true;
                break;
            }
        }
        if (!matched) {
            result.unmatched_left.push_back(i);
        }
    }
    return result;
}

template <typename A, typename B>
BipartiteMatchResult bipartite_max_match(
    const std::vector<A>& left,
    const std::vector<B>& right,
    std::function<bool(const A&, const B&)> can_match)
{
    std::vector<std::vector<size_t>> adjacency(left.size());
    for (size_t i = 0; i < left.size(); ++i) {
        for (size_t j = 0; j < right.size(); ++j) {
            if (can_match(left[i], right[j])) {
                adjacency[i].push_back(j);
            }
        }
    }
    return bipartite_max_match(adjacency, left.size(), right.size());
}
} // namespace asst::algorithm::bipartite
