#include "RoguelikeMapConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

using namespace asst;

static const std::unordered_map<std::string, RoguelikeNodeType> NODE_TYPE_MAPPING = {
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
    { "FaceOff", RoguelikeNodeType::FaceOff },
    { "Legend", RoguelikeNodeType::Legend },
    { "Omissions", RoguelikeNodeType::Omissions },
    { "Doubts", RoguelikeNodeType::Doubts },
    { "Disaster", RoguelikeNodeType::Disaster },
    { "Playtime", RoguelikeNodeType::Playtime },
    { "OldShop", RoguelikeNodeType::OldShop },
    { "YiTrader", RoguelikeNodeType::YiTrader },
    { "Scheme", RoguelikeNodeType::Scheme }
};

bool RoguelikeMapConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_templ_type_mappings.erase(theme);

    for (const auto& node_json : json.at("nodes").as_array()) {
        const std::string& type_name = node_json.at("type").as_string();
        auto it = NODE_TYPE_MAPPING.find(type_name);
        if (it == NODE_TYPE_MAPPING.end()) {
            Log.error("RoguelikeMapConfig::parse | Unknown roguelike node type name:", type_name);
            return false;
        }
        RoguelikeNodeType type = it->second;
        for (const auto& template_json : node_json.at("template").as_array()) {
            const std::string node_template = template_json.as_string();
            m_templ_type_mappings[theme].emplace(node_template, type);
        }
    }
    return true;
}

RoguelikeNodeType RoguelikeMapConfig::templ2type(const std::string& theme, const std::string& templ_name) const
{
    auto outer_it = m_templ_type_mappings.find(theme);
    if (outer_it == m_templ_type_mappings.end()) {
        Log.error(__FUNCTION__, "| Unsupported roguelike theme:", theme);
        return RoguelikeNodeType::Unknown;
    }
    const auto& templ_type_mapping = outer_it->second;
    auto inner_it = templ_type_mapping.find(templ_name);
    if (inner_it == templ_type_mapping.end()) {
        Log.error(__FUNCTION__, "| No roguelike node type is specified for template", templ_name);
        return RoguelikeNodeType::Unknown;
    }
    return inner_it->second;
}

std::string RoguelikeMapConfig::type2name(RoguelikeNodeType type)
{
    static const std::unordered_map<RoguelikeNodeType, std::string> NODE_NAME_MAPPING = []() {
        std::unordered_map<RoguelikeNodeType, std::string> mapping;
        mapping.reserve(NODE_TYPE_MAPPING.size());
        for (const auto& [name_, type_] : NODE_TYPE_MAPPING) {
            mapping.emplace(type_, name_);
        }
        return mapping;
    }();

    auto it = NODE_NAME_MAPPING.find(type);
    if (it == NODE_NAME_MAPPING.end()) {
        Log.error(__FUNCTION__, "| Unknown roguelike node type", static_cast<int>(type));
        return "Unknown";
    }
    return it->second;
}

RoguelikeNodeType RoguelikeMapConfig::name2type(const std::string& type_name)
{
    auto it = NODE_TYPE_MAPPING.find(type_name);
    if (it == NODE_TYPE_MAPPING.end()) {
        Log.error(__FUNCTION__, "| Unknown roguelike node type name:", type_name);
        return RoguelikeNodeType::Unknown;
    }
    return it->second;
}
