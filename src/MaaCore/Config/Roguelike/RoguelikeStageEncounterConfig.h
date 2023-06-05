#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    struct RoguelikeEvent
    {
        std::string name;
        int option_num = 0;
        int default_choose = 0;
    };

    class RoguelikeStageEncounterConfig final : public SingletonHolder<RoguelikeStageEncounterConfig>,
                                                public AbstractConfig
    {
    public:
        virtual ~RoguelikeStageEncounterConfig() override = default;

        const auto& get_events(const std::string& theme) const noexcept { return m_events.at(theme); }

    private:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, std::vector<RoguelikeEvent>> m_events;
    };

    inline static auto& RoguelikeStageEncounter = RoguelikeStageEncounterConfig::get_instance();
}
