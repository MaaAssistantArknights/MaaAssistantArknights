#include "ItemConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::ItemConfiger::parse(const json::value& json)
{
    LogTraceFunction;

    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name_i18n").at("zh").as_string();
        m_item_name.emplace(id, std::move(name));
    }
    return true;
}
