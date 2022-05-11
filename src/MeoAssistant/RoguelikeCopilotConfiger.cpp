#include "RoguelikeCopilotConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::RoguelikeCopilotConfiger::parse(const json::value& json)
{
    std::string stage_name = json.at("stage_name").as_string();

    std::vector<RoguelikeBattleAction> actions_vec;

    for (const auto& action_info : json.at("actions").as_array()) {
        RoguelikeBattleAction action;

        static const std::unordered_map<std::string, BattleActionType> ActionTypeMapping = {
            { "Deploy", BattleActionType::Deploy },
            { "DEPLOY", BattleActionType::Deploy },
            { "deploy", BattleActionType::Deploy },
            { "部署", BattleActionType::Deploy },

            { "Skill", BattleActionType::UseSkill },
            { "SKILL", BattleActionType::UseSkill },
            { "skill", BattleActionType::UseSkill },
            { "技能", BattleActionType::UseSkill },

            { "Retreat", BattleActionType::Retreat },
            { "RETREAT", BattleActionType::Retreat },
            { "retreat", BattleActionType::Retreat },
            { "撤退", BattleActionType::Retreat },

            { "AllSkill", BattleActionType::UseAllSkill },
            { "allskill", BattleActionType::UseAllSkill },
            { "ALLSKILL", BattleActionType::UseAllSkill },
            { "Allskill", BattleActionType::UseAllSkill },
            { "全部技能", BattleActionType::UseAllSkill },
        };

        std::string type_str = action_info.get("type", "Deploy");

        if (auto iter = ActionTypeMapping.find(type_str);
            iter != ActionTypeMapping.end()) {
            action.type = iter->second;
        }
        else {
            action.type = BattleActionType::Deploy;
        }
        action.kills = action_info.get("kills", 0);

        if (action_info.contains("roles")) {
            for (const auto& role_info : action_info.at("roles").as_array()) {
                static const std::unordered_map<std::string, BattleRole> RoleMapping = {
                    { "MEDIC", BattleRole::Medic },
                    { "Medic", BattleRole::Medic },
                    { "medic", BattleRole::Medic },
                    { "医疗", BattleRole::Medic },

                    { "WARRIOR", BattleRole::Warrior },
                    { "Warrior", BattleRole::Warrior },
                    { "warrior", BattleRole::Warrior },
                    { "近卫", BattleRole::Warrior },

                    { "SPECIAL", BattleRole::Special },
                    { "Special", BattleRole::Special },
                    { "special", BattleRole::Special },
                    { "特种", BattleRole::Special },

                    { "SNIPER", BattleRole::Sniper },
                    { "Sniper", BattleRole::Sniper },
                    { "sniper", BattleRole::Sniper },
                    { "狙击", BattleRole::Sniper },

                    { "PIONEER", BattleRole::Pioneer },
                    { "Pioneer", BattleRole::Pioneer },
                    { "pioneer", BattleRole::Pioneer },
                    { "先锋", BattleRole::Pioneer },

                    { "TANK", BattleRole::Tank },
                    { "Tank", BattleRole::Tank },
                    { "tank", BattleRole::Tank },
                    { "重装", BattleRole::Tank },

                    { "SUPPORT", BattleRole::Support },
                    { "Support", BattleRole::Support },
                    { "support", BattleRole::Support },
                    { "辅助", BattleRole::Support },

                    { "CASTER", BattleRole::Caster },
                    { "Caster", BattleRole::Caster },
                    { "caster", BattleRole::Caster },
                    { "术师", BattleRole::Caster },

                    { "Drone", BattleRole::Drone },
                    { "DRONE", BattleRole::Drone },
                    { "drone", BattleRole::Drone },
                    { "召唤物", BattleRole::Drone },
                    { "无人机", BattleRole::Drone }
                };

                std::string role_name = role_info.as_string();
                if (auto iter = RoleMapping.find(role_name);
                    iter != RoleMapping.end()) {
                    action.roles.emplace_back(iter->second);
                }
                else {
                    Log.error("Unknown role", role_name);
                }
            }
        }

        action.location.x = action_info.get("location", 0, 0);
        action.location.y = action_info.get("location", 1, 0);

        static const std::unordered_map<std::string, BattleDeployDirection> DeployDirectionMapping = {
            { "Right", BattleDeployDirection::Right },
            { "RIGHT", BattleDeployDirection::Right },
            { "right", BattleDeployDirection::Right },
            { "右", BattleDeployDirection::Right },

            { "Left", BattleDeployDirection::Left },
            { "LEFT", BattleDeployDirection::Left },
            { "left", BattleDeployDirection::Left },
            { "左", BattleDeployDirection::Left },

            { "Up", BattleDeployDirection::Up },
            { "UP", BattleDeployDirection::Up },
            { "up", BattleDeployDirection::Up },
            { "上", BattleDeployDirection::Up },

            { "Down", BattleDeployDirection::Down },
            { "DOWN", BattleDeployDirection::Down },
            { "down", BattleDeployDirection::Down },
            { "下", BattleDeployDirection::Down },

            { "None", BattleDeployDirection::None },
            { "NONE", BattleDeployDirection::None },
            { "none", BattleDeployDirection::None },
            { "无", BattleDeployDirection::None },
        };

        std::string direction_str = action_info.get("direction", "Right");

        if (auto iter = DeployDirectionMapping.find(direction_str);
            iter != DeployDirectionMapping.end()) {
            action.direction = iter->second;
        }
        else {
            action.direction = BattleDeployDirection::Right;
        }
        action.waiting_cost = action_info.get("waiting_cost", false);
        action.pre_delay = action_info.get("pre_delay", 0);
        action.rear_delay = action_info.get("rear_delay", 0);
        action.time_out = action_info.get("timeout", INT_MAX);

        actions_vec.emplace_back(std::move(action));
    }

    m_battle_actions[std::move(stage_name)] = std::move(actions_vec);

    return true;
}
