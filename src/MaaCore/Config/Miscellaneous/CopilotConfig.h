#pragma once
#include "Common/AsstBattleDef.h"
#include "Config/AbstractConfig.h"

namespace asst
{
    class CopilotConfig : public SingletonHolder<CopilotConfig>, public AbstractConfig
    {
    public:
        virtual ~CopilotConfig() override = default;

        const battle::copilot::CombatData& get_data() const noexcept { return m_data; }
        const std::string& get_stage_name() const noexcept { return m_stage_name; }
        void clear();

    protected:
        virtual bool parse(const json::value& json) override;

        battle::copilot::CombatData m_data;
        std::string m_stage_name;
    };

    inline static auto& Copilot = CopilotConfig::get_instance();
}
