// 黑流树海迷宫路线规划器（RoguelikeBlackflowRoutePlanner）单元测试。
// 规划器零 MaaCore 依赖，可直接编译测试。

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>

#include "Task/Roguelike/Map/RoguelikeBlackflowRoutePlanner.h"

using namespace asst::drowning_seekers;

namespace
{
// 简易构图工具：cell / edge / player 逐步搭建 PlannerMap
struct MapBuilder
{
    PlannerMap map;

    MapBuilder(int cols, int rows)
    {
        map.cols = cols;
        map.rows = rows;
        map.cells.resize(static_cast<size_t>(cols) * rows);
        map.adj.resize(static_cast<size_t>(cols) * rows);
    }

    [[nodiscard]] int idx(int col, int row) const { return row * map.cols + col; }

    MapBuilder& cell(int col, int row, const PlannerCell& c = {})
    {
        auto& target = map.cells[idx(col, row)];
        target = c;
        target.exists = true;
        return *this;
    }

    MapBuilder& edge(int c1, int r1, int c2, int r2)
    {
        const int a = idx(c1, r1);
        const int b = idx(c2, r2);
        map.adj[a].push_back(b);
        map.adj[b].push_back(a);
        return *this;
    }

    MapBuilder& player(int col, int row)
    {
        map.player = idx(col, row);
        return *this;
    }
};

PlannerCell weighted(double weight)
{
    PlannerCell c;
    c.weight = weight;
    return c;
}

PlannerCell endpoint()
{
    PlannerCell c;
    c.is_endpoint = true;
    return c;
}

PlannerParams make_params(int ap, bool endpoint_required)
{
    PlannerParams p;
    p.action_points = ap;
    p.endpoint_required = endpoint_required;
    p.best_effort = true;
    p.leftover_ap_weight = 0.0;
    return p;
}
} // namespace

TEST_CASE("planner.walk_to_endpoint_with_detour", "[dsplanner]")
{
    // row0: P -- . -- E    row1 的 (1,1) 挂着高权重节点
    //       |    |
    //            (1,1) w=5
    MapBuilder b(3, 2);
    b.cell(0, 0).cell(1, 0).cell(2, 0, endpoint()).cell(1, 1, weighted(5.0));
    b.edge(0, 0, 1, 0).edge(1, 0, 2, 0).edge(1, 0, 1, 1);
    b.player(0, 0);

    SECTION("行动力充足时绕路吃高权重节点")
    {
        const auto result = plan(b.map, {}, make_params(4, true));
        REQUIRE(result.has_route);
        REQUIRE(result.reaches_endpoint);
        REQUIRE(result.score == 5.0);
        REQUIRE(result.actions.size() == 4); // (1,0) (1,1) (1,0) (2,0)
        REQUIRE(result.actions.back().target == b.idx(2, 0));
    }

    SECTION("行动力刚够时直奔终点")
    {
        const auto result = plan(b.map, {}, make_params(2, true));
        REQUIRE(result.has_route);
        REQUIRE(result.reaches_endpoint);
        REQUIRE(result.score == 0.0);
        REQUIRE(result.actions.size() == 2);
    }

    SECTION("烧水模式按最短路线直奔终点，不为收益绕路")
    {
        auto params = make_params(4, true);
        params.shortest_endpoint = true;
        const auto result = plan(b.map, {}, params);
        REQUIRE(result.has_route);
        REQUIRE(result.reaches_endpoint);
        REQUIRE(result.actions.size() == 2);
        REQUIRE(result.actions.back().target == b.idx(2, 0));
    }

    SECTION("烧水模式先避战，再在无战斗路线中取最短")
    {
        b.map.cells[b.idx(1, 0)].is_combat = true;
        auto params = make_params(4, true);
        params.shortest_endpoint = true;
        params.avoid_combat_first = true;

        // 直线两步会经过战斗节点；绕开战斗节点需要三步。
        b.map.adj[b.idx(0, 0)].clear();
        b.map.adj[b.idx(0, 0)].push_back(b.idx(1, 1));
        b.map.adj[b.idx(1, 1)].push_back(b.idx(0, 0));
        b.map.adj[b.idx(1, 1)].push_back(b.idx(1, 0));
        b.map.adj[b.idx(1, 0)].push_back(b.idx(1, 1));

        const auto result = plan(b.map, {}, params);
        REQUIRE(result.has_route);
        REQUIRE(result.reaches_endpoint);
        REQUIRE(result.actions.size() == 3);
        REQUIRE(result.actions.front().target == b.idx(1, 1));
    }
}

