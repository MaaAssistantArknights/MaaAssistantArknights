#include "CombatRecruitConfiger.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::CombatRecruitConfiger::parse(const json::value& json)
{
    LogTraceFunction;

    for (const auto& [name, oper_json] : json.as_object()) {
        CombatRecruitOperInfo oper_temp;
        oper_temp.name = name;
        //oper_temp.type = oper_json.get("profession", "unknown");
        static const std::unordered_map<std::string, BattleRole> RoleMapping = {
            { "MEDIC", BattleRole::Medic },
            { "WARRIOR", BattleRole::Warrior },
            { "SPECIAL", BattleRole::Special },
            { "SNIPER", BattleRole::Sniper },
            { "PIONEER", BattleRole::Pioneer },
            { "TANK", BattleRole::Tank },
            { "SUPPORT", BattleRole::Support },
            { "CASTER", BattleRole::Caster },
            { "unknown", BattleRole::Unknown }
        };
        std::string role_str = oper_json.get("profession", "unknown");
        if (auto role_iter = RoleMapping.find(role_str);
            role_iter == RoleMapping.cend()) {
            oper_temp.role = role_iter->second;
        }
        else {
            oper_temp.role = BattleRole::Unknown;
        }

        oper_temp.subProfession = oper_json.get("subProfession", "unknown");
        oper_temp.position = oper_json.get("position", "unknown");
        oper_temp.level = oper_json.get("level", 0);
        m_all_opers.emplace(name, std::move(oper_temp));
        m_all_opers_name.emplace_back(name);
    }

    // // 按干员等级排个序
    // std::sort(m_all_opers.begin(), m_all_opers.end(), [](const auto& lhs, const auto& rhs) -> bool {
    //     return lhs.level > rhs.level;
    // });

    return true;
}
