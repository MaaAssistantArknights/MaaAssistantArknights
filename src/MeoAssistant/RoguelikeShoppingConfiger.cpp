#include "RoguelikeShoppingConfiger.h"

#include <meojson/json.hpp>

bool asst::RoguelikeShoppingConfiger::parse(const json::value& json)
{
    for (const auto& commodity_json : json.as_array()) {
        std::string name = commodity_json.at("name").as_string();
        static const std::unordered_map<std::string, BattleRole> RoleMap = {
            { "CASTER", BattleRole::Caster },
            { "MEDIC", BattleRole::Medic },
            { "PIONEER", BattleRole::Pioneer },
            { "SNIPER", BattleRole::Sniper},
            { "SPECIAL", BattleRole::Special},
            { "SUPPORT", BattleRole::Support},
            { "TANK", BattleRole::Tank},
            { "WARRIOR", BattleRole::Warrior},
        };
        BattleRole role = BattleRole::Unknown;
        std::string str_role = commodity_json.get("role", "");
        if (!str_role.empty()) {
            role = RoleMap.at(str_role);
        }
        m_goods.emplace_back(RoguelikeGoods{ name, role });
    }
    return true;
}
