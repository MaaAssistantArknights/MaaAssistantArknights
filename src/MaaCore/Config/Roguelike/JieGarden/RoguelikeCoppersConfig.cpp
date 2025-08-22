#include "RoguelikeCoppersConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeCoppersConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_coppers.erase(theme);

    const auto& coppers_json = json.at("coppers");
    for (const auto& copper_json : coppers_json.as_array()) {
        RoguelikeCopper copper;
        copper.name = copper_json.at("name").as_string();

        // 解析稀有度
        if (const std::string& rarity_str = copper_json.at("rarity").as_string(); rarity_str == "mid") {
            copper.rarity = CopperRarity::Mid;
        }
        else if (rarity_str == "high") {
            copper.rarity = CopperRarity::High;
        }
        else {
            copper.rarity = CopperRarity::Low;
        }

        copper.type = get_type_from_name(copper.name);
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