TEST_CASE("planner.jetpack_reaches_disconnected_endpoint", "[dsplanner]")
{
    // 终点与玩家完全不连通，只能靠一次性喷气背包（免耗、any）
    MapBuilder b(5, 1);
    b.cell(0, 0).cell(4, 0, endpoint());
    b.player(0, 0);

    PlannerGear jetpack;
    jetpack.range = GearRange::Any;
    jetpack.uses = 1;
    jetpack.ap_cost = 0;
    jetpack.carryover = false;

    auto params = make_params(0, true);
    params.best_effort = false;

    const auto result = plan(b.map, { jetpack }, params);
    REQUIRE(result.has_route);
    REQUIRE(result.reaches_endpoint);
    REQUIRE(result.actions.size() == 1);
    REQUIRE_FALSE(result.actions.front().is_walk);
    REQUIRE(result.actions.front().target == b.idx(4, 0));

    // 没有背包则不可达
    const auto without = plan(b.map, {}, params);
    REQUIRE_FALSE(without.has_route);
}

TEST_CASE("planner.winding_passage_teleport", "[dsplanner]")
{
    // P -- A(密道) ... B(密道) -- E；A/B 互为孪生，传送免费
    MapBuilder b(7, 1);
    PlannerCell passage_a;
    passage_a.teleport_twin = 5;
    PlannerCell passage_b;
    passage_b.teleport_twin = 1;
    b.cell(0, 0).cell(1, 0, passage_a).cell(5, 0, passage_b).cell(6, 0, endpoint());
    b.edge(0, 0, 1, 0).edge(5, 0, 6, 0);
    b.player(0, 0);

    auto params = make_params(3, true);
    params.best_effort = false;
    params.leftover_ap_weight = 1.0;

    const auto result = plan(b.map, {}, params);
    REQUIRE(result.has_route);
    REQUIRE(result.reaches_endpoint);
    // 走进密道(1) + 走到终点(1) = 2 步，剩余 1 点行动力
    REQUIRE(result.actions.size() == 2);
    REQUIRE(result.actions[0].target == b.idx(1, 0));
    REQUIRE(result.actions[1].target == b.idx(6, 0));
    REQUIRE(result.score == 1.0);
}

TEST_CASE("planner.vantage_point_extends_reach", "[dsplanner]")
{
    // P -- V(+1AP) -- . -- E，行动力只有 2，靠羽瞰点补给才够 3 步
    MapBuilder b(4, 1);
    PlannerCell vantage;
    vantage.ap_gain = 1;
    b.cell(0, 0).cell(1, 0, vantage).cell(2, 0).cell(3, 0, endpoint());
    b.edge(0, 0, 1, 0).edge(1, 0, 2, 0).edge(2, 0, 3, 0);
    b.player(0, 0);

    auto params = make_params(2, true);
    params.best_effort = false;

    const auto with_vantage = plan(b.map, {}, params);
    REQUIRE(with_vantage.has_route);
    REQUIRE(with_vantage.reaches_endpoint);
    REQUIRE(with_vantage.actions.size() == 3);

    // 已访问的羽瞰点不再补给 → 不可达
    b.map.cells[b.idx(1, 0)].visited = true;
    const auto visited_vantage = plan(b.map, {}, params);
    REQUIRE_FALSE(visited_vantage.has_route);
}

TEST_CASE("planner.remote_controller_free_ap", "[dsplanner]")
{
    // 行动力为 0：简易遥控器（免耗 ring8 + 行动力+3）先跳到 (1,1)，再徒步进终点
    MapBuilder b(3, 2);
    b.cell(0, 0).cell(1, 1, weighted(5.0)).cell(2, 1, endpoint());
    b.edge(1, 1, 2, 1);
    b.player(0, 0);

    PlannerGear remote;
    remote.range = GearRange::Ring8;
    remote.uses = 1;
    remote.ap_cost = 0;
    remote.ap_gain = 3;

    auto params = make_params(0, true);
    params.best_effort = false;

    const auto result = plan(b.map, { remote }, params);
    REQUIRE(result.has_route);
    REQUIRE(result.reaches_endpoint);
    REQUIRE(result.actions.size() == 2);
    REQUIRE_FALSE(result.actions[0].is_walk);
    REQUIRE(result.actions[0].target == b.idx(1, 1));
    REQUIRE(result.actions[1].is_walk);
    REQUIRE(result.score == 5.0);
}

