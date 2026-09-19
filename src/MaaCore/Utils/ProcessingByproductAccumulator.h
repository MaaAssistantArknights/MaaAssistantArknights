#pragma once

#include <algorithm>
#include <map>
#include <utility>

#include "MaterialCraftPlanner.h"

namespace asst
{
// Toasts persist across screenshots. Count an item only after two agreeing observations,
// and keep the largest confirmed quantity instead of adding the same toast repeatedly.
class ProcessingByproductAccumulator
{
public:
    explicit ProcessingByproductAccumulator(int batches) :
        m_batches(batches)
    {
    }

    void observe(const MaterialInventory& items)
    {
        int64_t total = 0;
        for (const auto& [id, count] : items) {
            if (id.empty() || count <= 0 || count > m_batches) {
                return;
            }
            total += count;
        }
        if (total > m_batches) {
            return;
        }
        for (const auto& [id, count] : items) {
            if (++m_observations[{ id, count }] >= 2) {
                m_confirmed[id] = std::max(m_confirmed[id], count);
            }
        }
    }

    MaterialInventory confirmed() const
    {
        int64_t total = 0;
        for (const auto& [_, count] : m_confirmed) {
            total += count;
        }
        return total <= m_batches ? m_confirmed : MaterialInventory {};
    }

private:
    int m_batches;
    std::map<std::pair<std::string, int>, int> m_observations;
    MaterialInventory m_confirmed;
};
}
