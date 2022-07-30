#include "RoguelikeShoppingConfiger.h"

#include <meojson/json.hpp>

bool asst::RoguelikeShoppingConfiger::parse(const json::value& json)
{
    for (const auto& goods_json : json.as_array()) {
        std::string name = goods_json.at("name").as_string();
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
        std::string str_role = goods_json.get("role", "");
        if (!str_role.empty()) {
            role = RoleMap.at(str_role);
        }
        RoguelikeGoods goods;
        goods.name = std::move(name);
        goods.role_restriction = role;
        goods.promotion = goods_json.get("promotion", 0);
        goods.no_longer_buy = goods_json.get("no_longer_buy", false);
        goods.ignore_no_longer_buy = goods_json.get("ignore_no_longer_buy", false);

        m_goods.emplace_back(std::move(goods));
    }
    return true;
}
