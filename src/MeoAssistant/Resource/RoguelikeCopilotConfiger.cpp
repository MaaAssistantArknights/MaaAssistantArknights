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

        if (auto opt = stage_info.find<json::value>("force_air_defense_when_deploy_blocking_num")) {
            data.stop_deploy_blocking_num = opt.value().get("melee_num", INT_MAX);
            data.force_deploy_air_defense_num = opt.value().get("air_defense_num", 0);
            if (data.force_deploy_air_defense_num == 0) {
                data.stop_deploy_blocking_num = INT_MAX;
            }
            data.force_ban_medic = opt.value().get("ban_medic", false);
        }
        else {
            data.stop_deploy_blocking_num = INT_MAX;
            data.force_deploy_air_defense_num = 0;
            data.force_ban_medic = false;
        }

        constexpr int RoleNumber = 9;
        static constexpr std::array<BattleRole, RoleNumber> RoleOrder = {
            BattleRole::Warrior, BattleRole::Pioneer, BattleRole::Medic,   BattleRole::Tank,  BattleRole::Sniper,
            BattleRole::Caster,  BattleRole::Support, BattleRole::Special, BattleRole::Drone,
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
                const auto role = get_role_type(role_name);
                if (role == BattleRole::Unknown) [[unlikely]] {
                    Log.error("Unknown BattleRole:", role_name);
                    is_legal = false;
                    break;
                }
                if (specified_role.contains(role)) [[unlikely]] {
                    Log.error("Duplicated BattleRole:", role_name);
                    is_legal = false;
                    break;
                }
                specified_role.emplace(role);
                role_order.emplace_back(role);
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
                std::unordered_set<BattleRole> fd_role;
                for (auto& role_name : point["role"].as_array()) {
                    const auto role = get_role_type(role_name.as_string());
                    if (role == BattleRole::Unknown) [[unlikely]] {
                        Log.error("Unknown role name:", role_name);
                        return false;
                    }
                    fd_role.emplace(role);
                }
                fd_dir.role = std::move(fd_role);
                data.force_deploy_direction.emplace(location, fd_dir);
            }
        }

        m_stage_data.emplace(std::move(stage_name), std::move(data));
    }
    return true;
}
