#include "RoguelikeStageEncounterConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();

    // m_event[(肉鸽主题,模式)] 默认继承 m_event["(肉鸽主题,std::nullopt)"]
    std::pair<std::string, int> key = std::make_pair(theme, -1);
    std::vector<int> modes = json.get("mode", std::vector<int> {});
    std::unordered_map<std::string, RoguelikeEvent> events;
    if (!modes.empty()) {
        events = m_events.at(key);
    }
    else {
        modes.emplace_back(-1);
    }

    std::vector<std::string>& event_names = m_event_names[theme];

    for (const auto& event_json : json.at("stage").as_array()) {
        RoguelikeEvent event;
        event.name = event_json.at("name").as_string();
        if (std::find(event_names.begin(), event_names.end(), event.name) == event_names.end()) {
            event_names.emplace_back(event.name);
        }
        if (int num_options = event_json.get("option_num", 0); num_options >= 0) {
            event.option_num = static_cast<size_t>(num_options);
        }
        else {
            Log.error(
                std::format(
                    "RoguelikeEncounterConfig | default number of options {} for event {} is less than zero",
                    num_options,
                    event.name));
            return false;
        }
        if (int choice = event_json.get("choose", 0); choice >= 0) {
            event.default_choose = static_cast<size_t>(choice);
        }
        else {
            Log.error(
                std::format(
                    "RoguelikeEncounterConfig | default choice {} for event {} is less than zero",
                    choice,
                    event.name));
            return false;
        }
        event.next_event = event_json.get("next_event", "");
        if (auto fallback_array_opt = event_json.find("fallback_choices");
            fallback_array_opt && fallback_array_opt->is_array()) {
            for (const auto& pair_json : fallback_array_opt->as_array()) {
                if (!pair_json.is_array()) {
                    continue;
                }
                auto pair_arr = pair_json.as_array();
                if (pair_arr.size() < 2) {
                    continue;
                }

                int option_num = pair_arr[0].as_integer();
                if (option_num < 0) {
                    Log.error(
                        std::format(
                            "RoguelikeEncounterConfig | callback option_num for event {} is less than zero",
                            event.name));
                    return false;
                }
                int choice = pair_arr[1].as_integer();
                if (option_num < 0) {
                    Log.error(
                        std::format(
                            "RoguelikeEncounterConfig | callback choice for event {} with {} option(s) is less than "
                            "zero",
                            event.name,
                            option_num));
                    return false;
                }
                event.fallback_choices.emplace_back(static_cast<size_t>(option_num), static_cast<size_t>(choice));
            }
        }

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
                            choice.vision.type = parse_comparison_type(requirement_json.get("type", ""));
                        }
                        else if (name == "Relic") {
                            // not supported
                        }
                    }
                }
                event.choice_require.emplace_back(std::move(choice));
            }
        }
        events[event.name] = std::move(event);
    }

    for (int mode : modes) {
        key = std::make_pair(theme, mode);
        m_events[key] = events;
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
