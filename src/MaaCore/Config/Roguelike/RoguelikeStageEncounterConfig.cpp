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
        if (auto choice_require_opt = event_json.find("choices");
            choice_require_opt && choice_require_opt->is_array()) {
            for (const auto& choice_json : choice_require_opt->as_array()) {
                ChoiceRequire choice;
                choice.name = choice_json.at("name").as_string();
                choice.choose = choice_json.get("choose", -1);
                if (auto requirements_opt = choice_json.find("requirements");
                    requirements_opt && requirements_opt->is_array()) {
                    for (const auto& requirement_json : requirements_opt->as_array()) {
                        auto name = requirement_json.get("name", "");
                        if (name == "Vision") {
                            choice.vision.value = requirement_json.get("value", "");
                            choice.vision.type =
                                parse_comparison_type(requirement_json.get("type", ""));
                        }
                        else if (name == "Relic") {
                            // not supported
                        }
                    }
                }
                event.choice_require.emplace_back(std::move(choice));
            }
        }
        m_events[theme].emplace_back(std::move(event));
    }
    return true;
}

asst::RoguelikeStageEncounterConfig::ComparisonType
    asst::RoguelikeStageEncounterConfig::parse_comparison_type(const std::string& type_str)
{
    if (type_str == ">") {
        return ComparisonType::GreaterThan;
    }
    if (type_str == "<") {
        return ComparisonType::LessThan;
    }
    if (type_str == "=") {
        return ComparisonType::Equal;
    }
    return ComparisonType::Unsupported;
}
