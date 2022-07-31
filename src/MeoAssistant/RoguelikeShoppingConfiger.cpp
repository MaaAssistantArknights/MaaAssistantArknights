#include "RoguelikeShoppingConfiger.h"

#include <meojson/json.hpp>

bool asst::RoguelikeShoppingConfiger::parse(const json::value& json)
{
    for (const auto& goods_json : json.as_array()) {
        std::string name = goods_json.at("name").as_string();

        std::vector<BattleRole> roles;
        if (auto roles_opt = goods_json.find<json::array>("roles")) {
            for (const auto& role_json : roles_opt.value()) {
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
                roles.emplace_back(RoleMap.at(role_json.as_string()));
            }
        }
        std::vector<std::string> chars;
        if (auto chars_opt = goods_json.find<json::array>("chars")) {
            for (const auto& char_json : chars_opt.value()) {
                chars.emplace_back(char_json.as_string());
            }
        }

        RoguelikeGoods goods;
        goods.name = std::move(name);
        goods.roles = std::move(roles);
        goods.chars = std::move(chars);
        goods.promotion = goods_json.get("promotion", 0);
        goods.no_longer_buy = goods_json.get("no_longer_buy", false);
        goods.ignore_no_longer_buy = goods_json.get("ignore_no_longer_buy", false);

        m_goods.emplace_back(std::move(goods));
    }
    return true;
}
