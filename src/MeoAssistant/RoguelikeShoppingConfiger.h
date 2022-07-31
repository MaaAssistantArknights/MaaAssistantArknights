#pragma once

#include "AbstractConfiger.h"

#include <vector>

#include "AsstBattleDef.h"

namespace asst
{
    struct RoguelikeGoods
    {
        std::string name;
        std::vector<BattleRole> roles;
        std::vector<std::string> chars;
        int promotion = 0;  // 晋升 N 个干员
        bool no_longer_buy = false;
        bool ignore_no_longer_buy = false;
    };

    class RoguelikeShoppingConfiger : public AbstractConfiger
    {
    public:
        virtual ~RoguelikeShoppingConfiger() override = default;

        const auto& get_goods() const noexcept
        {
            return m_goods;
        }

    private:
        virtual bool parse(const json::value& json) override;

        std::vector<RoguelikeGoods> m_goods;
    };
}
