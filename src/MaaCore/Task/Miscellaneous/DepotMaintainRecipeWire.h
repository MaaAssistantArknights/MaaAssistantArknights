#pragma once

#include "DepotMaintainRecipes.h"
#include "DepotMaintainWire.h"
#include <unordered_set>

namespace asst::depot_maintain
{
inline std::optional<Recipes>
    recipe_catalog(const json::value& formulas, const std::unordered_set<std::string>& known_items)
{
    if (!formulas.is_object() || formulas.as_object().size() > 2048) {
        return std::nullopt;
    }
    Recipes result;
    for (const auto& [id, ingredients] : formulas.as_object()) {
        if (id.empty() || !known_items.contains(id) || !ingredients.is_object() || ingredients.as_object().empty() ||
            ingredients.as_object().size() > 8) {
            return std::nullopt;
        }
        Ingredients parsed;
        for (const auto& [ingredient, value] : ingredients.as_object()) {
            const auto quantity = recognized_count(value);
            if (ingredient.empty() || !known_items.contains(ingredient) || !quantity || *quantity <= 0 ||
                *quantity > INT_MAX) {
                return std::nullopt;
            }
            parsed.emplace(ingredient, *quantity);
        }
        result.emplace(id, std::move(parsed));
    }
    return result;
}

inline json::value dependency_receipt(const DependencyPreview& preview)
{
    auto object = [](const Ingredients& values) {
        json::value data = json::object {};
        for (const auto& [id, count] : values) {
            data[id] = count;
        }
        return data;
    };
    json::array unknown;
    for (const auto& id : preview.unknown) {
        unknown.emplace_back(id);
    }
    return json::object {
        { "verified", preview.verified() },
        { "error", preview.error },
        { "unknown", std::move(unknown) },
        { "root_shortages", object(preview.deficits) },
        { "protected_stock", object(preview.protected_stock) },
        { "allocated_stock", object(preview.allocated_stock) },
        { "hypothetical_crafts", object(preview.crafts) },
        { "raw_material_shortages", object(preview.farm) },
        { "reserve_semantics", "independent_final_stock_duplicate_max" },
        { "source", "native_item_index_one_output_formulas" },
        { "craft_executed", false },
        { "purchase_executed", false },
        { "targets_met", false },
        { "currency_fees_verified", false },
        { "recipe_unlock_verified", false },
        { "farm_routes_verified", false },
    };
}
}
