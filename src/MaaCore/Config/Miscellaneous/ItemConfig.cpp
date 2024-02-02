#include "ItemConfig.h"

#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

static asst::ItemClassifyType string_to_enum(std::string type_str)
{
    static const std::unordered_map<std::string, asst::ItemClassifyType> map = {
        { "NONE", asst::ItemClassifyType::None },
        { "MATERIAL", asst::ItemClassifyType::Material },
        { "NORMAL", asst::ItemClassifyType::Normal },
        { "CONSUME", asst::ItemClassifyType::Consume }
    };
    if (auto it = map.find(type_str); it != map.end()) {
        return it->second;
    }
    return asst::ItemClassifyType::Invalid;
}

bool asst::ItemConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();

    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name").as_string();
        m_item_name.emplace(id, std::move(name));
        m_all_item_id.emplace(id);

        const auto item_classify_type = string_to_enum(item_json.at("classifyType").as_string());
        m_all_item_id_with_sortId_filtered_by_classify_type[item_classify_type].emplace(
            id, item_json.at("sortId").as_integer());
    }

    return true;
}

void asst::ItemConfig::clear()
{
    m_item_name.clear();
    m_all_item_id.clear();
    m_all_item_id_with_sortId_filtered_by_classify_type = {
        { None, {} }, { Normal, {} }, { Material, {} }, { Consume, {} }
    };
}

const std::vector<std::string> asst::ItemConfig::get_ordered_item_id_filtered_by_classify_type(const std::vector<ItemClassifyType>& types) noexcept
{
    std::vector<std::string> result;

    std::unordered_map<std::string, int> combined_map;
    for (const auto& type : types) {
        auto& map = m_all_item_id_with_sortId_filtered_by_classify_type[type];
        combined_map.insert(map.begin(), map.end());
    }

    result.reserve(combined_map.size());
    asst::ranges::copy(combined_map | views::keys, std::back_inserter(result));
    asst::ranges::sort(result, std::less {}, [&](const std::string& name) -> int { return combined_map.at(name); });

    return result;
}
