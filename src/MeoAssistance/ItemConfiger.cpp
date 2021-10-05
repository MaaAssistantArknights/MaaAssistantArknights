#include "ItemConfiger.h"

#include <json.h>

void asst::ItemConfiger::clear_drop_count()
{
    m_drop_count.clear();
}

bool asst::ItemConfiger::parse(const json::value& json)
{
    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name_i18n").at("zh").as_string();
        m_item_name.emplace(id, std::move(name));
    }
    return true;
}