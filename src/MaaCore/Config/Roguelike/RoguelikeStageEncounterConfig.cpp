#include "RoguelikeStageEncounterConfig.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterConfig::parse(const json::value& json)
{
    LogTraceFunction;

    const std::string theme = json.at("theme").as_string();

    // m_event[(肉鸽主题,模式)] 默认继承 m_event["(肉鸽主题,std::nullopt)"]。
    // 额外配置的单个 stage 也可以用 mode 限定覆盖范围。
    std::pair<std::string, int> key = std::make_pair(theme, -1);
    std::vector<int> modes = json.get("mode", std::vector<int> {});
    const bool is_mode_override = !modes.empty();
    std::unordered_map<std::string, RoguelikeEvent> inherited_events;
    if (!modes.empty()) {
        inherited_events = m_events.at(key);
    }
    else {
        modes.emplace_back(-1);
    }

    struct ParsedEvent
    {
        RoguelikeEvent event;
        std::vector<int> modes;
    };
    std::vector<ParsedEvent> parsed_events;

    std::vector<std::string>& event_names = m_event_names[theme];

    for (const auto& event_json : json.at("stage").as_array()) {
        RoguelikeEvent event;
        event.name = event_json.at("name").as_string();
        std::vector<int> event_modes = modes;
        if (auto event_modes_opt = event_json.find("mode"); event_modes_opt) {
            if (!event_modes_opt->is_array()) {
                Log.error(std::format("RoguelikeEncounterConfig | mode for event {} must be an array", event.name));
                return false;
            }
            event_modes = static_cast<std::vector<int>>(event_modes_opt.value());
            if (event_modes.empty()) {
                Log.error(std::format("RoguelikeEncounterConfig | mode for event {} must not be empty", event.name));
                return false;
            }
            if (is_mode_override && std::ranges::any_of(event_modes, [&](int mode) {
                    return std::ranges::find(modes, mode) == modes.end();
                })) {
                Log.error(std::format("RoguelikeEncounterConfig | event {} mode is outside file mode", event.name));
                return false;
            }
        }
        if (is_mode_override) {
            if (auto inherited = inherited_events.find(event.name); inherited != inherited_events.end()) {
                event = inherited->second;
            }
        }
        if (std::find(event_names.begin(), event_names.end(), event.name) == event_names.end()) {
            event_names.emplace_back(event.name);
        }
        if (auto option_text_opt = event_json.find("option_text"); option_text_opt && option_text_opt->is_array()) {
            event.option_text = static_cast<std::vector<std::string>>(option_text_opt.value());
        }
        if (auto option_num_opt = event_json.find("option_num"); option_num_opt) {
            int num_options = option_num_opt->as_integer();
            if (num_options < 0) {
                Log.error(
                    std::format(
                        "RoguelikeEncounterConfig | default number of options {} for event {} is less than zero",
                        num_options,
                        event.name));
                return false;
            }
            event.option_num = static_cast<size_t>(num_options);
        }
        if (auto choose_opt = event_json.find("choose"); choose_opt) {
            int choice = choose_opt->as_integer();
            if (choice < 0) {
                Log.error(
                    std::format(
                        "RoguelikeEncounterConfig | default choice {} for event {} is less than zero",
                        choice,
                        event.name));
                return false;
            }
            event.default_choose = static_cast<size_t>(choice);
        }
        if (auto next_event_opt = event_json.find("next_event"); next_event_opt) {
            event.next_event = next_event_opt->as_string();
        }
        if (auto fallback_array_opt = event_json.find("fallback_choices");
            fallback_array_opt && fallback_array_opt->is_array()) {
            event.fallback_choices.clear();
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
                if (choice < 0) {
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
            event.choice_require.clear();
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

        if (auto dynamic_opt = event_json.find("dynamic_options"); dynamic_opt) {
            event.dynamic_options = dynamic_opt->as_boolean();
        }
        if (auto prefer_event_rule_opt = event_json.find("prefer_event_rule"); prefer_event_rule_opt) {
            event.prefer_event_rule = prefer_event_rule_opt->as_boolean();
        }
        if (auto max_steps_opt = event_json.find("max_steps"); max_steps_opt) {
            int max_steps = max_steps_opt->as_integer();
            if (max_steps <= 0) {
                Log.error(std::format("RoguelikeEncounterConfig | max_steps for event {} must be positive", event.name));
                return false;
            }
            event.max_steps = static_cast<size_t>(max_steps);
        }

        const auto parse_preferred_options = [&](const json::value& parent,
                                                  std::vector<RoguelikeEncounterOptionSelector::PreferredOption>& out) {
            auto preferred_opt = parent.find("preferred_options");
            if (!preferred_opt || !preferred_opt->is_array()) {
                return true;
            }
            out.clear();
            for (const auto& preferred_json : preferred_opt->as_array()) {
                RoguelikeEncounterOptionSelector::PreferredOption preferred;
                if (preferred_json.is_string()) {
                    preferred.texts.emplace_back(preferred_json.as_string());
                }
                else if (preferred_json.is_object()) {
                    if (auto texts_opt = preferred_json.find("texts"); texts_opt && texts_opt->is_array()) {
                        preferred.texts = static_cast<std::vector<std::string>>(texts_opt.value());
                    }
                    else if (auto text_opt = preferred_json.find("text"); text_opt && text_opt->is_string()) {
                        preferred.texts.emplace_back(text_opt->as_string());
                    }
                    int occurrence = preferred_json.get("occurrence", 1);
                    if (occurrence <= 0) {
                        Log.error(std::format("RoguelikeEncounterConfig | invalid occurrence for event {}", event.name));
                        return false;
                    }
                    preferred.occurrence = static_cast<size_t>(occurrence);
                }
                if (preferred.texts.empty()) {
                    Log.error(std::format("RoguelikeEncounterConfig | empty preferred option for event {}", event.name));
                    return false;
                }
                out.emplace_back(std::move(preferred));
            }
            return true;
        };

        if (auto safe_opt = event_json.find("safe_fallback_options"); safe_opt && safe_opt->is_array()) {
            event.option_rule.safe_fallback_options = static_cast<std::vector<std::string>>(safe_opt.value());
        }
        if (!parse_preferred_options(event_json, event.option_rule.preferred_options)) {
            return false;
        }
        if (auto variants_opt = event_json.find("branches"); variants_opt && variants_opt->is_array()) {
            event.option_variants.clear();
            for (const auto& variant_json : variants_opt->as_array()) {
                RoguelikeEncounterOptionSelector::Rule variant;
                variant.id = variant_json.get("id", "");
                if (auto option_texts_opt = variant_json.find("option_texts");
                    option_texts_opt && option_texts_opt->is_array()) {
                    variant.option_texts = static_cast<std::vector<std::string>>(option_texts_opt.value());
                }
                if (auto safe_opt = variant_json.find("safe_fallback_options"); safe_opt && safe_opt->is_array()) {
                    variant.safe_fallback_options = static_cast<std::vector<std::string>>(safe_opt.value());
                }
                if (!parse_preferred_options(variant_json, variant.preferred_options)) {
                    return false;
                }
                if (variant.option_texts.empty()) {
                    Log.error(std::format("RoguelikeEncounterConfig | branch {} has no option_texts", variant.id));
                    return false;
                }
                event.option_variants.emplace_back(std::move(variant));
            }
        }
        event.dynamic_options = event.dynamic_options || !event.option_rule.preferred_options.empty() ||
            !event.option_rule.safe_fallback_options.empty() || !event.option_variants.empty();
        parsed_events.emplace_back(ParsedEvent { std::move(event), std::move(event_modes) });
    }

    for (int mode : modes) {
        auto events = inherited_events;
        for (const auto& parsed : parsed_events) {
            if (std::ranges::find(parsed.modes, mode) != parsed.modes.end()) {
                events[parsed.event.name] = parsed.event;
            }
        }
        key = std::make_pair(theme, mode);
        m_events[key] = std::move(events);
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
