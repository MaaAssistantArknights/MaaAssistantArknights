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

    bool contains(const std::string& name) const noexcept { return m_data.stages_data.contains(name); }

    const battle::sss::CombatData& get_data(const std::string& name) const noexcept
    {
        return m_data.stages_data.at(name);
    }

    void clear();

protected:
    virtual bool parse(const json::value& json) override;

    battle::sss::CompleteData m_data;
};

inline static auto& SSSCopilot = SSSCopilotConfig::get_instance();
}
