#pragma once
#include "Config/AbstractConfig.h"
#include "StrongholdStrategyGrid.h"
#include "StrongholdTierBadgeConfig.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"

namespace asst
{
// 卫戍协议极速模拟部署格位（1280x720 基准）
struct StrongholdTiles
{
    std::vector<Point> safe; // 敌人不经过的格子
    std::vector<Point> path; // 近战堵口顺序，首格是开局前排
    std::vector<Point> rear; // 首格在开局前排身后，仍会接敌
};

// 卫戍协议赛季配置
class StrongholdProtocolConfig final : public MAA_NS::SingletonHolder<StrongholdProtocolConfig>, public AbstractConfig
{
public:
    virtual ~StrongholdProtocolConfig() override = default;

    // 数值越高越优先，未列入名单时返回 0。编队时只考虑标记 squad 的条目。
    int priority(const std::string& name, bool squad_only = false) const;

    bool is_substitute(const std::string& name) const { return m_purchase.substitutes.contains(name); }

    bool is_squad_blacklisted(const std::string& name) const { return m_purchase.squad_blacklist.contains(name); }

    bool is_six_star(const std::string& name) const;

    // 同名干员有多个职业时取赛季指定的职业，无法确定时返回 Unknown。
    battle::Role get_role(const std::string& name) const;

    const StrongholdTierBadgeConfig& get_tier_badges() const { return m_tier_badges; }

    const StrongholdTiles& get_tiles() const { return m_tiles; }

    const StrongholdStrategyGrid& get_strategy_grid() const { return m_strategy_grid; }

protected:
    virtual bool parse(const json::value& json) override;

private:
    struct Purchase
    {
        using Oper = std::shared_ptr<const battle::OperProps>;
        using NamedOpers = std::unordered_map<std::string, Oper>;
        std::vector<std::pair<Oper, bool>> priority;
        NamedOpers substitutes;
        NamedOpers forms;
        std::unordered_set<std::string> squad_blacklist;
    };

    Purchase::Oper season_oper(const std::string& name) const;
    bool parse_purchase(const json::value& json, bool custom);
    Purchase m_purchase;
    StrongholdTierBadgeConfig m_tier_badges;
    StrongholdTiles m_tiles;
    StrongholdStrategyGrid m_strategy_grid {};
};

inline static auto& StrongholdProtocol = StrongholdProtocolConfig::get_instance();
}
