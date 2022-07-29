#pragma once

#include "AbstractConfiger.h"

#include <vector>

#include "AsstBattleDef.h"

namespace asst
{
    struct RoguelikeGoods
    {
        std::string name;
        BattleRole restriction = BattleRole::Unknown;
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
