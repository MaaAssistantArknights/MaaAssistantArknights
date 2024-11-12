#include "ItemConfig.h"

#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::ItemConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();
    std::unordered_map<std::string, int> material_sortid;
    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name").as_string();
        m_item_name.emplace(id, std::move(name));
        m_all_item_id.emplace(id);
        if (item_json.at("classifyType").as_string() == "MATERIAL") {
            material_sortid.emplace(id, item_json.at("sortId").as_integer());
        }
    }

    m_ordered_material_item_id.clear();
    m_ordered_material_item_id.reserve(material_sortid.size());
    ranges::copy(material_sortid | views::keys, std::back_inserter(m_ordered_material_item_id));
    ranges::sort(m_ordered_material_item_id, std::less {}, [&](const std::string& name) -> int {
        return material_sortid[name];
    });

    return true;
}

void asst::ItemConfig::clear()
{
    m_item_name.clear();
    m_all_item_id.clear();
    m_ordered_material_item_id.clear();
}
