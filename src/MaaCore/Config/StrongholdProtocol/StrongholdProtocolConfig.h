#pragma once
#include "Config/AbstractConfig.h"

#include <string>
#include <utility>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
// 卫戍协议极速模拟部署格位（1280x720 基准）
struct StrongholdTiles
{
    std::vector<Point> safe; // 木框高台安全位（敌人不经过），优先给远程输出
    std::vector<Point> path; // 最右三列最上两行敌人路径，按优先顺序
    std::vector<Point> rest; // 其余可部署格
    Point lift { 507, 433 }; // 拿起干员后的悬停点（不遮挡目标格）
};

// 卫戍协议极速模拟购买权重与干员分类
class StrongholdProtocolConfig final : public MAA_NS::SingletonHolder<StrongholdProtocolConfig>, public AbstractConfig
{
public:
    virtual ~StrongholdProtocolConfig() override = default;

    const std::vector<std::string>& get_priority() const { return m_priority; }

    const std::vector<std::string>& get_safe_keys() const { return m_safe_keys; }

    const std::vector<std::string>& get_dps() const { return m_dps; }

    const std::vector<std::string>& get_healers() const { return m_healers; }

    const StrongholdTiles& get_tiles() const { return m_tiles; }

protected:
    virtual bool parse(const json::value& json) override;

    std::vector<std::string> m_priority; // 购买优先级，数组序即权重序
    std::vector<std::string> m_safe_keys;
    std::vector<std::string> m_dps;
    std::vector<std::string> m_healers;
    StrongholdTiles m_tiles;
};

inline static auto& StrongholdProtocol = StrongholdProtocolConfig::get_instance();
}
