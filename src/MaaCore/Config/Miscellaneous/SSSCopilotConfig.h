#pragma once
#include "Common/AsstBattleDef.h"
#include "Config/AbstractConfig.h"

namespace asst
{
    class SSSCopilotConfig : public SingletonHolder<SSSCopilotConfig>, public AbstractConfig
    {
    public:
        virtual ~SSSCopilotConfig() override = default;

        const battle::sss::CompleteData& get_data() const noexcept { return m_data; }
        size_t stages_size() const noexcept { return m_data.stages_data.size(); }
        const battle::sss::CombatData& get_data(size_t index) const noexcept { return m_data.stages_data.at(index); }
        void clear();

    protected:
        virtual bool parse(const json::value& json) override;

        battle::sss::CompleteData m_data;
    };
}
