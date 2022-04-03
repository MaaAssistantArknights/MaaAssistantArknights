#include "CopilotConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::CopilotConfiger::parse(const json::value& json)
{
    std::string stage_name = json.at("stage_name").as_string();

    BattleActionsGroup battle_actions;

    if (json.contains("groups")) {
        for (const auto& group_info : json.at("groups").as_array()) {
            std::string group_name = group_info.at("name").as_string();
            std::vector<BattleDeployOper> oper_vec;
            for (const auto& oper_info : group_info.at("opers").as_array()) {
                BattleDeployOper oper;
                oper.name = oper_info.at("name").as_string();
                oper.skill = oper_info.get("skill", 1);
                oper.skill_usage = static_cast<BattleSkillUsage>(oper_info.get("skill_usage", 0));
                oper_vec.emplace_back(std::move(oper));
            }
            battle_actions.groups.emplace(std::move(group_name), std::move(oper_vec));
        }
    }

    if (json.contains("opers")) {
        for (const auto& oper_info : json.at("opers").as_array()) {
            BattleDeployOper oper;
            oper.name = oper_info.at("name").as_string();
            oper.skill = oper_info.get("skill", 1);
            oper.skill_usage = static_cast<BattleSkillUsage>(oper_info.get("skill_usage", 0));

            // 单个干员的，干员名直接作为组名
            std::string group_name = oper.name;

            battle_actions.groups.emplace(std::move(group_name), std::vector{ std::move(oper) });
        }
    }

    for (const auto& action_info : json.at("actions").as_array()) {
        BattleAction action;
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

            { "SpeedUp", BattleActionType::SwitchSpeed },
            { "SPEEDUP", BattleActionType::SwitchSpeed },
            { "Speedup", BattleActionType::SwitchSpeed },
            { "speedup", BattleActionType::SwitchSpeed },
            { "二倍速", BattleActionType::SwitchSpeed },

            { "BulletTime", BattleActionType::SwitchSpeed },
            { "BULLETTIME", BattleActionType::SwitchSpeed },
            { "Bullettime", BattleActionType::SwitchSpeed },
            { "bullettime", BattleActionType::SwitchSpeed },
            { "子弹时间", BattleActionType::SwitchSpeed },

            { "SkillUsage", BattleActionType::SkillUsage },
            { "SKILLUSAGE", BattleActionType::SkillUsage },
            { "Skillusage", BattleActionType::SkillUsage },
            { "skillusage", BattleActionType::SkillUsage },
            { "技能用法", BattleActionType::SkillUsage },
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
        action.group_name = action_info.get("name", std::string());

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
        action.modify_usage = static_cast<BattleSkillUsage>(action_info.get("skill_usage", 0));
        action.pre_delay = action_info.get("pre_delay", 0);
        action.rear_delay = action_info.get("rear_delay", 0);
        action.time_out = action_info.get("timeout", INT_MAX);

        battle_actions.actions.emplace_back(std::move(action));
    }

    m_battle_actions[std::move(stage_name)] = std::move(battle_actions);

    return true;
}
