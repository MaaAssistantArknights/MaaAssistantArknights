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
        if (auto opt = stage_info.find<bool>("not_use_dice")) {
            data.use_dice_stage = (!opt.value());
        }
        constexpr int RoleNumber = 9;
        static const std::array<BattleRole, RoleNumber> RoleOrder = {
            BattleRole::Warrior, BattleRole::Pioneer, BattleRole::Medic,   BattleRole::Tank,  BattleRole::Sniper,
            BattleRole::Caster,  BattleRole::Support, BattleRole::Special, BattleRole::Drone,
        };
        static const std::unordered_map<std::string, BattleRole> NameToRole = {
            { "caster", BattleRole::Caster }, { "medic", BattleRole::Medic },   { "pioneer", BattleRole::Pioneer },
            { "sniper", BattleRole::Sniper }, { "special", BattleRole::Special }, { "support", BattleRole::Support },
            { "tank", BattleRole::Tank },   { "warrior", BattleRole::Warrior }, { "drone", BattleRole::Drone }
        };
        auto to_lower = [](char c) -> char { return static_cast<char>(std::tolower(c)); };
        if (auto opt = stage_info.find<json::array>("role_order")) {
            std::unordered_set<BattleRole> specified_role;
            std::vector<BattleRole> role_order_vec;
            bool is_legal = true;
            for (size_t i = 0; i < opt.value().size(); ++i) {
                std::string role_name = static_cast<std::string>(opt.value().at(i));
                ranges::transform(role_name, role_name.begin(), to_lower);
                auto iter = NameToRole.find(role_name);
                if (iter == NameToRole.end()) {
                    is_legal = false;
                    break;
                }
                specified_role.emplace(iter->second);
                role_order_vec.emplace_back(iter->second);
            }
            for (const auto& role : RoleOrder) {
                if (!specified_role.contains(role)) {
                    role_order_vec.emplace_back(role);
                }
            }
            if (is_legal) {
                std::move(role_order_vec.begin(), role_order_vec.end(), data.role_order.begin());
            }
            else {
                data.role_order = RoleOrder;
            }
        }
        else {
            data.role_order = RoleOrder;
        }
        m_stage_data.emplace(std::move(stage_name), std::move(data));
    }
    return true;
}
