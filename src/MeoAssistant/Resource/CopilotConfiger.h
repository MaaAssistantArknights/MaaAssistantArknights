#pragma once
#include "AbstractConfiger.h"
#include "Utils/AsstBattleDef.h"

namespace asst
{
    class CopilotConfiger : public SingletonHolder<CopilotConfiger>, public AbstractConfiger
    {
    public:
        virtual ~CopilotConfiger() override = default;

        const BattleCopilotData& get_data() const noexcept { return m_data; }
        const std::string& get_stage_name() const noexcept { return m_stage_name; }
        void clear();

    protected:
        virtual bool parse(const json::value& json) override;

        BattleCopilotData m_data;
        std::string m_stage_name;
    };

    inline static auto& Copilot = CopilotConfiger::get_instance();
}
