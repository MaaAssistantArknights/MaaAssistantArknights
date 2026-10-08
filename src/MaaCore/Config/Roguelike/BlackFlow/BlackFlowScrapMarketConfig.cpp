#include "BlackFlowScrapMarketConfig.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>

#include "Utils/Logger.hpp"

namespace asst
{
std::optional<std::reference_wrapper<const blackflow::ScrapItem>>
    BlackFlowScrapMarketConfig::find(const std::string& name) const
{
    const auto found = m_index.find(name);
    if (found == m_index.end()) {
        return std::nullopt;
    }
    return std::cref(m_items[found->second]);
}

bool BlackFlowScrapMarketConfig::parse(const json::value& value)
{
    LogTraceFunction;

    if (value.get("schema_version", 0) != 1) {
        LogError << __FUNCTION__ << "Unsupported scrap market schema version";
        return false;
    }

    const auto refresh_costs = value.find<std::vector<int>>("refresh_costs");
    if (!refresh_costs || std::ranges::any_of(*refresh_costs, [](int cost) { return cost <= 0; })) {
        LogError << __FUNCTION__ << "refresh_costs must be positive integers";
        return false;
    }
    const int processing_keep_limit = value.get("processing_keep_limit", -1);
    if (processing_keep_limit < 0) {
        LogError << __FUNCTION__ << "processing_keep_limit must be a nonnegative integer";
        return false;
    }

    static const std::unordered_map<std::string, blackflow::ScrapCategory> Categories = {
        { "processing", blackflow::ScrapCategory::Processing },
        { "natural", blackflow::ScrapCategory::Natural },
        { "concept", blackflow::ScrapCategory::Concept },
    };

    std::vector<blackflow::ScrapItem> items;
    std::unordered_map<std::string, std::size_t> index;
    const auto scraps = value.find<json::array>("scraps");
    if (!scraps || scraps->empty()) {
        LogError << __FUNCTION__ << "scraps must be a nonempty array";
        return false;
    }
    for (const auto& entry : *scraps) {
        blackflow::ScrapItem item;
        item.name = entry.get("name", std::string());
        const auto category = Categories.find(entry.get("category", std::string()));
        if (item.name.empty() || category == Categories.end() || !index.emplace(item.name, items.size()).second) {
            LogError << __FUNCTION__ << "Invalid or duplicate scrap" << item.name;
            return false;
        }
        item.category = category->second;
        if (const auto price = entry.find<int>("price")) {
            item.price = *price;
        }
        item.value = entry.get("value", -1);
        item.growth_per_acquisition = entry.get("growth_per_acquisition", 0);
        if ((item.price && *item.price <= 0) || item.value < 0 || item.growth_per_acquisition < 0) {
            LogError << __FUNCTION__ << "Invalid scrap price or value" << item.name;
            return false;
        }
        if (const auto bundle = entry.find<json::object>("bundle")) {
            for (const auto& [name, count] : *bundle) {
                if (!count.is_number() || count.as_integer() <= 0) {
                    LogError << __FUNCTION__ << "Invalid scrap bundle" << item.name << name;
                    return false;
                }
                item.bundle.emplace_back(name, count.as_integer());
            }
        }
        if (const auto covers = entry.find<std::vector<std::string>>("covers")) {
            if (item.category != blackflow::ScrapCategory::Processing ||
                std::ranges::any_of(*covers, [](const std::string& shape) { return shape.empty(); })) {
                LogError << __FUNCTION__ << "Invalid scrap covers" << item.name;
                return false;
            }
            item.covers = std::move(*covers);
        }
        items.emplace_back(std::move(item));
    }
    // 附带的零件只能是普通条目，避免获得次数递归展开。
    for (const auto& item : items) {
        for (const auto& [name, count] : item.bundle) {
            const auto found = index.find(name);
            if (found == index.end() || !items[found->second].bundle.empty()) {
                LogError << __FUNCTION__ << "Scrap bundle references an invalid scrap" << item.name << name;
                return false;
            }
        }
    }

    static const std::array<std::string, 3> CategoryKeys = { "processing", "natural", "concept" };
    std::vector<blackflow::ScrapShopType> shop_types;
    std::unordered_set<std::string> type_ids;
    const auto types = value.find<json::array>("shop_types");
    if (!types || types->empty()) {
        LogError << __FUNCTION__ << "shop_types must be a nonempty array";
        return false;
    }
    for (const auto& entry : *types) {
        blackflow::ScrapShopType type;
        type.id = entry.get("id", std::string());
        const auto layouts = entry.find<json::array>("layouts");
        const auto rates = entry.find<json::object>("offer_rates");
        if (type.id.empty() || !type_ids.emplace(type.id).second || !layouts || layouts->empty() || !rates) {
            LogError << __FUNCTION__ << "Invalid scrap shop type" << type.id;
            return false;
        }
        for (const auto& layout_json : *layouts) {
            blackflow::ScrapShopLayout layout;
            for (std::size_t category = 0; category < CategoryKeys.size(); ++category) {
                layout.counts[category] = layout_json.get(CategoryKeys[category], -1);
            }
            layout.probability = layout_json.get("probability", 0.0);
            if (std::ranges::any_of(layout.counts, [](int count) { return count < 0; }) ||
                std::accumulate(layout.counts.begin(), layout.counts.end(), 0) != 6 || layout.probability <= 0) {
                LogError << __FUNCTION__ << "Invalid scrap shop layout" << type.id;
                return false;
            }
            type.layouts.emplace_back(layout);
        }
        for (const auto& [name, rate] : *rates) {
            const auto found = index.find(name);
            if (found == index.end() || !items[found->second].price || !rate.is_number() || rate.as_double() < 0 ||
                rate.as_double() > 1) {
                LogError << __FUNCTION__ << "Invalid scrap offer rate" << type.id << name;
                return false;
            }
            type.offer_rates.emplace(name, rate.as_double());
        }
        shop_types.emplace_back(std::move(type));
    }

    m_items = std::move(items);
    m_index = std::move(index);
    m_refresh_costs = std::move(*refresh_costs);
    m_processing_keep_limit = processing_keep_limit;
    m_shop_types = std::move(shop_types);
    return true;
}
} // namespace asst
