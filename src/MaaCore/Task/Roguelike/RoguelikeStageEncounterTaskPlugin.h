#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"

namespace asst
{
    class RoguelikeStageEncounterTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        static bool satisfies_condition(const asst::ChoiceRequire& requirement, const int special_val)
        {
            if (requirement.chaos_level.type == ">") {
                return special_val > std::stoi(requirement.chaos_level.value);
            }
            if (requirement.chaos_level.type == "<") {
                return special_val < std::stoi(requirement.chaos_level.value);
            }
            if (requirement.chaos_level.type == "=") {
                return special_val == std::stoi(requirement.chaos_level.value);
            }
            return false;
        }

        static int process_task(const asst::RoguelikeEvent& event, const int special_val)
        {
            for (const auto& requirement : event.choice_require) {
                if (satisfies_condition(requirement, special_val)) {
                    return requirement.choose == -1 ? event.default_choose : requirement.choose;
                }
            }
            return event.default_choose;
        }
    };
}
