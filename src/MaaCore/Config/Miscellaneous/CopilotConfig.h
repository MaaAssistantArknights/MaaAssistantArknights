#pragma once
#include "Common/AsstBattleDef.h"
#include "Config/AbstractConfig.h"

namespace asst
{
    class CopilotConfig : public SingletonHolder<CopilotConfig>, public AbstractConfig
    {
    public:
        static battle::copilot::BasicInfo parse_basic_info(const json::value& json);
        static battle::copilot::OperUsageGroups parse_groups(const json::value& json);
        static std::vector<battle::copilot::Action> parse_actions(const json::value& json);
        static battle::RoleCounts parse_role_counts(const json::value& json);
        static battle::DeployDirection string_to_direction(const std::string& str);

    public:
        virtual ~CopilotConfig() override = default;

        const battle::copilot::CombatData& get_data() const noexcept { return m_data; }
        const std::string& get_stage_name() const noexcept { return m_data.info.stage_name; }
        void clear();

    protected:
        virtual bool parse(const json::value& json) override;

        battle::copilot::CombatData m_data;
    };

    inline static auto& Copilot = CopilotConfig::get_instance();
}
