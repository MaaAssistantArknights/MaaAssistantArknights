#include "MaterialCraftPlanner.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace asst
{
namespace
{
int checked_count(int64_t value)
{
    if (value < 0 || value > std::numeric_limits<int>::max()) {
        throw std::overflow_error("Material quantity is out of range");
    }
    return static_cast<int>(value);
}

void add_cost(int64_t& total, int cost, int batches)
{
    const int64_t delta = static_cast<int64_t>(cost) * batches;
    if (delta < 0 || total > std::numeric_limits<int64_t>::max() - delta) {
        throw std::overflow_error("Material craft cost is out of range");
    }
    total += delta;
}

int64_t missing_total(const MaterialCraftPlan& plan)
{
    return std::accumulate(plan.missing.begin(), plan.missing.end(), int64_t { 0 }, [](auto sum, const auto& item) {
        return sum + item.second;
    });
}

int formula_order(const std::string& id)
{
    int value = 0;
    const auto [end, ec] = std::from_chars(id.data(), id.data() + id.size(), value);
    return ec == std::errc() && end == id.data() + id.size() ? value : std::numeric_limits<int>::max();
}
}

MaterialCraftPlanner::MaterialCraftPlanner(std::vector<MaterialFormula> formulas)
{
    for (auto& formula : formulas) {
        if (formula.item_id.empty() || formula.count <= 0 || formula.costs.empty() || formula.gold_cost < 0 ||
            formula.ap_cost < 0 ||
            std::ranges::any_of(
                formula.costs,
                [](const auto& cost) { return cost.item_id.empty() || cost.count <= 0; })) {
            throw std::invalid_argument("Invalid material recipe");
        }
        m_formulas[formula.item_id].emplace_back(std::move(formula));
    }
    for (auto& [_, alternatives] : m_formulas) {
        std::ranges::sort(alternatives, [](const auto& lhs, const auto& rhs) {
            const auto left = formula_order(lhs.formula_id), right = formula_order(rhs.formula_id);
            return left == right ? lhs.formula_id < rhs.formula_id : left < right;
        });
    }
}

MaterialCraftPlan
    MaterialCraftPlanner::build(const MaterialCraftRequest& request, const std::function<bool()>& cancelled) const
{
    MaterialCraftPlan plan;
    plan.inventory = request.inventory;
    try {
        if (request.targets.empty() || std::ranges::any_of(request.inventory, [](const auto& item) {
                return item.first.empty() || item.second < 0;
            })) {
            throw std::invalid_argument("Invalid material craft request");
        }
        std::unordered_set<std::string> stack;
        for (const auto& target : request.targets) {
            if (target.item_id.empty() || target.count <= 0) {
                throw std::invalid_argument("Invalid material craft target");
            }
            if (!craft(target.item_id, target.count, plan, stack, cancelled)) {
                plan.error = cancelled && cancelled() ? "Cancelled" : "No acyclic recipe for " + target.item_id;
                return plan;
            }
        }
        if (cancelled && cancelled()) {
            plan.error = "Cancelled";
            return plan;
        }
        if (plan.inventory.contains("4001")) {
            // Costs are estimated without operator bonuses; keep missing currency explicit.
            const auto available = plan.inventory.at("4001");
            if (plan.gold_cost > available) {
                plan.missing["4001"] = checked_count(plan.gold_cost - available);
            }
            plan.inventory["4001"] = static_cast<int>(std::max(int64_t { 0 }, available - plan.gold_cost));
        }
        plan.valid = true;
    }
    catch (const std::exception& e) {
        plan.error = e.what();
    }
    return plan;
}

bool MaterialCraftPlanner::craft(
    const std::string& item_id,
    int count,
    MaterialCraftPlan& state,
    std::unordered_set<std::string>& stack,
    const std::function<bool()>& cancelled) const
{
    if ((cancelled && cancelled()) || stack.contains(item_id) || !m_formulas.contains(item_id)) {
        return false;
    }
    stack.emplace(item_id);
    MaterialCraftPlan best;
    bool found = false;
    for (const auto& formula : m_formulas.at(item_id)) {
        auto candidate = state;
        const int batches = checked_count((static_cast<int64_t>(count) + formula.count - 1) / formula.count);
        bool valid = true;
        for (const auto& cost : formula.costs) {
            const MaterialAmount required { cost.item_id, checked_count(static_cast<int64_t>(cost.count) * batches) };
            if (!consume(required, candidate, stack, cancelled)) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        candidate.inventory[item_id] = checked_count(
            static_cast<int64_t>(candidate.inventory[item_id]) + static_cast<int64_t>(formula.count) * batches);
        add_cost(candidate.gold_cost, formula.gold_cost, batches);
        add_cost(candidate.ap_cost, formula.ap_cost, batches);
        if (!candidate.operations.empty() && candidate.operations.back().formula.formula_id == formula.formula_id) {
            candidate.operations.back().batches =
                checked_count(static_cast<int64_t>(candidate.operations.back().batches) + batches);
        }
        else {
            candidate.operations.push_back({ formula, batches });
        }
        if (!found || missing_total(candidate) < missing_total(best)) {
            best = std::move(candidate);
            found = true;
        }
        if (missing_total(best) == missing_total(state)) {
            break;
        }
    }
    stack.erase(item_id);
    if (found) {
        state = std::move(best);
    }
    return found;
}

bool MaterialCraftPlanner::consume(
    const MaterialAmount& cost,
    MaterialCraftPlan& state,
    std::unordered_set<std::string>& stack,
    const std::function<bool()>& cancelled) const
{
    if (cancelled && cancelled()) {
        return false;
    }
    const int used = std::min(state.inventory[cost.item_id], cost.count);
    state.inventory[cost.item_id] -= used;
    const int shortage = cost.count - used;
    if (shortage == 0) {
        return true;
    }
    if (!m_formulas.contains(cost.item_id)) {
        state.missing[cost.item_id] = checked_count(static_cast<int64_t>(state.missing[cost.item_id]) + shortage);
        return true;
    }
    if (!craft(cost.item_id, shortage, state, stack, cancelled)) {
        return false;
    }
    state.inventory[cost.item_id] -= std::min(state.inventory[cost.item_id], shortage);
    return true;
}
}
