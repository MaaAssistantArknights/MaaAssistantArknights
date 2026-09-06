#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace asst
{
struct MaterialAmount
{
    std::string item_id;
    int count = 0;
};

struct MaterialFormula
{
    std::string formula_id;
    std::string item_id;
    int count = 1;
    int gold_cost = 0;
    int ap_cost = 0;
    std::vector<MaterialAmount> costs;
};

using MaterialInventory = std::map<std::string, int>;

struct MaterialCraftRequest
{
    std::vector<MaterialAmount> targets;
    MaterialInventory inventory;
};

struct MaterialCraftOperation
{
    // Own the recipe so resource reloads cannot invalidate an executing plan.
    MaterialFormula formula;
    int batches = 0;
};

struct MaterialCraftPlan
{
    bool valid = false;
    std::string error;
    MaterialInventory inventory;
    MaterialInventory missing;
    std::vector<MaterialCraftOperation> operations;
    int64_t gold_cost = 0;
    int64_t ap_cost = 0;
};

/// Pure planning: no controller, resources, callbacks or image recognition.
class MaterialCraftPlanner
{
public:
    explicit MaterialCraftPlanner(std::vector<MaterialFormula> formulas);

    MaterialCraftPlan build(const MaterialCraftRequest& request, const std::function<bool()>& cancelled = {}) const;

private:
    bool craft(
        const std::string& item_id,
        int count,
        MaterialCraftPlan& state,
        std::unordered_set<std::string>& stack,
        const std::function<bool()>& cancelled) const;
    bool consume(
        const MaterialAmount& cost,
        MaterialCraftPlan& state,
        std::unordered_set<std::string>& stack,
        const std::function<bool()>& cancelled) const;

    std::map<std::string, std::vector<MaterialFormula>> m_formulas;
};
}
