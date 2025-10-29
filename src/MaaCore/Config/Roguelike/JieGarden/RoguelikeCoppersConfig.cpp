#include "RoguelikeCoppersConfig.h"

#include <unordered_map>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

// 解析JSON配置文件，加载通宝数据
bool asst::RoguelikeCoppersConfig::parse(const json::value& json)
{
    LogTraceFunction;

    // 获取主题名称并清空该主题的现有数据
    const std::string theme = json.at("theme").as_string();
    m_coppers.erase(theme);

    // 解析通宝列表
    const auto& coppers_json = json.at("coppers");
    for (const auto& copper_json : coppers_json.as_array()) {
        RoguelikeCopper copper;
        copper.name = copper_json.at("name").as_string();

        // 解析稀有度字符串为枚举值
        const std::unordered_map<std::string, CopperRarity> rarity_map = { { "RARE", CopperRarity::RARE },
                                                                           { "SUPER_RARE", CopperRarity::SUPER_RARE },
                                                                           { "NORMAL", CopperRarity::NORMAL } };
        const std::string& rarity_str = copper_json.at("rarity").as_string();
        auto it = rarity_map.find(rarity_str);
        copper.rarity = (it != rarity_map.end()) ? it->second : CopperRarity::NONE;

        // 根据名称推断通宝类型
        copper.type = get_type_from_name(copper.name);

        // 读取优先级配置，默认值为0（拾取优先级）和1000（丢弃优先级）
        copper.pickup_priority = copper_json.get("pickup_priority", 0);
        copper.discard_priority = copper_json.get("discard_priority", 1000);
        copper.cast_discard_priority = copper_json.get("cast_discard_priority", -1);

        // 将解析完成的通宝添加到主题列表中
        m_coppers[theme].emplace_back(std::move(copper));
    }

    return true;
}

// 清空所有配置数据
void asst::RoguelikeCoppersConfig::clear()
{
    m_coppers.clear();
}
