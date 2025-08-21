#include "RoguelikeCoppersConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

asst::RoguelikeCopper::RoguelikeCopper(
    std::string _name,
    CopperType _type,
    int col,
    int index,
    const std::string& theme) :
    name(std::move(_name)),
    type(_type),
    col(col),
    index(index)
{
    LogTraceFunction;

    // 从 m_coppers 中查找并设置优先级
    const auto& all_coppers = RoguelikeCoppers.get_coppers(theme);

    if (auto it = ranges::find_if(
            all_coppers,
            [&](const RoguelikeCopper& copper) {
                // 返回 copper.name 是否是 name 的字串
                return name.find(copper.name) != std::string::npos;
            });
        it != all_coppers.end()) {
        rarity = it->rarity;
        type = it->type;
        pickup_priority = it->pickup_priority;
        discard_priority = it->discard_priority;

        Log.info(__FUNCTION__, "copper found in config, name:", name, ", type:", static_cast<int>(type));
    }
    else {
        Log.error(__FUNCTION__, "copper not found in config, name:", name, ", type:", static_cast<int>(type));
    }
}

bool asst::RoguelikeCoppersConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_coppers.erase(theme);

    const auto& coppers_json = json.at("coppers");
    for (const auto& copper_json : coppers_json.as_array()) {
        std::string name = copper_json.at("name").as_string();

        // 解析稀有度
        CopperRarity rarity = CopperRarity::Low;
        const std::string& rarity_str = copper_json.at("rarity").as_string();
        if (rarity_str == "mid") {
            rarity = CopperRarity::Mid;
        }
        else if (rarity_str == "high") {
            rarity = CopperRarity::High;
        }

        // 解析类型（从名称前缀判断）
        CopperType type = CopperType::Heng;
        if (name.starts_with("厉-")) {
            type = CopperType::Li;
        }
        else if (name.starts_with("花-") == 0) {
            type = CopperType::Hua;
        }

        RoguelikeCopper copper;
        copper.name = std::move(name);
        copper.rarity = rarity;
        copper.type = type;
        copper.pickup_priority = copper_json.get("pickup_priority", 0);
        copper.discard_priority = copper_json.get("discard_priority", 2000);

        m_coppers[theme].emplace_back(std::move(copper));
    }

    return true;
}

void asst::RoguelikeCoppersConfig::clear()
{
    m_coppers.clear();
}
