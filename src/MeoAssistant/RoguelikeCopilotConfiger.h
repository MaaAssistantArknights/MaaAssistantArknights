#pragma once
#include "AbstractConfiger.h"

#include <optional>

#include "AsstBattleDef.h"

namespace asst
{
    class RoguelikeCopilotConfiger : public AbstractConfiger
    {
    public:
        virtual ~RoguelikeCopilotConfiger() override = default;

        std::optional<RoguelikeBattleData> get_stage_data(const std::string& stage_name) const;

    protected:
        virtual bool parse(const json::value& json) override;
        std::unordered_map<std::string, RoguelikeBattleData> m_stage_data;
    };
}
