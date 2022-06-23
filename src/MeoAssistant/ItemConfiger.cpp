#include "ItemConfiger.h"

#include <meojson/json.hpp>

#include "Logger.hpp"

bool asst::ItemConfiger::parse(const json::value& json)
{
    LogTraceFunction;

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
    for (const auto& [item_id, _] : material_sortid) {
        m_ordered_material_item_id.emplace_back(item_id);
    }
    std::sort(m_ordered_material_item_id.begin(), m_ordered_material_item_id.end(),
        [&](const std::string& lhs, const std::string& rhs) -> bool {
            return material_sortid[lhs] < material_sortid[rhs];
        });

    return true;
}
