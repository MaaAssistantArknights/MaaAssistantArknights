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
                if (auto vision_opt = requirement_json.find("Vision")) {
                    const auto& vision_json = vision_opt.value();
                    requirement.vision.value = vision_json.at("value").as_string();
                    requirement.vision.type = parse_comparison_type(vision_json.at("type").as_string());
                }
                else
                    continue;
                event.choice_require.emplace_back(std::move(requirement));
            }
        }
        m_events[theme].emplace_back(std::move(event));
    }
    return true;
}