TEST_CASE("planner.range_geometry", "[dsplanner]")
{
    MapBuilder b(5, 5);
    for (int c = 0; c < 5; ++c) {
        for (int r = 0; r < 5; ++r) {
            b.cell(c, r);
        }
    }
    const int center = b.idx(2, 2);

    auto as_set = [](const std::vector<int>& v) { return std::set<int>(v.begin(), v.end()); };

    SECTION("line 2：十字 8 格")
    {
        PlannerGear gear;
        gear.range = GearRange::Line;
        gear.distance = 2;
        const auto targets = as_set(range_targets(b.map, center, gear));
        const std::set<int> expected {
            b.idx(0, 2), b.idx(1, 2), b.idx(3, 2), b.idx(4, 2),
            b.idx(2, 0), b.idx(2, 1), b.idx(2, 3), b.idx(2, 4),
        };
        REQUIRE(targets == expected);
    }

    SECTION("ring8：周围 8 格")
    {
        PlannerGear gear;
        gear.range = GearRange::Ring8;
        REQUIRE(range_targets(b.map, center, gear).size() == 8);
    }

    SECTION("ring12：ring8 + 直线距离 2 的 4 格（剑雨范围）")
    {
        PlannerGear gear;
        gear.range = GearRange::Ring12;
        const auto targets = as_set(range_targets(b.map, center, gear));
        REQUIRE(targets.size() == 12);
        REQUIRE(targets.contains(b.idx(2, 0)));
        REQUIRE(targets.contains(b.idx(0, 2)));
        REQUIRE_FALSE(targets.contains(b.idx(0, 0))); // 对角距离 2 不在范围内
    }

    SECTION("空格不可作为落点")
    {
        MapBuilder sparse(3, 1);
        sparse.cell(0, 0); // (1,0)、(2,0) 不存在
        PlannerGear gear;
        gear.range = GearRange::Line;
        gear.distance = 2;
        REQUIRE(range_targets(sparse.map, sparse.idx(0, 0), gear).empty());
    }

    SECTION("anyTrader 只落行商节点")
    {
        MapBuilder t(3, 1);
        PlannerCell trader;
        trader.is_trader = true;
        t.cell(0, 0).cell(1, 0).cell(2, 0, trader);
        PlannerGear gear;
        gear.range = GearRange::AnyTrader;
        const auto targets = range_targets(t.map, t.idx(0, 0), gear);
        REQUIRE(targets == std::vector<int> { t.idx(2, 0) });
    }

    SECTION("随机传送不参与规划")
    {
        PlannerGear gear;
        gear.range = GearRange::RandomNonCombat;
        REQUIRE(range_targets(b.map, center, gear).empty());
    }
}

