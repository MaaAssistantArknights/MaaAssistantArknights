#include "RoguelikeCopilotConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

std::optional<asst::RoguelikeBattleData> asst::RoguelikeCopilotConfiger::get_stage_data(const std::string& stage_name) const
{
    auto it = m_stage_data.find(stage_name);
    if (it == m_stage_data.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool asst::RoguelikeCopilotConfiger::parse(const json::value& json)
{
    for (const auto& stage_info : json.as_array()) {
        std::string stage_name = stage_info.at("stage_name").as_string();
        RoguelikeBattleData data;
        data.stage_name = stage_name;
        if (stage_info.contains("replacement_home")) {
            for (const auto& points : stage_info.at("replacement_home").as_array()) {
                const auto& points_array = points.as_array();
                Point point(points_array.at(0).as_integer(), points_array.at(1).as_integer());
                data.replacement_home.push_back(point);
            }
        }
        if (stage_info.contains("key_kills")) {
            for (const auto& key_kill : stage_info.at("key_kills").as_array()) {
                data.key_kills.emplace_back(key_kill.as_integer());
            }
        }
        m_stage_data.emplace(std::move(stage_name), std::move(data));
    }
    return true;
}
