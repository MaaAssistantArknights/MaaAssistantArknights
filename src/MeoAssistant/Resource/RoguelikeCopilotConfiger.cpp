#include "RoguelikeCopilotConfiger.h"

#include <meojson/json.hpp>

#include "Utils/AsstUtils.hpp"
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
        if (auto opt = stage_info.find<json::array>("replacement_home")) {
            for (auto& point : opt.value()) {
                ReplacementHome home;
                home.location = Point(point["location"][0].as_integer(), point["location"][1].as_integer());
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
        data.use_dice_stage = !stage_info.get("not_use_dice", false);

        data.stop_melee_deploy_num = stage_info.get("stop_melee_deploy_num", INT_MAX);
        data.deploy_ranged_num = stage_info.get("deploy_ranged_num", 0);
        if (data.deploy_ranged_num == 0) {
            data.stop_melee_deploy_num = INT_MAX;
        }

        constexpr int RoleNumber = 9;
        static constexpr std::array<BattleRole, RoleNumber> RoleOrder = {
            BattleRole::Warrior, BattleRole::Pioneer, BattleRole::Medic,   BattleRole::Tank,  BattleRole::Sniper,
            BattleRole::Caster,  BattleRole::Support, BattleRole::Special, BattleRole::Drone,
        };
        static const std::unordered_map<std::string, BattleRole> NameToRole = {
            { "warrior", BattleRole::Warrior }, { "WARRIOR", BattleRole::Warrior },
            { "Warrior", BattleRole::Warrior }, { "近卫", BattleRole::Warrior },

            { "pioneer", BattleRole::Pioneer }, { "PIONEER", BattleRole::Pioneer },
            { "Pioneer", BattleRole::Pioneer }, { "先锋", BattleRole::Pioneer },

            { "medic", BattleRole::Medic },     { "MEDIC", BattleRole::Medic },
            { "Medic", BattleRole::Medic },     { "医疗", BattleRole::Medic },

            { "tank", BattleRole::Tank },       { "TANK", BattleRole::Tank },
            { "Tank", BattleRole::Tank },       { "重装", BattleRole::Tank },

            { "sniper", BattleRole::Sniper },   { "SNIPER", BattleRole::Sniper },
            { "Sniper", BattleRole::Sniper },   { "狙击", BattleRole::Sniper },

            { "caster", BattleRole::Caster },   { "CASTER", BattleRole::Caster },
            { "Caster", BattleRole::Caster },   { "术师", BattleRole::Caster },

            { "support", BattleRole::Support }, { "SUPPORT", BattleRole::Support },
            { "Support", BattleRole::Support }, { "辅助", BattleRole::Support },

            { "special", BattleRole::Special }, { "SPECIAL", BattleRole::Special },
            { "Special", BattleRole::Special }, { "特种", BattleRole::Special },

            { "drone", BattleRole::Drone },     { "DRONE", BattleRole::Drone },
            { "Drone", BattleRole::Drone },     { "无人机", BattleRole::Drone },
        };
        auto to_lower = [](char c) -> char { return static_cast<char>(std::tolower(c)); };
        if (auto opt = stage_info.find<json::array>("role_order")) {
            const auto& raw_roles = opt.value();
            using views::filter, views::transform;
            std::unordered_set<BattleRole> specified_role;
            std::vector<BattleRole> role_order;
            bool is_legal = true;
            if (ranges::find_if_not(raw_roles | views::all, std::mem_fn(&json::value::is_string)) != raw_roles.end()) {
                Log.error("BattleRole should be string");
                return false;
            }
            auto roles = raw_roles | filter(&json::value::is_string) | transform(&json::value::as_string) |
                         transform([&](std::string name) {
                             ranges::for_each(name, to_lower);
                             return std::move(name);
                         });
            for (const std::string& role_name : roles) {
                auto iter = NameToRole.find(role_name);
                if (iter == NameToRole.end()) [[unlikely]] {
                    Log.error("Unknown BattleRole:", role_name);
                    is_legal = false;
                    break;
                }
                if (specified_role.contains(iter->second)) [[unlikely]] {
                    Log.error("Duplicated BattleRole:", role_name);
                    is_legal = false;
                    break;
                }
                specified_role.emplace(iter->second);
                role_order.emplace_back(iter->second);
            }
            if (is_legal) [[likely]] {
                ranges::copy(RoleOrder | filter([&](BattleRole role) { return !specified_role.contains(role); }),
                             std::back_inserter(role_order));
                if (role_order.size() != RoleNumber) [[unlikely]] {
                    Log.error("Unexpected role_order size:", role_order.size());
                    return false;
                }
                ranges::move(role_order, data.role_order.begin());
            }
            else {
                Log.error("Illegal role_order detected");
                return false;
            }
        }
        else {
            data.role_order = RoleOrder;
        }

        if (auto opt = stage_info.find<json::array>("force_deploy_direction")) {
            for (auto& point : opt.value()) {
                ForceDeployDirection fd_dir;
                Point location = Point(point["location"][0].as_integer(), point["location"][1].as_integer());
                const std::string& direction_str = point.get("direction", "none");
                if (auto iter = DeployDirectionMapping.find(direction_str); iter != DeployDirectionMapping.end()) {
                    fd_dir.direction = iter->second;
                }
                else {
                    fd_dir.direction = BattleDeployDirection::None;
                }
                if (fd_dir.direction == BattleDeployDirection::None) [[unlikely]] {
                    Log.error("Unknown direction");
                    return false;
                }
                std::unordered_set<BattleRole> role;
                for (auto& role_name : point["role"].as_array()) {
                    auto iter = NameToRole.find(role_name.as_string());
                    if (iter == NameToRole.end()) [[unlikely]] {
                        Log.error("Unknown role name:", role_name);
                        return false;
                    }
                    role.emplace(iter->second);
                }
                fd_dir.role = std::move(role);
                data.force_deploy_direction.emplace(location, fd_dir);
            }
        }

        m_stage_data.emplace(std::move(stage_name), std::move(data));
    }
    return true;
}
