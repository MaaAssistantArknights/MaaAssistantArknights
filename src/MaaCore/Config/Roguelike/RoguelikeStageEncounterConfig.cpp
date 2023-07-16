#include "RoguelikeStageEncounterConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterConfig::parse(const json::value& json)
{
    LogTraceFunction;

    m_events.clear();

    for (const auto& theme_view : { RoguelikePhantomThemeName, RoguelikeMizukiThemeName, RoguelikeSamiThemeName }) {
        const std::string theme(theme_view);
        const auto& theme_json = json.at(theme);
        for (const auto& event_json : theme_json.as_object()) {
            RoguelikeEvent event;
            event.name = event_json.first;
            json::value event_info = event_json.second;
            event.option_num = event_info.get("option_num", 0);
            event.default_choose = event_info.get("choose", 0);
            m_events[theme].emplace_back(std::move(event));
        }
    }
    return true;
}
