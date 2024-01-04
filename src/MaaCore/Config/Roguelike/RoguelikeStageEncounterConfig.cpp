#include "RoguelikeStageEncounterConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();
    m_events.erase(theme);

    for (const auto& event_json : json.at("stage").as_array()) {
        RoguelikeEvent event;
        event.name = event_json.at("name").as_string();
        event.option_num = event_json.get("option_num", 0);
        event.default_choose = event_json.get("choose", 0);
        if (event_json.contains("choice_require")) {
            for (const auto& requirement_json : event_json.at("choice_require").as_array()) {
                ChoiceRequire requirement;
                requirement.name = requirement_json.at("name").as_string();
                requirement.choose = requirement_json.get("choose", -1);
                requirement.chaos_level.value = requirement_json.at("ChaosLevel").get("value", 0);
                requirement.chaos_level.type = requirement_json.at("ChaosLevel").at("type").as_string();
                event.choice_require.emplace_back(std::move(requirement));
            }
        }
        m_events[theme].emplace_back(std::move(event));
    }
    return true;
}
