#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst
{
    enum class ComparisonType
    {
        GreaterThan,
        LessThan,
        Equal,
        Unsupported
    };

    struct Vision
    {
        std::string value;
        ComparisonType type = ComparisonType::Unsupported;
    };
    struct ChoiceRequire
    {
        std::string name;
        Vision vision;
        int choose = -1;
    };
    struct RoguelikeEvent
    {
        std::string name;
        int option_num = 0;
        int default_choose = 0;
        std::vector<ChoiceRequire> choice_require;
    };

    class RoguelikeStageEncounterConfig final : public SingletonHolder<RoguelikeStageEncounterConfig>,
                                                public AbstractConfig
    {
    public:
        virtual ~RoguelikeStageEncounterConfig() override = default;

        const auto& get_events(const std::string& theme) const noexcept { return m_events.at(theme); }

    private:
        virtual bool parse(const json::value& json) override;

        static ComparisonType parse_comparison_type(const std::string& type_str)
        {
            if (type_str == ">")
                return ComparisonType::GreaterThan;
            if (type_str == "<")
                return ComparisonType::LessThan;
            if (type_str == "=")
                return ComparisonType::Equal;

            return ComparisonType::Unsupported;
        }

        std::unordered_map<std::string, std::vector<RoguelikeEvent>> m_events;
    };

    inline static auto& RoguelikeStageEncounter = RoguelikeStageEncounterConfig::get_instance();
}
