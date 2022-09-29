#include "RoguelikeCopilotConfiger.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

std::optional<asst::RoguelikeBattleData> asst::RoguelikeCopilotConfiger::get_stage_data(
    const std::string& stage_name) const
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
                ReplacementHome home;
                home.location = Point(point["location"][0].as_integer(), point["location"][1].as_integer());
                static const std::unordered_map<std::string, BattleDeployDirection> DeployDirectionMapping = {
                    { "Right", BattleDeployDirection::Right }, { "RIGHT", BattleDeployDirection::Right },
                    { "right", BattleDeployDirection::Right }, { "右", BattleDeployDirection::Right },

                    { "Left", BattleDeployDirection::Left },   { "LEFT", BattleDeployDirection::Left },
                    { "left", BattleDeployDirection::Left },   { "左", BattleDeployDirection::Left },

                    { "Up", BattleDeployDirection::Up },       { "UP", BattleDeployDirection::Up },
                    { "up", BattleDeployDirection::Up },       { "上", BattleDeployDirection::Up },

                    { "Down", BattleDeployDirection::Down },   { "DOWN", BattleDeployDirection::Down },
                    { "down", BattleDeployDirection::Down },   { "下", BattleDeployDirection::Down },

                    { "None", BattleDeployDirection::None },   { "NONE", BattleDeployDirection::None },
                    { "none", BattleDeployDirection::None },   { "无", BattleDeployDirection::None },
                };
                const std::string& direction_str = point.get("direction", "none");
                if (auto iter = DeployDirectionMapping.find(direction_str); iter != DeployDirectionMapping.end()) {
                    home.direction = iter->second;
                }
                else {
                    home.direction = BattleDeployDirection::None;
                }
                data.replacement_home.emplace_back(std::move(home));
            }
        }
        if (auto opt = stage_info.find<json::array>("blacklist_location")) {
            for (auto& point : opt.value()) {
                data.blacklist_location.emplace(point[0].as_integer(), point[1].as_integer());
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