TEST_CASE("planner.investment_profile", "[dsplanner]")
{
    // P -- T1(w10) -- . -- T2(w10) -- E；非终点模式下终点是禁区
    MapBuilder b(5, 1);
    PlannerCell trader1 = weighted(10.0);
    trader1.is_trader = true;
    PlannerCell trader2 = weighted(10.0);
    trader2.is_trader = true;
    b.cell(0, 0).cell(1, 0, trader1).cell(2, 0).cell(3, 0, trader2).cell(4, 0, endpoint());
    b.edge(0, 0, 1, 0).edge(1, 0, 2, 0).edge(2, 0, 3, 0).edge(3, 0, 4, 0);
    b.player(0, 0);

    SECTION("依次吃掉两个行商，绝不踏入终点")
    {
        const auto result = plan(b.map, {}, make_params(5, false));
        REQUIRE(result.has_route);
        REQUIRE_FALSE(result.reaches_endpoint);
        REQUIRE(result.score == 20.0);
        for (const auto& action : result.actions) {
            REQUIRE(action.target != b.idx(4, 0));
        }
    }

    SECTION("全图无正收益时最优分不为正（由调用方裁决放弃）")
    {
        b.map.cells[b.idx(1, 0)].weight = -5.0;
        b.map.cells[b.idx(3, 0)].weight = -5.0;
        const auto result = plan(b.map, {}, make_params(3, false));
        REQUIRE(result.has_route);
        REQUIRE(result.score <= 0.0);
    }

    SECTION("坎诺特的触须：跳到不连通的行商并计入使用收益")
    {
        MapBuilder far(4, 4);
        PlannerCell trader = weighted(10.0);
        trader.is_trader = true;
        far.cell(0, 0).cell(3, 3, trader);
        far.player(0, 0);

        PlannerGear tentacle;
        tentacle.range = GearRange::AnyTrader;
        tentacle.uses = 2;
        tentacle.ap_cost = 1;
        tentacle.use_cost = 0.5;
        tentacle.use_reward = 2.0;

        const auto result = plan(far.map, { tentacle }, make_params(3, false));
        REQUIRE(result.has_route);
        REQUIRE(result.actions.size() >= 1);
        REQUIRE_FALSE(result.actions.front().is_walk);
        REQUIRE(result.actions.front().target == far.idx(3, 3));
        // 第一跳：10 + 2 - 0.5 = 11.5；第二跳回不去（已无其他行商），若跳同格无意义
        REQUIRE(result.score >= 11.5);
    }
}

TEST_CASE("planner.endpoint_unreachable_policies", "[dsplanner]")
{
    // 图中根本没有终点
    MapBuilder b(3, 1);
    b.cell(0, 0).cell(1, 0, weighted(5.0)).cell(2, 0);
    b.edge(0, 0, 1, 0).edge(1, 0, 2, 0);
    b.player(0, 0);

    SECTION("bestEffort：输出最优刷分路线")
    {
        auto params = make_params(3, true);
        params.best_effort = true;
        const auto result = plan(b.map, {}, params);
        REQUIRE(result.has_route);
        REQUIRE_FALSE(result.reaches_endpoint);
        REQUIRE(result.score == 5.0);
    }

    SECTION("abandon：判无路线")
    {
        auto params = make_params(3, true);
        params.best_effort = false;
        const auto result = plan(b.map, {}, params);
        REQUIRE_FALSE(result.has_route);
    }
}

TEST_CASE("planner.determinism", "[dsplanner]")
{
    // 较复杂的图 + 多个加工品，两次规划结果必须完全一致
    MapBuilder b(6, 4);
    for (int c = 0; c < 6; ++c) {
        for (int r = 0; r < 4; ++r) {
            b.cell(c, r);
        }
    }
    b.map.cells[b.idx(1, 1)].weight = 6.0;
    b.map.cells[b.idx(4, 2)].weight = 8.0;
    b.map.cells[b.idx(2, 3)].weight = -3.0;
    b.map.cells[b.idx(5, 0)].is_endpoint = true;
    b.map.cells[b.idx(3, 1)].ap_gain = 1;
    for (int c = 0; c + 1 < 6; ++c) {
        for (int r = 0; r < 4; ++r) {
            b.edge(c, r, c + 1, r);
        }
    }
    for (int c = 0; c < 6; ++c) {
        for (int r = 0; r + 1 < 4; ++r) {
            b.edge(c, r, c, r + 1);
        }
    }
    b.player(0, 0);

    PlannerGear engine;
    engine.range = GearRange::Ring8;
    engine.uses = 3;
    engine.use_cost = 0.5;
    PlannerGear spring;
    spring.range = GearRange::Ring12;
    spring.uses = 1;
    spring.ap_cost = 0;
    spring.carryover = false;

    const auto params = make_params(7, true);
    const auto first = plan(b.map, { engine, spring }, params);
    const auto second = plan(b.map, { engine, spring }, params);

    REQUIRE(first.has_route);
    REQUIRE(first.has_route == second.has_route);
    REQUIRE(first.score == second.score);
    REQUIRE(first.reaches_endpoint == second.reaches_endpoint);
    REQUIRE(first.actions.size() == second.actions.size());
    for (size_t i = 0; i < first.actions.size(); ++i) {
        REQUIRE(first.actions[i].is_walk == second.actions[i].is_walk);
        REQUIRE(first.actions[i].gear_index == second.actions[i].gear_index);
        REQUIRE(first.actions[i].target == second.actions[i].target);
    }
}
