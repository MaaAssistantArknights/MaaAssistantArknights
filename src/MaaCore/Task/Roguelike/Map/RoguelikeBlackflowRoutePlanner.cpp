#include "RoguelikeBlackflowRoutePlanner.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace asst::drowning_seekers
{
namespace
{
constexpr size_t kMaxGears = 12;  // 剩余次数打包进 uint64（每个 4 bit）
constexpr int kMaxTracked = 64;   // visited 位掩码可跟踪的格数上限
constexpr size_t kMaxSearchStates = 100000; // 防止高行动力 + 多加工品导致搜索爆炸

struct SearchNode
{
    int pos = -1;
    int ap = 0;
    std::uint64_t uses = 0; // 各加工品剩余次数（4 bit × 加工品）
    std::uint64_t mask = 0; // 本次规划中已"消费"过奖励/效果的格
    double score = 0.0;
    int combat_count = 0;   // 路线上进入战斗节点的次数（避战策略使用）
    int carry_spent = 0;    // 已消耗的可携带加工品次数（平局裁决用）
    int parent = -1;        // arena 下标
    PlannerAction act;
    int depth = 0;
    bool terminal = false;  // 已进入 endpoint 格，不再扩展
};

struct StateKey
{
    std::uint64_t a = 0;
    std::uint64_t b = 0;

    bool operator==(const StateKey&) const = default;
};

struct StateKeyHash
{
    size_t operator()(const StateKey& k) const noexcept
    {
        std::uint64_t h = k.a * 0x9E3779B97F4A7C15ull;
        h ^= k.b + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
        return static_cast<size_t>(h);
    }
};

StateKey make_key(const SearchNode& node)
{
    const std::uint64_t pos = static_cast<std::uint64_t>(node.pos) & 0xFF;
    const std::uint64_t ap = static_cast<std::uint64_t>(std::min(node.ap, 255)) & 0xFF;
    return { (node.uses & 0xFFFFFFFFFFFFull) | (pos << 48) | (ap << 56), node.mask };
}

int packed_uses(std::uint64_t packed, size_t i)
{
    return static_cast<int>((packed >> (i * 4)) & 0xF);
}

// a 是否严格优于 b（确定性平局裁决：分高 → 可携带耗得少 → 剩余 AP 多 → 格序号小 → 深度小）
bool better(const SearchNode& a, const SearchNode& b, bool avoid_combat_first = false)
{
    if (avoid_combat_first) {
        if (a.combat_count != b.combat_count) {
            return a.combat_count < b.combat_count;
        }
        if (a.depth != b.depth) {
            return a.depth < b.depth;
        }
    }
    if (a.score != b.score) {
        return a.score > b.score;
    }
    if (a.carry_spent != b.carry_spent) {
        return a.carry_spent < b.carry_spent;
    }
    if (a.ap != b.ap) {
        return a.ap > b.ap;
    }
    if (a.pos != b.pos) {
        return a.pos < b.pos;
    }
    return a.depth < b.depth;
}
} // namespace

std::vector<int> range_targets(const PlannerMap& map, int from, const PlannerGear& gear)
{
    std::vector<int> result;
    const int n = map.cols * map.rows;
    if (from < 0 || from >= n || map.cols <= 0) {
        return result;
    }
    const int fc = from % map.cols;
    const int fr = from / map.cols;

    auto add_if_exists = [&](int c, int r) {
        if (c < 0 || c >= map.cols || r < 0 || r >= map.rows) {
            return;
        }
        const int idx = r * map.cols + c;
        if (idx != from && map.cells[idx].exists) {
            result.push_back(idx);
        }
    };

    switch (gear.range) {
    case GearRange::Line:
        for (int k = 1; k <= gear.distance; ++k) {
            add_if_exists(fc + k, fr);
            add_if_exists(fc - k, fr);
            add_if_exists(fc, fr + k);
            add_if_exists(fc, fr - k);
        }
        break;
    case GearRange::Ring8:
    case GearRange::Ring12:
        for (int dc = -1; dc <= 1; ++dc) {
            for (int dr = -1; dr <= 1; ++dr) {
                if (dc != 0 || dr != 0) {
                    add_if_exists(fc + dc, fr + dr);
                }
            }
        }
        if (gear.range == GearRange::Ring12) {
            add_if_exists(fc + 2, fr);
            add_if_exists(fc - 2, fr);
            add_if_exists(fc, fr + 2);
            add_if_exists(fc, fr - 2);
        }
        break;
    case GearRange::Any:
    case GearRange::AnyNonCombat:
    case GearRange::AnyTrader:
        for (int idx = 0; idx < n; ++idx) {
            if (idx == from || !map.cells[idx].exists) {
                continue;
            }
            if (gear.range == GearRange::AnyNonCombat && map.cells[idx].is_combat) {
                continue;
            }
            if (gear.range == GearRange::AnyTrader && !map.cells[idx].is_trader) {
                continue;
            }
            result.push_back(idx);
        }
        break;
    case GearRange::RandomNonCombat:
        break; // 落点不可控，不参与规划
    }

    std::ranges::sort(result);
    result.erase(std::ranges::unique(result).begin(), result.end());
    return result;
}

PlannerResult plan(const PlannerMap& map, const std::vector<PlannerGear>& gears, const PlannerParams& params)
{
    PlannerResult result;
    const int n = map.cols * map.rows;
    if (n <= 0 || static_cast<int>(map.cells.size()) != n || map.player < 0 || map.player >= n ||
        static_cast<int>(map.adj.size()) != n || params.action_points < 0) {
        return result;
    }

    // —— 跟踪格分配位号：一格一 bit，进入过（消费过奖励/效果）就置位 ——
    // n <= 64 时全部可精确跟踪；更大的图只跟踪最重要的 64 格，其余保守视为已消费（不重复计奖励）。
    std::vector<int> bit_of(n, -1);
    if (n <= kMaxTracked) {
        for (int i = 0; i < n; ++i) {
            bit_of[i] = i;
        }
    }
    else {
        std::vector<int> tracked;
        for (int i = 0; i < n; ++i) {
            const PlannerCell& cell = map.cells[i];
            if (cell.exists && !cell.visited && (cell.weight != 0.0 || cell.ap_gain > 0)) {
                tracked.push_back(i);
            }
        }
        std::ranges::sort(tracked, [&](int a, int b) {
            const double wa = std::abs(map.cells[a].weight);
            const double wb = std::abs(map.cells[b].weight);
            if (wa != wb) {
                return wa > wb;
            }
            if (map.cells[a].ap_gain != map.cells[b].ap_gain) {
                return map.cells[a].ap_gain > map.cells[b].ap_gain;
            }
            return a < b;
        });
        if (tracked.size() > static_cast<size_t>(kMaxTracked)) {
            tracked.resize(kMaxTracked);
        }
        for (size_t bit = 0; bit < tracked.size(); ++bit) {
            bit_of[tracked[bit]] = static_cast<int>(bit);
        }
    }

    const size_t gear_cnt = std::min(gears.size(), kMaxGears);
    std::uint64_t init_uses = 0;
    for (size_t i = 0; i < gear_cnt; ++i) {
        if (gears[i].controllable && gears[i].uses > 0) {
            init_uses |= static_cast<std::uint64_t>(std::min(gears[i].uses, 15)) << (i * 4);
        }
    }

    // 预生成每个加工品从每格出发的落点集合
    std::vector<std::vector<std::vector<int>>> targets_cache(gear_cnt);
    for (size_t i = 0; i < gear_cnt; ++i) {
        targets_cache[i].resize(n);
        if (!gears[i].controllable) {
            continue;
        }
        for (int p = 0; p < n; ++p) {
            targets_cache[i][p] = range_targets(map, p, gears[i]);
        }
    }

    const bool shortest_endpoint = params.endpoint_required && params.shortest_endpoint;
    const bool avoid_combat_first = shortest_endpoint && params.avoid_combat_first;
    const int beam_width = std::max(8, params.beam_width);
    const int max_depth = std::max(1, params.max_depth);

    std::vector<SearchNode> arena;
    arena.reserve(1024);
    SearchNode root;
    root.pos = map.player;
    root.ap = params.action_points;
    root.uses = init_uses;
    arena.push_back(root);

    std::vector<int> frontier { 0 };

    // 候选在束裁剪前记录（否则唯一可达终点的路线可能被高分刷分路线挤出束外）。
    // 候选节点自身无需进入 arena：其 parent 链上的节点必然已在 arena 中。
    std::optional<SearchNode> best_terminal_node;
    std::optional<SearchNode> best_any_node;
    auto value_of = [&](const SearchNode& node) { return node.score + params.leftover_ap_weight * node.ap; };
    auto candidate_better = [&](const SearchNode& a, const SearchNode& b) {
        const double va = value_of(a);
        const double vb = value_of(b);
        if (va != vb) {
            return va > vb;
        }
        return better(a, b);
    };

    auto combat_candidate_better = [&](const SearchNode& a, const SearchNode& b) {
        return better(a, b, true);
    };

    std::optional<SearchNode> best_combat_terminal;

    auto make_result = [&](const SearchNode* chosen) {
        PlannerResult out;
        if (chosen == nullptr) {
            return out;
        }
        out.has_route = true;
        out.reaches_endpoint = chosen->terminal;
        out.score = value_of(*chosen);
        out.actions.push_back(chosen->act);
        for (int idx = chosen->parent; idx > 0; idx = arena[idx].parent) {
            out.actions.push_back(arena[idx].act);
        }
        std::ranges::reverse(out.actions);
        return out;
    };

    // 进入 target 格；不可行返回 nullopt
    auto try_enter = [&](int cur_idx, int target, int gear_idx) -> std::optional<SearchNode> {
        const SearchNode& cur = arena[cur_idx];
        const PlannerCell& cell = map.cells[target];
        if (!cell.exists) {
            return std::nullopt;
        }
        // 非终点模式下终点格视为禁区（进入会直接离层）
        if (cell.is_endpoint && !params.endpoint_required) {
            return std::nullopt;
        }
        int cost = 1;
        int gear_ap_gain = 0;
        double gear_delta = 0.0;
        if (gear_idx >= 0) {
            const PlannerGear& gear = gears[gear_idx];
            if (packed_uses(cur.uses, gear_idx) <= 0) {
                return std::nullopt;
            }
            cost = gear.ap_cost;
            gear_ap_gain = gear.ap_gain;
            gear_delta = gear.use_reward - gear.use_cost;
        }
        if (cur.ap < cost) {
            return std::nullopt;
        }

        SearchNode nxt = cur;
        nxt.parent = cur_idx;
        nxt.depth = cur.depth + 1;
        nxt.act = { gear_idx < 0, gear_idx, target };
        nxt.ap = cur.ap - cost + gear_ap_gain;
        nxt.score = cur.score + gear_delta;
        nxt.combat_count = cur.combat_count + (cell.is_combat ? 1 : 0);
        if (gear_idx >= 0) {
            nxt.uses = cur.uses - (1ull << (gear_idx * 4));
            if (gears[gear_idx].carryover) {
                ++nxt.carry_spent;
            }
        }

        const int bit = bit_of[target];
        bool consumed = cell.visited;
        if (bit >= 0 && ((cur.mask >> bit) & 1)) {
            consumed = true;
        }
        if (bit < 0 && !cell.visited) {
            consumed = true; // 未跟踪的格保守处理，避免重复计奖励
        }
        if (!consumed) {
            nxt.score += cell.weight;
            nxt.ap += cell.ap_gain;
            if (bit >= 0) {
                nxt.mask |= 1ull << bit;
            }
        }

        // 曲折密道：进入后免费移动至孪生格（不触发孪生格的事件）
        if (cell.teleport_twin >= 0 && cell.teleport_twin < n && map.cells[cell.teleport_twin].exists) {
            nxt.pos = cell.teleport_twin;
        }
        else {
            nxt.pos = target;
        }
        nxt.terminal = cell.is_endpoint;
        return nxt;
    };

    for (int depth = 1; depth <= max_depth && !frontier.empty(); ++depth) {
        std::vector<SearchNode> generated;
        for (const int cur_idx : frontier) {
            const int cur_pos = arena[cur_idx].pos;
            for (const int nb : map.adj[cur_pos]) {
                if (auto nxt = try_enter(cur_idx, nb, -1)) {
                    generated.push_back(std::move(*nxt));
                }
            }
            for (size_t gi = 0; gi < gear_cnt; ++gi) {
                if (!gears[gi].controllable || packed_uses(arena[cur_idx].uses, gi) <= 0) {
                    continue;
                }
                for (const int target : targets_cache[gi][cur_pos]) {
                    if (auto nxt = try_enter(cur_idx, target, static_cast<int>(gi))) {
                        generated.push_back(std::move(*nxt));
                    }
                }
            }
        }
        if (generated.empty()) {
            break;
        }

        // 烧水模式只关心到终点的最短动作序列。普通最短路在当前层发现终点
        // 就可以返回；避战模式则先比较整条路线的战斗次数，再比较动作数，
        // 所以要继续搜索，直到找到零战斗终点或达到搜索深度。
        if (avoid_combat_first) {
            for (const auto& node : generated) {
                if (!node.terminal || !best_combat_terminal || combat_candidate_better(node, *best_combat_terminal)) {
                    if (node.terminal) {
                        best_combat_terminal = node;
                    }
                }
            }
            if (best_combat_terminal && best_combat_terminal->combat_count == 0) {
                return make_result(&*best_combat_terminal);
            }
        }
        else if (shortest_endpoint) {
            const SearchNode* shortest_terminal = nullptr;
            for (const auto& node : generated) {
                if (!node.terminal || shortest_terminal == nullptr || better(node, *shortest_terminal)) {
                    if (node.terminal) {
                        shortest_terminal = &node;
                    }
                }
            }
            if (shortest_terminal != nullptr) {
                return make_result(shortest_terminal);
            }
        }

        // 同状态去重，保留更优者
        std::unordered_map<StateKey, size_t, StateKeyHash> dedup;
        dedup.reserve(generated.size());
        std::vector<size_t> kept;
        kept.reserve(generated.size());
        for (size_t i = 0; i < generated.size(); ++i) {
            const StateKey key = make_key(generated[i]);
            auto [it, inserted] = dedup.try_emplace(key, i);
            if (inserted) {
                kept.push_back(i);
            }
            else if (better(generated[i], generated[it->second], avoid_combat_first)) {
                // 用更优者替换占位（kept 中仍是旧下标，替换内容即可）
                generated[it->second] = generated[i];
            }
        }

        // 记录候选（束裁剪前，保证不漏掉可行终点路线）
        for (const size_t gi : kept) {
            const SearchNode& node = generated[gi];
            if (!best_any_node || candidate_better(node, *best_any_node)) {
                best_any_node = node;
            }
            if (node.terminal && (!best_terminal_node || candidate_better(node, *best_terminal_node))) {
                best_terminal_node = node;
            }
        }

        std::ranges::stable_sort(
            kept,
            [&](size_t a, size_t b) { return better(generated[a], generated[b], avoid_combat_first); });
        // 避战终点模式也必须裁剪束宽：若没有零战斗终点，不能让所有加工品组合无限扩张。
        // 每层仍按战斗次数优先排序，因此不会改变“避战第一”的决策顺序，只限制候选规模。
        const bool use_beam_pruning = !shortest_endpoint || avoid_combat_first;
        if (use_beam_pruning && kept.size() > static_cast<size_t>(beam_width)) {
            kept.resize(beam_width);
        }

        std::vector<int> next_frontier;
        next_frontier.reserve(kept.size());
        bool state_limit_reached = false;
        for (const size_t gi : kept) {
            if (generated[gi].terminal) {
                continue; // 终点节点不再扩展，也无需成为 parent
            }
            if (arena.size() >= kMaxSearchStates) {
                state_limit_reached = true;
                break;
            }
            arena.push_back(generated[gi]);
            next_frontier.push_back(static_cast<int>(arena.size()) - 1);
        }
        if (state_limit_reached) {
            frontier.clear();
        }
        else {
            frontier = std::move(next_frontier);
        }
    }

    const SearchNode* chosen = nullptr;
    if (avoid_combat_first) {
        if (best_combat_terminal) {
            return make_result(&*best_combat_terminal);
        }
        if (params.best_effort && best_any_node) {
            chosen = &*best_any_node;
        }
    }
    else if (params.endpoint_required) {
        if (best_terminal_node) {
            chosen = &*best_terminal_node;
        }
        else if (params.best_effort && best_any_node) {
            chosen = &*best_any_node;
        }
    }
    else if (best_any_node) {
        chosen = &*best_any_node;
    }
    if (chosen == nullptr) {
        return result;
    }

    return make_result(chosen);
}
} // namespace asst::drowning_seekers
