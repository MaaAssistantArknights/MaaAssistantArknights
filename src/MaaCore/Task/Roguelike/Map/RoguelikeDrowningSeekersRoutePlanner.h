#pragma once

// 黑流树海（DrowningSeekers）迷宫路线规划器：束搜索求解带行动力预算的 orienteering 问题。
// 刻意零 MaaCore 依赖（仅标准库），以便被 unit_test 直接编译；
// MAA 侧类型（RoguelikeNodeType、加工品配置等）由插件翻译成本文件的输入结构。

#include <vector>

namespace asst::drowning_seekers
{
// 加工品移动范围。Line 配合 distance 使用；Ring12 = Ring8 ∪ {(±2,0),(0,±2)}（剑雨范围）。
enum class GearRange
{
    Line,            // 直线 distance 格内任意节点
    Ring8,           // 相邻 8 格
    Ring12,          // 相邻 12 格
    Any,             // 任意节点
    AnyNonCombat,    // 任意非作战节点
    AnyTrader,       // 任意行商节点
    RandomNonCombat, // 随机传送（落点不可控，规划器不主动使用）
};

struct PlannerCell
{
    bool exists = false;
    bool visited = false;     // 已访问（铭牌前有 ✓）：进入不再获得 weight / ap_gain
    bool is_endpoint = false; // 进入即结束本层（险路尽头/险路小径/险路恶敌）
    bool is_combat = false;   // AnyNonCombat 落点过滤用
    bool is_trader = false;   // AnyTrader 落点过滤用
    double weight = 0.0;      // 进入未访问节点的奖励（负=惩罚），路点为 0
    int ap_gain = 0;          // 进入未访问节点获得的行动力（羽瞰点 = 1）
    int teleport_twin = -1;   // 曲折密道孪生格 index（进入本格后免费移动到该格），无则 -1
};

struct PlannerMap
{
    int cols = 0;
    int rows = 0;
    int player = -1;                   // 玩家格 index（index = row * cols + col）
    std::vector<PlannerCell> cells;    // size == cols * rows
    std::vector<std::vector<int>> adj; // 徒步邻接表：adj[index] = 存在连通边的相邻格 index
};

struct PlannerGear
{
    GearRange range = GearRange::Any;
    int distance = 0;        // 仅 Line 有效
    int uses = 0;            // 剩余次数
    int ap_cost = 1;         // 每次移动消耗的行动力
    int ap_gain = 0;         // 每次移动后获得的行动力（简易遥控器 +3）
    double use_cost = 0.0;   // 每次使用的机会成本（从 score 扣除）
    double use_reward = 0.0; // 每次使用的额外收益（如触须的源石锭折算）
    bool carryover = true;   // 能否携带至下一区域（平局时优先省着可携带的）
    bool controllable = true;
};

struct PlannerParams
{
    int action_points = 0;
    bool endpoint_required = false; // 路线必须结束在 is_endpoint 格
    bool shortest_endpoint = false; // 终点模式按动作数做 BFS，不为中途收益绕路
    bool avoid_combat_first = false; // 终点模式先最少战斗，再最短动作数
    bool best_effort = true;        // 终点不可达时：true=输出最优刷分路线（接受追猎），false=判无路线
    double leftover_ap_weight = 0.0;
    int beam_width = 256;
    int max_depth = 24;
};

struct PlannerAction
{
    bool is_walk = true;
    int gear_index = -1; // is_walk == false 时为 gears 下标
    int target = -1;     // 进入的格 index（若为密道，进入后实际位于其孪生格）
};

struct PlannerResult
{
    bool has_route = false;
    bool reaches_endpoint = false;
    double score = 0.0;                 // 路线收益（含剩余行动力折算）；是否达到放弃阈值由调用方裁决
    std::vector<PlannerAction> actions; // 依序动作；执行方通常只执行第一个，之后重识别重规划
};

// 主入口。gears 传入当前持有的加工品（uses > 0）；不可控加工品会被忽略。
PlannerResult plan(const PlannerMap& map, const std::vector<PlannerGear>& gears, const PlannerParams& params);

// 范围几何：从 from 出发按 gear.range 可达的格 index 列表（升序；不含 from；仅 exists 格；
// 不考虑 endpoint/行动力等约束）。供 plan 内部与单测使用。
std::vector<int> range_targets(const PlannerMap& map, int from, const PlannerGear& gear);
}
