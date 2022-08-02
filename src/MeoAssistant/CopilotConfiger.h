#pragma once
#include "AbstractConfiger.h"
#include "AsstBattleDef.h"

namespace asst
{
    class CopilotConfiger : public AbstractConfiger
    {
    public:
        virtual ~CopilotConfiger() override = default;

        bool contains_actions(const std::string& stage_name) const noexcept
        {
            return m_battle_actions.contains(stage_name);
        }

        auto get_actions(const std::string& stage_name) const noexcept
        {
            return m_battle_actions.at(stage_name);
        }
    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, BattleCopilotData> m_battle_actions;
    };
}
