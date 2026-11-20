#pragma once
#include "Config/AbstractConfig.h"

#include <string>
#include <unordered_map>
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
    Point lift { 507, 433 }; // 拿起干员后的悬停点（不遮挡目标格）
};

// 卫戍协议极速模拟购买权重与干员分类
class StrongholdProtocolConfig final : public MAA_NS::SingletonHolder<StrongholdProtocolConfig>, public AbstractConfig
{
public:
    virtual ~StrongholdProtocolConfig() override = default;

    const std::vector<std::string>& get_priority() const { return m_priority; }

    const std::vector<std::string>& get_dps() const { return m_dps; }

    // 同名干员有多个职业时取赛季指定的职业，无法确定时返回 Unknown。
    battle::Role get_role(const std::string& name) const;

    const StrongholdTiles& get_tiles() const { return m_tiles; }

protected:
    virtual bool parse(const json::value& json) override;

    std::vector<std::string> m_priority; // 购买优先级，数组序即权重序
    std::vector<std::string> m_dps;
    std::unordered_map<std::string, battle::Role> m_role_overrides;
    StrongholdTiles m_tiles;
};

inline static auto& StrongholdProtocol = StrongholdProtocolConfig::get_instance();
}
