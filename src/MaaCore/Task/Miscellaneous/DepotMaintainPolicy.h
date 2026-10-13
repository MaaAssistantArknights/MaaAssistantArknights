#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace asst::depot_maintain
{
// No cache is accepted by this policy. The caller must first finish a new,
// successful DepotTask and pass only its explicitly recognized quantities.
class Inventory
{
public:
    using Counts = std::unordered_map<std::string, int64_t>;

    bool replace_fresh(Counts counts, bool recognition_succeeded, bool done)
    {
        m_counts.clear();
        m_fresh = false;
        if (!recognition_succeeded || !done) {
            return false;
        }
        for (const auto& [id, count] : counts) {
            if (id.empty() || count < 0) {
                return false;
            }
        }
        m_counts = std::move(counts);
        m_fresh = true;
        return true;
    }

    std::optional<int64_t> count(const std::string& id) const
    {
        if (!m_fresh) {
            return std::nullopt;
        }
        auto found = m_counts.find(id);
        return found == m_counts.end() ? std::nullopt : std::optional(found->second);
    }

    // Only verified stock is exposed to callback consumers. Invalidated stock
    // must not survive in a GUI snapshot after an overflow or failed refresh.
    const Counts& exact_counts() const
    {
        static const Counts empty;
        return m_fresh ? m_counts : empty;
    }

    std::optional<int64_t> shortage(const std::string& id, int64_t target) const
    {
        auto current = count(id);
        if (!current || target <= 0) {
            return std::nullopt;
        }
        return std::max<int64_t>(0, target - *current);
    }

    bool add_actual_drop(const std::string& id, int64_t quantity)
    {
        if (!m_fresh || id.empty() || quantity < 0) {
            return false;
        }
        auto found = m_counts.find(id);
        // A new drop cannot establish the prior quantity of an unseen item.
        if (found == m_counts.end()) {
            return true;
        }
        if (quantity > std::numeric_limits<int64_t>::max() - found->second) {
            m_fresh = false;
            return false;
        }
        found->second += quantity;
        return true;
    }

private:
    bool m_fresh = false;
    Counts m_counts;
};

struct Goal
{
    std::string item;
    int64_t target;
    bool enabled;
    bool stage_open;
};

enum class Completion
{
    AllMet,
    NaturalBudgetIncomplete,
    Unresolved,
    UnverifiedStop,
};

// A later stage may also drop an earlier goal's material. Assess the final
// observed inventory, not sticky flags from when that goal was first visited.
// Unknown quantities never authorize fallback; stage availability matters only
// while a known goal still has a deficit.
inline Completion completion(const Inventory& inventory, const std::vector<Goal>& goals, bool natural_limit_seen)
{
    bool remaining = false;
    for (const auto& goal : goals) {
        if (!goal.enabled) {
            continue;
        }
        const auto shortage = inventory.shortage(goal.item, goal.target);
        if (!shortage || (*shortage > 0 && !goal.stage_open)) {
            return Completion::Unresolved;
        }
        remaining |= *shortage > 0;
    }
    if (!remaining) {
        return Completion::AllMet;
    }
    return natural_limit_seen ? Completion::NaturalBudgetIncomplete : Completion::UnverifiedStop;
}
}
