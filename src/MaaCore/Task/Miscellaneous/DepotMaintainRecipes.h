#pragma once

#include "DepotMaintainPolicy.h"
#include <functional>
#include <map>
#include <set>

namespace asst::depot_maintain
{
using Ingredients = std::map<std::string, int64_t>;
// The native item-index exporter retains only one-output formulas.
using Recipes = std::map<std::string, Ingredients>;

struct DependencyPreview
{
    std::string error;
    std::set<std::string> unknown;
    Ingredients deficits;
    Ingredients protected_stock;
    Ingredients allocated_stock;
    Ingredients crafts;
    Ingredients farm;

    bool verified() const { return error.empty() && unknown.empty(); }
};

inline bool add_requirement(Ingredients& counts, const std::string& id, int64_t quantity)
{
    auto& current = counts[id];
    if (quantity < 0 || quantity > std::numeric_limits<int64_t>::max() - current) {
        return false;
    }
    current += quantity;
    return true;
}

// This is a reserve calculation, NOT a consumable operator training bill.
// Every enabled root is a final held-stock goal. Duplicate roots use MAX;
// ingredient stock below its own reserve is never spent on another root.
// A caller that wants an ingredient only for crafting must not add it as a root.
// Nothing here updates the actual Inventory or authorizes a craft/purchase.
inline DependencyPreview preview_dependencies(
    const Inventory& inventory,
    const Recipes& recipes,
    const std::vector<Goal>& goals,
    const std::set<std::string>& direct_items)
{
    DependencyPreview result;
    auto fail = [&](std::string error) {
        result.error = std::move(error);
        result.allocated_stock.clear();
        result.crafts.clear();
        result.farm.clear();
        return result;
    };
    if (goals.size() > 256) {
        return fail("goal_limit");
    }
    Ingredients reserves;
    for (const auto& goal : goals) {
        if (!goal.enabled) {
            continue;
        }
        if (goal.item.empty() || goal.target <= 0) {
            return fail("invalid_goal");
        }
        reserves[goal.item] = std::max(reserves[goal.item], goal.target);
    }
    Ingredients demand;
    for (const auto& [id, target] : reserves) {
        const auto stock = inventory.count(id);
        if (!stock) {
            result.unknown.emplace(id);
            continue;
        }
        result.protected_stock[id] = std::min(*stock, target);
        const int64_t deficit = std::max<int64_t>(0, target - *stock);
        result.deficits[id] = deficit;
        if (deficit > 0) {
            demand[id] = deficit;
        }
    }
    if (!result.unknown.empty()) {
        return fail("unknown_inventory");
    }
    // Parent-before-child order aggregates shared ingredient demand before
    // deducting its one observed stock quantity. Per-root recursion would
    // incorrectly spend the same ingredient stock twice.
    std::map<std::string, int> colors;
    std::vector<std::string> order;
    std::string graph_error;
    std::function<bool(const std::string&, size_t)> visit = [&](const std::string& id, size_t depth) {
        if (depth > 16) {
            graph_error = "recipe_depth_limit";
            return false;
        }
        if (colors[id] == 1) {
            graph_error = "recipe_cycle";
            return false;
        }
        if (colors[id] == 2) {
            return true;
        }
        if (colors.size() > 1024) {
            graph_error = "recipe_node_limit";
            return false;
        }
        colors[id] = 1;
        if (const auto recipe = recipes.find(id); recipe != recipes.end() && !direct_items.contains(id)) {
            if (recipe->second.empty() || recipe->second.size() > 8) {
                graph_error = "invalid_recipe";
                return false;
            }
            for (const auto& [ingredient, count] : recipe->second) {
                if (ingredient.empty() || count <= 0) {
                    graph_error = "invalid_recipe";
                    return false;
                }
                if (!visit(ingredient, depth + 1)) {
                    return false;
                }
            }
        }
        colors[id] = 2;
        order.push_back(id);
        return true;
    };
    for (const auto& [id, quantity] : demand) {
        (void)quantity;
        if (!visit(id, 0)) {
            return fail(graph_error);
        }
    }
    std::ranges::reverse(order);
    for (const auto& id : order) {
        const auto wanted = demand.find(id);
        if (wanted == demand.end() || wanted->second == 0) {
            continue;
        }
        const auto stock = inventory.count(id);
        if (!stock) {
            result.unknown.emplace(id);
            continue;
        }
        const auto reserve = reserves.find(id);
        const int64_t available = reserve == reserves.end() ? *stock : std::max<int64_t>(0, *stock - reserve->second);
        const int64_t allocated = std::min(available, wanted->second);
        const int64_t missing = wanted->second - allocated;
        if (allocated > 0) {
            result.allocated_stock[id] = allocated;
        }
        if (missing == 0) {
            continue;
        }
        const auto recipe = recipes.find(id);
        if (recipe == recipes.end() || direct_items.contains(id)) {
            result.farm[id] = missing;
            continue;
        }
        result.crafts[id] = missing;
        for (const auto& [ingredient, count] : recipe->second) {
            if (missing > std::numeric_limits<int64_t>::max() / count ||
                !add_requirement(demand, ingredient, missing * count)) {
                return fail("requirement_overflow");
            }
        }
    }
    if (!result.unknown.empty()) {
        return fail("unknown_inventory");
    }
    return result;
}
}
