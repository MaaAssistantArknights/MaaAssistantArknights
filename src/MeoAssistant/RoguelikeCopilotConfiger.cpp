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
        if (auto opt = stage_info.find<json::array>("replacement_home")) {
            for (auto& point : opt.value()) {
                data.replacement_home.emplace_back(static_cast<int>(point[0]), static_cast<int>(point[1]));
            }
        }
        if (auto opt = stage_info.find<json::array>("key_kills")) {
            for (const auto& key_kill : opt.value()) {
                data.key_kills.emplace_back(static_cast<int>(key_kill));
            }
        }
        m_stage_data.emplace(std::move(stage_name), std::move(data));
    }
    return true;
}
