#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    struct RoguelikeGoods
    {
        std::string name;
        std::vector<battle::Role> roles;
        std::vector<std::string> chars;
        int promotion = 0; // 晋升 N 个干员
        bool no_longer_buy = false;
        bool ignore_no_longer_buy = false;
    };

    class RoguelikeShoppingConfig final : public SingletonHolder<RoguelikeShoppingConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeShoppingConfig() override = default;

        const auto& get_goods(const std::string& theme) const noexcept { return m_goods.at(theme); }

    private:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_map<std::string, std::vector<RoguelikeGoods>> m_goods;
    };

    inline static auto& RoguelikeShopping = RoguelikeShoppingConfig::get_instance();
}
