#pragma once
#include "AbstractConfig.h"
#include "Utils/AsstBattleDef.h"

namespace asst
{
    class CopilotConfig: public SingletonHolder<CopilotConfig>, public AbstractConfig
    {
    public:
        virtual ~CopilotConfig() override = default;

        const BattleCopilotData& get_data() const noexcept { return m_data; }
        const std::string& get_stage_name() const noexcept { return m_stage_name; }
        void clear();

    protected:
        virtual bool parse(const json::value& json) override;

        BattleCopilotData m_data;
        std::string m_stage_name;
    };

    inline static auto& Copilot = CopilotConfig::get_instance();
}
