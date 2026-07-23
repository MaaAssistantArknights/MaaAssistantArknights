#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "Utils/MinCostFlow.hpp"

namespace asst::algorithm
{
struct BipartiteMatchResult
{
    size_t match_count = 0;
    std::vector<std::pair<size_t, size_t>> matched;
    std::vector<size_t> unmatched_left;

    [[nodiscard]] bool all_matched() const noexcept { return unmatched_left.empty(); }
};

inline BipartiteMatchResult
    bipartite_max_match(const std::vector<std::vector<size_t>>& adjacency, size_t left_count, size_t right_count)
{
    using flow::FlowGraph;

    int src = 0;
    int sink = static_cast<int>(left_count + right_count + 1);
    FlowGraph fg { .src = src, .sink = sink };
    fg.graph.resize(sink + 1);

    for (size_t i = 0; i < left_count; ++i) {
        fg.add_edge(src, static_cast<int>(i + 1), 1, 0);
    }
    for (size_t j = 0; j < right_count; ++j) {
        fg.add_edge(static_cast<int>(left_count + j + 1), sink, 1, 0);
    }
    for (size_t i = 0; i < left_count; ++i) {
        for (size_t j : adjacency[i]) {
            int64_t cost = static_cast<int64_t>(i) * static_cast<int64_t>(right_count) + static_cast<int64_t>(j);
            fg.add_edge(static_cast<int>(i + 1), static_cast<int>(left_count + j + 1), 1, cost);
        }
    }

    flow::min_cost_max_flow(fg);

    BipartiteMatchResult result;
    for (size_t i = 0; i < left_count; ++i) {
        bool matched = false;
        for (auto& e : fg.graph[i + 1]) {
            if (e.cap == 0) {
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
} // namespace asst::algorithm
