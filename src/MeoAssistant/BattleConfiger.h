#pragma once
#include "AbstractConfiger.h"
#include "AsstBattleDef.h"

namespace asst
{
    class BattleConfiger : public AbstractConfiger
    {
    public:
        virtual ~BattleConfiger() = default;
    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, BattleActions> m_battle_actions;
    };
}
