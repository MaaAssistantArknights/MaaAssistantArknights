#include "RoguelikeRecruitConfiger.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "Logger.hpp"

const asst::RoguelikeOperInfo& asst::RoguelikeRecruitConfiger::get_oper_info(const std::string& name) const noexcept

{
    if (auto find_iter = m_all_opers.find(name);
        find_iter != m_all_opers.cend()) {
        return find_iter->second;
    }
    else {
        static RoguelikeOperInfo empty;
        return empty;
    }
}

bool asst::RoguelikeRecruitConfiger::parse(const json::value& json)
{
    LogTraceFunction;

    for (const auto& role_name : json.at("roles").as_array()) {
        std::string str_role = role_name.as_string();
        for (const auto& oper_info : json.at(str_role).as_array()) {
            std::string name = oper_info.at("name").as_string();
            RoguelikeOperInfo info;
            info.name = name;
            info.skill = oper_info.at("skill").as_integer();
            info.alternate_skill = oper_info.get("alternate_skill", 0);
            info.skill_usage = static_cast<BattleSkillUsage>(oper_info.get("skill_usage", 1));
            info.alternate_skill_usage = static_cast<BattleSkillUsage>(oper_info.get("alternate_skill_usage", 1));

            m_all_opers.emplace(name, std::move(info));
            m_ordered_all_opers_name.emplace_back(name);
        }
    }

    return true;
}
