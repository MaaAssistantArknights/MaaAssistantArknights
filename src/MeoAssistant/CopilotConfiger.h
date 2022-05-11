#pragma once
#include "AbstractConfiger.h"
#include "AsstBattleDef.h"

namespace asst
{
    class CopilotConfiger : public AbstractConfiger
    {
    public:
        virtual ~CopilotConfiger() = default;

        bool contains_actions(const std::string& stage_name) const noexcept
        {
            return m_battle_actions.find(stage_name) != m_battle_actions.cend();
        }

        auto get_actions(const std::string& stage_name) const noexcept
        {
            return m_battle_actions.at(stage_name);
        }
    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, BattleActionsGroup> m_battle_actions;
    };
}
