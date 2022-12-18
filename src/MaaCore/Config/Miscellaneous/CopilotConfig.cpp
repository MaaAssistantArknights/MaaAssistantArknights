#include "CopilotConfig.h"

#include <meojson/json.hpp>

#include "TilePack.h"
#include "Utils/Logger.hpp"

using namespace asst::battle;
using namespace asst::battle::copilot;

void asst::CopilotConfig::clear()
{
    m_data = decltype(m_data)();
    m_stage_name.clear();
}

bool asst::CopilotConfig::parse(const json::value& json)
{
    LogTraceFunction;
    clear();

    m_stage_name = json.at("stage_name").as_string();

    m_data.title = json.get("doc", "title", std::string());
    m_data.title_color = json.get("doc", "title_color", std::string());
    m_data.details = json.get("doc", "details", std::string());
    m_data.details_color = json.get("doc", "details_color", std::string());

    if (auto opt = json.find<json::array>("groups")) {
        for (const auto& group_info : opt.value()) {
            std::string group_name = group_info.at("name").as_string();
            std::vector<OperUsage> oper_vec;
            for (const auto& oper_info : group_info.at("opers").as_array()) {
                OperUsage oper;
                oper.name = oper_info.at("name").as_string();
                oper.skill = oper_info.get("skill", 1);
                oper.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 0));
                oper_vec.emplace_back(std::move(oper));
            }
            m_data.groups.emplace(std::move(group_name), std::move(oper_vec));
        }
    }

    if (auto opt = json.find<json::array>("opers")) {
        for (const auto& oper_info : opt.value()) {
            OperUsage oper;
            oper.name = oper_info.at("name").as_string();
            oper.skill = oper_info.get("skill", 1);
            oper.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 0));

            // 单个干员的，干员名直接作为组名
            std::string group_name = oper.name;
            m_data.groups.emplace(std::move(group_name), std::vector { std::move(oper) });
        }
    }

    for (const auto& action_info : json.at("actions").as_array()) {
        Action action;
        static const std::unordered_map<std::string, ActionType> ActionTypeMapping = {
            { "Deploy", ActionType::Deploy },
            { "DEPLOY", ActionType::Deploy },
            { "deploy", ActionType::Deploy },
            { "部署", ActionType::Deploy },

            { "Skill", ActionType::UseSkill },
            { "SKILL", ActionType::UseSkill },
            { "skill", ActionType::UseSkill },
            { "技能", ActionType::UseSkill },

            { "Retreat", ActionType::Retreat },
            { "RETREAT", ActionType::Retreat },
            { "retreat", ActionType::Retreat },
            { "撤退", ActionType::Retreat },

            { "SpeedUp", ActionType::SwitchSpeed },
            { "SPEEDUP", ActionType::SwitchSpeed },
            { "Speedup", ActionType::SwitchSpeed },
            { "speedup", ActionType::SwitchSpeed },
            { "二倍速", ActionType::SwitchSpeed },

            { "BulletTime", ActionType::BulletTime },
            { "BULLETTIME", ActionType::BulletTime },
            { "Bullettime", ActionType::BulletTime },
            { "bullettime", ActionType::BulletTime },
            { "子弹时间", ActionType::BulletTime },

            { "SkillUsage", ActionType::SkillUsage },
            { "SKILLUSAGE", ActionType::SkillUsage },
            { "Skillusage", ActionType::SkillUsage },
            { "skillusage", ActionType::SkillUsage },
            { "技能用法", ActionType::SkillUsage },

            { "Output", ActionType::Output },
            { "OUTPUT", ActionType::Output },
            { "output", ActionType::Output },
            { "输出", ActionType::Output },
            { "打印", ActionType::Output },

            { "SkillDaemon", ActionType::SkillDaemon },
            { "skilldaemon", ActionType::SkillDaemon },
            { "SKILLDAEMON", ActionType::SkillDaemon },
            { "Skilldaemon", ActionType::SkillDaemon },
            { "DoNothing", ActionType::SkillDaemon },
            { "摆完挂机", ActionType::SkillDaemon },
            { "开摆", ActionType::SkillDaemon },
        };

        std::string type_str = action_info.get("type", "Deploy");

        if (auto iter = ActionTypeMapping.find(type_str); iter != ActionTypeMapping.end()) {
            action.type = iter->second;
        }
        else {
            action.type = ActionType::Deploy;
        }
        action.kills = action_info.get("kills", 0);
        action.cost_changes = action_info.get("cost_changes", 0);
        action.costs = action_info.get("costs", 0);
        action.cooling = action_info.get("cooling", -1);
        action.group_name = action_info.get("name", std::string());

        action.location.x = action_info.get("location", 0, 0);
        action.location.y = action_info.get("location", 1, 0);

        static const std::unordered_map<std::string, DeployDirection> DeployDirectionMapping = {
            { "Right", DeployDirection::Right }, { "RIGHT", DeployDirection::Right },
            { "right", DeployDirection::Right }, { "右", DeployDirection::Right },

            { "Left", DeployDirection::Left },   { "LEFT", DeployDirection::Left },
            { "left", DeployDirection::Left },   { "左", DeployDirection::Left },

            { "Up", DeployDirection::Up },       { "UP", DeployDirection::Up },
            { "up", DeployDirection::Up },       { "上", DeployDirection::Up },

            { "Down", DeployDirection::Down },   { "DOWN", DeployDirection::Down },
            { "down", DeployDirection::Down },   { "下", DeployDirection::Down },

            { "None", DeployDirection::None },   { "NONE", DeployDirection::None },
            { "none", DeployDirection::None },   { "无", DeployDirection::None },
        };

        std::string direction_str = action_info.get("direction", "Right");

        if (auto iter = DeployDirectionMapping.find(direction_str); iter != DeployDirectionMapping.end()) {
            action.direction = iter->second;
        }
        else {
            action.direction = DeployDirection::Right;
        }
        action.modify_usage = static_cast<battle::SkillUsage>(action_info.get("skill_usage", 0));
        action.pre_delay = action_info.get("pre_delay", 0);
        auto post_delay_opt = action_info.find<int>("post_delay");
        // 历史遗留字段，兼容一下
        action.post_delay = post_delay_opt ? *post_delay_opt : action_info.get("rear_delay", 0);
        action.time_out = action_info.get("timeout", INT_MAX);
        action.doc = action_info.get("doc", std::string());
        action.doc_color = action_info.get("doc_color", std::string());

        m_data.actions.emplace_back(std::move(action));
    }

    return true;
}
