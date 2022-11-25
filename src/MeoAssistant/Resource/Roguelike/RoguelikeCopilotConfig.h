#pragma once
#include "Resource/AbstractConfig.h"

#include <optional>

#include "Common/AsstBattleDef.h"

namespace asst
{
    class RoguelikeCopilotConfig final : public SingletonHolder<RoguelikeCopilotConfig>, public AbstractConfig
    {
    public:
        virtual ~RoguelikeCopilotConfig() override = default;

        std::optional<RoguelikeBattleData> get_stage_data(const std::string& stage_name) const;

    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, RoguelikeBattleData> m_stage_data;
    };

    inline static auto& RoguelikeCopilot = RoguelikeCopilotConfig::get_instance();
}
