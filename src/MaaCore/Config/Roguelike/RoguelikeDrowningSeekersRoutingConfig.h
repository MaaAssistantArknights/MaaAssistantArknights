#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Config/AbstractConfig.h"
#include "RoguelikeMapConfig.h"
#include "Task/Roguelike/Map/RoguelikeDrowningSeekersRoutePlanner.h"

namespace asst
{
// 加工品定义（resource/roguelike/DrowningSeekers/routing.json 的 gears 项）
struct DrowningSeekersGearInfo
{
    std::string name;
    drowning_seekers::GearRange range = drowning_seekers::GearRange::Any;
    int distance = 0;         // range == Line 时的最大直线距离
    int max_uses = 1;
    int ap_cost = 1;          // 每次移动消耗行动力
    int ap_gain = 0;          // 每次移动后获得行动力（简易遥控器 +3）
    bool carryover = true;    // 能否携带至下一区域
    bool controllable = true; // false = 落点随机（小八界）
};

// 策略档案：节点权重、终点约束与加工品使用成本
struct DrowningSeekersStrategyProfile
{
    std::string name;
    bool endpoint_required = false;
    bool shortest_endpoint = false;       // 到终点模式：按动作数优先，不为节点收益绕路
    bool avoid_combat_first = false;      // 终点模式先最小化战斗节点数量，再比较路线长度
    bool best_effort_when_unreachable = true; // endpointUnreachable: bestEffort / abandon
    bool abandon_when_no_positive = false;    // 最优路线收益 <= 0 时放弃本局
    std::unordered_map<RoguelikeNodeType, double> node_weights;
    double gear_use_cost = 0.5;
    double non_carryover_use_cost = 0.0;
    std::unordered_map<std::string, double> gear_use_reward; // 按加工品名
    double leftover_ap_weight = 0.1;
};

// 黑流树海迷宫导航配置（resource/roguelike/DrowningSeekers/routing.json）
class RoguelikeDrowningSeekersRoutingConfig final
    : public MAA_NS::SingletonHolder<RoguelikeDrowningSeekersRoutingConfig>,
      public AbstractConfig
{
public:
    virtual ~RoguelikeDrowningSeekersRoutingConfig() override = default;

    const std::vector<DrowningSeekersGearInfo>& gears() const { return m_gears; }
    const DrowningSeekersGearInfo* gear_by_name(const std::string& name) const;

    // 进入未访问节点获得的行动力（羽瞰点 = 1，其余 0）
    int node_ap_gain(RoguelikeNodeType type) const;
    // 该类型节点是否成对传送（曲折密道）
    bool node_teleport_paired(RoguelikeNodeType type) const;

    bool is_combat(RoguelikeNodeType type) const { return m_combat_types.contains(type); }
    bool is_trader(RoguelikeNodeType type) const { return m_trader_types.contains(type); }
    bool is_endpoint(RoguelikeNodeType type) const { return m_endpoint_types.contains(type); }

    // 该模式无策略档案时返回 nullptr（= 不启用迷宫导航）
    const DrowningSeekersStrategyProfile* strategy_for_mode(int mode) const;

private:
    virtual bool parse(const json::value& json) override;

    struct NodeEffect
    {
        int ap_gain = 0;
        bool teleport_paired = false;
    };

    std::vector<DrowningSeekersGearInfo> m_gears;
    std::unordered_map<RoguelikeNodeType, NodeEffect> m_node_effects;
    std::unordered_set<RoguelikeNodeType> m_combat_types;
    std::unordered_set<RoguelikeNodeType> m_trader_types;
    std::unordered_set<RoguelikeNodeType> m_endpoint_types;
    std::unordered_map<std::string, DrowningSeekersStrategyProfile> m_strategies;
    std::unordered_map<int, std::string> m_mode_strategies;
};

inline static auto& DrowningSeekersRoutingInfo = RoguelikeDrowningSeekersRoutingConfig::get_instance();
}
