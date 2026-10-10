#include "RoguelikeShoppingConfig.h"

#include <algorithm>
#include <utility>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeShoppingConfig::parse_goods(const json::array& json, std::vector<RoguelikeGoods>& goods)
{
    static const std::unordered_map<std::string, battle::Role> RoleMap = {
        { "CASTER", battle::Role::Caster }, { "MEDIC", battle::Role::Medic },     { "PIONEER", battle::Role::Pioneer },
        { "SNIPER", battle::Role::Sniper }, { "SPECIAL", battle::Role::Special }, { "SUPPORT", battle::Role::Support },
        { "TANK", battle::Role::Tank },     { "WARRIOR", battle::Role::Warrior },
    };
    for (const auto& goods_json : json) {
        RoguelikeGoods item;
        item.name = goods_json.at("name").as_string();
        if (item.name.empty()) {
            LogError << __FUNCTION__ << "Shopping item name must not be empty";
            return false;
        }
        if (const auto roles = goods_json.find_value("roles")) {
            if (!roles->is<std::vector<std::string>>()) {
                LogError << __FUNCTION__ << "Shopping roles must be an array of strings" << item.name;
                return false;
            }
            for (const auto& role : roles->as_array()) {
                const auto found = RoleMap.find(role.as_string());
                if (found == RoleMap.end()) {
                    LogError << __FUNCTION__ << "Unknown shopping role" << role.as_string();
                    return false;
                }
                item.roles.emplace_back(found->second);
            }
        }
        if (const auto chars = goods_json.find_value("chars")) {
            if (!chars->is<std::vector<std::string>>()) {
                LogError << __FUNCTION__ << "Shopping chars must be an array of strings" << item.name;
                return false;
            }
            item.chars = chars->as<std::vector<std::string>>();
        }
        item.promotion = goods_json.get("promotion", 0);
        item.promotion_rarity = goods_json.get("promotion_rarity", 6);
        item.no_longer_buy = goods_json.get("no_longer_buy", false);
        item.ignore_no_longer_buy = goods_json.get("ignore_no_longer_buy", false);
        item.decrease_collapse = goods_json.get("decrease_collapse", false);
        if (const auto price = goods_json.find<int>("price")) {
            if (*price <= 0) {
                LogError << __FUNCTION__ << "Shopping item price must be positive" << item.name;
                return false;
            }
            item.price = *price;
        }
        goods.emplace_back(std::move(item));
    }
    return true;
}

bool asst::RoguelikeShoppingConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    const bool strategy_shopping = strategy_shopping_enabled(theme);
    if (!strategy_shopping) {
        return parse_legacy(json, theme);
    }
    Tables tables;
    // priority 保留为默认买表，其他主题无需迁移。
    if (!parse_goods(json.at("priority").as_array(), tables.buy["default"])) {
        return false;
    }
    if (const auto buy = json.find_value("buy_tables")) {
        if (!buy->is_object()) {
            LogError << __FUNCTION__ << "buy_tables must be an object" << theme;
            return false;
        }
        for (const auto& [name, table] : buy->as_object()) {
            if (name.empty() || name == "default" || !table.is_array()) {
                LogError << __FUNCTION__ << "Invalid shopping buy table" << theme << name;
                return false;
            }
            if (!parse_goods(table.as_array(), tables.buy[name])) {
                return false;
            }
        }
    }
    if (const auto sell = json.find_value("sell_tables")) {
        if (!sell->is_object()) {
            LogError << __FUNCTION__ << "sell_tables must be an object" << theme;
            return false;
        }
        for (const auto& [name, table] : sell->as_object()) {
            if (name.empty() || !table.is<std::vector<std::string>>()) {
                LogError << __FUNCTION__ << "Invalid shopping sell table" << theme << name;
                return false;
            }
            auto names = table.as<std::vector<std::string>>();
            if (std::ranges::any_of(names, [](const std::string& item) { return item.empty(); })) {
                LogError << __FUNCTION__ << "Shopping sell table contains an empty name" << theme << name;
                return false;
            }
            tables.sell.emplace(name, std::move(names));
        }
    }
    m_tables.insert_or_assign(theme, std::move(tables));
    return true;
}

bool asst::RoguelikeShoppingConfig::parse_legacy(const json::value& json, const std::string& theme)
{
    m_goods.erase(theme);

    const auto& theme_json = json.at("priority");
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
        goods.promotion_rarity = goods_json.get("promotion_rarity", 6);
        goods.no_longer_buy = goods_json.get("no_longer_buy", false);
        goods.ignore_no_longer_buy = goods_json.get("ignore_no_longer_buy", false);
        goods.decrease_collapse = goods_json.get("decrease_collapse", false);

        m_goods[theme].emplace_back(std::move(goods));
    }
    return true;
}

bool asst::RoguelikeShoppingConfig::has_tables(
    const std::string& theme,
    const std::string& buy_table,
    const std::string& sell_table) const
{
    const auto found = m_tables.find(theme);
    return found != m_tables.end() && found->second.buy.contains(buy_table) &&
           (sell_table.empty() || found->second.sell.contains(sell_table));
}

void asst::RoguelikeShoppingConfig::clear()
{
    m_goods.clear();
    m_tables.clear();
}
