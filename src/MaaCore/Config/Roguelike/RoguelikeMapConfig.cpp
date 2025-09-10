#include "RoguelikeMapConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeMapConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_templ_type_mappings.erase(theme);

    for (const auto& node_json : json.at("node").as_array()) {
        const RoguelikeNodeType node_type = get_node_type(node_json.at("type").as_string());
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
