#pragma once

#include "Config/AbstractConfig.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
struct RoguelikeGoods
{
    std::string name;
    std::vector<battle::Role> roles;
    std::vector<std::string> chars;
    int promotion = 0;        // 晋升 N 个干员
    int promotion_rarity = 6; // 晋升 N 星及以下成员
    bool no_longer_buy = false;
    bool ignore_no_longer_buy = false;
    bool decrease_collapse = false;
};

class RoguelikeShoppingConfig final : public MAA_NS::SingletonHolder<RoguelikeShoppingConfig>, public AbstractConfig
{
public:
    virtual ~RoguelikeShoppingConfig() override = default;

    [[nodiscard]] static bool strategy_shopping_enabled(const std::string& theme) noexcept
    {
        return theme == "BlackFlow";
    }

    const auto& get_goods(const std::string& theme) const noexcept { return m_goods.at(theme); }

    const std::vector<RoguelikeGoods>& get_goods(const std::string& theme, const std::string& table) const
    {
        if (!strategy_shopping_enabled(theme)) {
            return get_goods(theme);
        }
        return m_tables.at(theme).buy.at(table);
    }

    const std::vector<std::string>& get_sell_goods(const std::string& theme, const std::string& table) const
    {
        return m_tables.at(theme).sell.at(table);
    }

    [[nodiscard]] bool
        has_tables(const std::string& theme, const std::string& buy_table, const std::string& sell_table) const;

private:
    struct Tables
    {
        std::unordered_map<std::string, std::vector<RoguelikeGoods>> buy;
        std::unordered_map<std::string, std::vector<std::string>> sell;
    };

    virtual bool parse(const json::value& json) override;
    bool parse_legacy(const json::value& json, const std::string& theme);
    static bool parse_goods(const json::array& json, std::vector<RoguelikeGoods>& goods);

    void clear();

    std::unordered_map<std::string, std::vector<RoguelikeGoods>> m_goods;
    std::unordered_map<std::string, Tables> m_tables;
};

inline static auto& RoguelikeShopping = RoguelikeShoppingConfig::get_instance();
}
