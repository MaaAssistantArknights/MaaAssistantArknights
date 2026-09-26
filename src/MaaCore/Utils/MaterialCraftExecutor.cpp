#include "MaterialCraftExecutor.h"

#include <algorithm>
#include <limits>

using namespace asst;

MaterialCraftExecutor::MaterialCraftExecutor(std::vector<MaterialFormula> formulas, Actions actions) :
    m_formulas(std::move(formulas)),
    m_actions(std::move(actions))
{
}

bool MaterialCraftExecutor::fail(std::string error)
{
    m_error = std::move(error);
    return false;
}

bool MaterialCraftExecutor::cancelled() const
{
    return m_actions.cancelled && m_actions.cancelled();
}

bool MaterialCraftExecutor::craft(const MaterialAmount& target)
{
    m_error.clear();
    std::unordered_set<std::string> stack;
    return craft(target, stack);
}

bool MaterialCraftExecutor::craft(const MaterialAmount& target, std::unordered_set<std::string>& stack)
{
    if (cancelled()) {
        return fail("Cancelled");
    }
    const auto formula = std::ranges::find(m_formulas, target.item_id, &MaterialFormula::item_id);
    if (target.count <= 0 || formula == m_formulas.end() || formula->count <= 0 || formula->costs.empty()) {
        return fail("No recipe for " + target.item_id);
    }
    if (!stack.insert(target.item_id).second) {
        return fail("Cyclic recipe for " + target.item_id);
    }
    int remaining = static_cast<int>((static_cast<int64_t>(target.count) + formula->count - 1) / formula->count);
    std::optional<MaterialAmount> produced;
    while (remaining > 0) {
        if (cancelled()) {
            return fail("Cancelled");
        }
        if (!formula->is_manufacturing()) {
            const auto inventory = m_actions.read(*formula);
            if (cancelled() || !inventory) {
                return fail("Recipe quantities unavailable for " + target.item_id);
            }
            if (produced &&
                (!inventory->contains(produced->item_id) || inventory->at(produced->item_id) <= produced->count)) {
                return fail("Ingredient production unconfirmed for " + produced->item_id);
            }
            produced.reset();
            std::optional<MaterialAmount> missing;
            for (const auto& cost : formula->costs) {
                const int64_t required = static_cast<int64_t>(cost.count) * remaining;
                const auto owned = inventory->find(cost.item_id);
                if (cost.count <= 0 || required > std::numeric_limits<int>::max() || owned == inventory->end() ||
                    owned->second < 0) {
                    return fail("Invalid recipe quantities for " + target.item_id);
                }
                if (required > owned->second) {
                    // Detect unavailable base materials before spending anything on other children.
                    if (std::ranges::none_of(m_formulas, [&](const auto& f) { return f.item_id == cost.item_id; })) {
                        return fail(
                            "Missing material " + cost.item_id + ": " + std::to_string(required - owned->second));
                    }
                    if (!missing) {
                        missing = MaterialAmount { cost.item_id, static_cast<int>(required - owned->second) };
                    }
                }
            }
            if (missing) {
                produced = MaterialAmount { missing->item_id, inventory->at(missing->item_id) };
                if (!craft(*missing, stack) || cancelled()) {
                    return false;
                }
                // Reopen the parent after each child; another ingredient or useful byproduct may have changed.
                continue;
            }
        }
        if (cancelled()) {
            return fail("Cancelled");
        }
        const auto completed = m_actions.execute({ *formula, remaining });
        if (!completed || *completed <= 0 || *completed > remaining) {
            return fail("Craft failed for " + target.item_id);
        }
        remaining -= *completed;
    }
    stack.erase(target.item_id);
    return true;
}
