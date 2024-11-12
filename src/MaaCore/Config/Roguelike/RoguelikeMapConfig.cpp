#include "RoguelikeMapConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeMapConfig::parse(const json::value& json)
{
    LogTraceFunction;

    static const std::unordered_map<std::string, RoguelikeNodeType> NodeTypeMapping = {
        { "CombatOps", RoguelikeNodeType::CombatOps },
        { "EmergencyOps", RoguelikeNodeType::EmergencyOps },
        { "DreadfulFoe", RoguelikeNodeType::DreadfulFoe },
        { "Encounter", RoguelikeNodeType::Encounter },
        { "Boons", RoguelikeNodeType::Boons },
        { "SafeHouse", RoguelikeNodeType::SafeHouse },
        { "Recreation", RoguelikeNodeType::Recreation },
        { "RogueTrader", RoguelikeNodeType::RogueTrader },
        { "RegionalCommissions", RoguelikeNodeType::RegionalCommissions },
        { "LostAndFound", RoguelikeNodeType::LostAndFound },
        { "Scout", RoguelikeNodeType::Scout },
        { "BoskyPassage", RoguelikeNodeType::BoskyPassage },
        { "Prophecy", RoguelikeNodeType::Prophecy },
        { "MysteriousPresage", RoguelikeNodeType::MysteriousPresage },
        { "FerociousPresage", RoguelikeNodeType::FerociousPresage },
        { "IdeaFilter", RoguelikeNodeType::IdeaFilter },
        { "FaceOff", RoguelikeNodeType::FaceOff }
    };

    const std::string theme = json.at("theme").as_string();
    m_templ_type_mappings.erase(theme);

    for (const auto& node_json : json.at("node").as_array()) {
        const RoguelikeNodeType node_type = NodeTypeMapping.at(node_json.at("type").as_string());
        for (const auto& template_json : node_json.at("template").as_array()) {
            const std::string node_template = template_json.as_string();
            m_templ_type_mappings[theme].emplace(node_template, node_type);
        }
    }
    return true;
}

void asst::RoguelikeMapConfig::clear()
{
    m_templ_type_mappings.clear();
}
