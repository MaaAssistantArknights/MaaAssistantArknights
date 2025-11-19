#include "RoguelikeCoppersConfig.h"

#include <unordered_map>

#include "Utils/Logger.hpp"

// 解析JSON配置文件，加载通宝数据
bool asst::RoguelikeCoppersConfig::parse(const json::value& json)
{
    LogTraceFunction;
    // 解析稀有度字符串为枚举值
    static const std::unordered_map<std::string, CopperRarity> rarity_map = { { "RARE", CopperRarity::RARE },
                                                                              { "SUPER_RARE",
                                                                                CopperRarity::SUPER_RARE },
                                                                              { "NORMAL", CopperRarity::NORMAL } };

    // 获取主题名称并清空该主题的现有数据
    const std::string theme = json.at("theme").as_string();
    m_coppers.erase(theme);

    // 解析通宝列表
    const auto& coppers_opt = json.find<json::value>("coppers");
    if (coppers_opt) {
        const auto& coppers_json = static_cast<std::vector<CoppersConfig>>(*coppers_opt);

        for (const auto& copper_json : coppers_json) {
            auto it = rarity_map.find(copper_json.rarity);
            RoguelikeCopper copper {
                .name = copper_json.name,
                .rarity = (it != rarity_map.end()) ? it->second : CopperRarity::NONE,
                .type = get_type_from_name(copper_json.name),
                .pickup_priority = copper_json.pickup_priority,
                .discard_priority = copper_json.discard_priority,
                .cast_discard_priority = copper_json.cast_discard_priority,
            };
            // 将解析完成的通宝添加到主题列表中
            m_coppers[theme].emplace_back(std::move(copper));
        }
    }
    return true;
}

// 清空所有配置数据
void asst::RoguelikeCoppersConfig::clear()
{
    m_coppers.clear();
}
