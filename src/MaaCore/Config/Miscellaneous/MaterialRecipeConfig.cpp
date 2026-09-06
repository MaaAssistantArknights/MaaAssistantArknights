#include "MaterialRecipeConfig.h"

#include <charconv>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>

namespace asst
{
namespace
{
int quantity(const json::value& value)
{
    int64_t count = 0;
    if (value.is_string()) {
        const auto& text = value.as_string();
        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), count);
        if (ec != std::errc() || end != text.data() + text.size()) {
            throw std::invalid_argument("Invalid material quantity");
        }
    }
    else {
        count = value.as_integer();
    }
    if (count < 0 || count > std::numeric_limits<int>::max()) {
        throw std::out_of_range("Material quantity is out of range");
    }
    return static_cast<int>(count);
}
}

bool MaterialRecipeConfig::parse(const json::value& json)
{
    std::unique_lock lock(m_mutex);
    auto formulas = m_formulas;
    for (const auto& [id, entry] : json.as_object()) {
        MaterialFormula formula;
        formula.formula_id = entry.get("formulaId", id);
        formula.item_id = entry.at("itemId").as_string();
        formula.count = quantity(entry.at("count"));
        formula.gold_cost = quantity(entry.at("goldCost"));
        formula.ap_cost = quantity(entry.at("apCost"));
        for (const auto& cost : entry.at("costs").as_array()) {
            formula.costs.push_back({ cost.at("id").as_string(), quantity(cost.at("count")) });
        }
        // Validate before publishing the replacement (including custom resource overlays).
        MaterialCraftPlanner validator({ formula });
        formulas.insert_or_assign(formula.formula_id, std::move(formula));
    }
    if (formulas.empty()) {
        return false;
    }
    m_formulas = std::move(formulas);
    return true;
}

std::vector<MaterialFormula> MaterialRecipeConfig::formulas() const
{
    std::shared_lock lock(m_mutex);
    std::vector<MaterialFormula> result;
    result.reserve(m_formulas.size());
    for (const auto& [_, formula] : m_formulas) {
        result.push_back(formula);
    }
    return result;
}

std::vector<std::string> MaterialRecipeConfig::item_ids() const
{
    std::shared_lock lock(m_mutex);
    std::set<std::string> ids;
    for (const auto& [_, formula] : m_formulas) {
        ids.insert(formula.item_id);
    }
    return { ids.begin(), ids.end() };
}

MaterialCraftRequest parse_material_craft_request(const json::value& params)
{
    MaterialCraftRequest request;
    auto append = [&](const json::value& target) {
        auto id = target.get("itemId", target.get("id", std::string()));
        int count = quantity(target.at("count"));
        if (id.empty() || count == 0) {
            throw std::invalid_argument("Invalid material craft target");
        }
        request.targets.push_back({ std::move(id), count });
    };
    if (auto items = params.find<json::array>("items")) {
        for (const auto& target : *items) {
            append(target);
        }
    }
    else if (auto item_map = params.find<json::object>("items")) {
        for (const auto& [id, count] : *item_map) {
            append(json::object { { "itemId", id }, { "count", count } });
        }
    }
    else if (auto targets = params.find<json::array>("targets")) {
        for (const auto& target : *targets) {
            append(target);
        }
    }
    else {
        append(params);
    }
    auto inventory = params.find<json::object>("inventory");
    if (!inventory) {
        inventory = params.find<json::object>("depot");
    }
    if (!inventory) {
        throw std::invalid_argument("Inventory is required");
    }
    for (const auto& [id, count] : *inventory) {
        if (id.empty()) {
            throw std::invalid_argument("Empty inventory item ID");
        }
        request.inventory[id] = quantity(count);
    }
    return request;
}

json::value material_craft_plan_json(const MaterialCraftPlan& plan)
{
    json::array operations, missing;
    json::object inventory;
    for (const auto& op : plan.operations) {
        operations.emplace_back(
            json::object {
                { "formula_id", op.formula.formula_id },
                { "item_id", op.formula.item_id },
                { "batches", op.batches },
                { "count", static_cast<int64_t>(op.formula.count) * op.batches },
            });
    }
    for (const auto& [id, count] : plan.missing) {
        missing.emplace_back(json::object { { "item_id", id }, { "count", count } });
    }
    for (const auto& [id, count] : plan.inventory) {
        inventory[id] = count;
    }
    return json::object {
        { "valid", plan.valid },
        { "error", plan.error },
        { "operations", std::move(operations) },
        { "missing", std::move(missing) },
        { "inventory", std::move(inventory) },
        { "gold_cost", plan.gold_cost },
        { "ap_cost", plan.ap_cost },
        { "mood_cost", static_cast<double>(plan.ap_cost) / 360000 },
    };
}
}
