#pragma once

#include "AbstractConfiger.h"

#include <vector>

#include "Utils/AsstBattleDef.h"

namespace asst
{
    struct RoguelikeGoods
    {
        std::string name;
        std::vector<BattleRole> roles;
        std::vector<std::string> chars;
        int promotion = 0; // 晋升 N 个干员
        bool no_longer_buy = false;
        bool ignore_no_longer_buy = false;
    };

    class RoguelikeShoppingConfiger final : public SingletonHolder<RoguelikeShoppingConfiger>, public AbstractConfiger
    {
    public:
        virtual ~RoguelikeShoppingConfiger() override = default;

        const auto& get_goods(const std::string& theme) const noexcept {return m_goods.at(theme); }

    private:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_map<std::string, std::vector<RoguelikeGoods>> m_goods;
    };

    inline static auto& RoguelikeShopping = RoguelikeShoppingConfiger::get_instance();
}
