#pragma once
#include "AbstractConfiger.h"
#include "Utils/AsstBattleDef.h"

namespace asst
{
    class CopilotConfiger : public SingletonHolder<CopilotConfiger>, public AbstractConfiger
    {
    public:
        virtual ~CopilotConfiger() override = default;

        bool contains_actions(const std::string& stage_name) const noexcept
        {
            return m_battle_actions.contains(stage_name);
        }

        auto get_actions(const std::string& stage_name) const noexcept { return m_battle_actions.at(stage_name); }

        auto get_stage_name() { return m_stage_name; }

    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, BattleCopilotData> m_battle_actions;
        std::string m_stage_name = "";
    };

    inline static auto& Copilot = CopilotConfiger::get_instance();
}
