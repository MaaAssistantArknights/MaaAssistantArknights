#include "RoguelikeStageEncounterConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterConfig::parse(const json::value& json)
{
#ifdef ASST_DEBUG
    LogTraceFunction;
#endif

    const std::string theme = json.at("theme").as_string();
    m_events.erase(theme);

    for (const auto& event_json : json.at("stage").as_array()) {
        RoguelikeEvent event;
        event.name = event_json.at("name").as_string();
        event.option_num = event_json.get("option_num", 0);
        event.default_choose = event_json.get("choose", 0);
        m_events[theme].emplace_back(std::move(event));
    }
    return true;
}
