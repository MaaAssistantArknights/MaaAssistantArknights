#include "RoguelikeShoppingConfig.h"

#include <meojson/json.hpp>

bool asst::RoguelikeShoppingConfig::parse(const json::value& json)
{
    clear();

    for (const auto& theme_view : { RoguelikePhantomThemeName, RoguelikeMizukiThemeName }) {
        const std::string theme(theme_view);
        const auto& theme_json = json.at(theme);
        for (const auto& goods_json : theme_json.as_array()) {
            std::string name = goods_json.at("name").as_string();

            std::vector<battle::Role> roles;
            if (auto roles_opt = goods_json.find<json::array>("roles")) {
                for (const auto& role_json : roles_opt.value()) {
                    static const std::unordered_map<std::string, battle::Role> RoleMap = {
                        { "CASTER", battle::Role::Caster },   { "MEDIC", battle::Role::Medic },
                        { "PIONEER", battle::Role::Pioneer }, { "SNIPER", battle::Role::Sniper },
                        { "SPECIAL", battle::Role::Special }, { "SUPPORT", battle::Role::Support },
                        { "TANK", battle::Role::Tank },       { "WARRIOR", battle::Role::Warrior },
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

            m_goods[theme].emplace_back(std::move(goods));
        }
    }
    return true;
}

void asst::RoguelikeShoppingConfig::clear()
{
    m_goods.clear();
}
