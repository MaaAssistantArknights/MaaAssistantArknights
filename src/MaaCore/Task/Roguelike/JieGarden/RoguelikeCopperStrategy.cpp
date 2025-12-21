#include "RoguelikeCopperStrategy.h"
#include <algorithm>

namespace asst
{

static CopperPickupResult default_pickup_strategy(
    const std::vector<RoguelikeCopper>& options,
    [[maybe_unused]] const std::vector<RoguelikeCopper>& existing)
{
    if (options.empty()) {
        return { 0, "empty options" };
    }

    auto it = std::max_element(options.begin(), options.end(),
        [](const auto& a, const auto& b) { return a.pickup_priority < b.pickup_priority; });

    return { static_cast<size_t>(std::distance(options.begin(), it)), "highest priority" };
}

static CopperPickupResult findling_pickup_strategy(
    const std::vector<RoguelikeCopper>& options,
    [[maybe_unused]] const std::vector<RoguelikeCopper>& existing)
{
    if (options.empty()) {
        return { 0, "empty options" };
    }

    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i].type == CopperType::Li) {
            return { i, "Li-type" };
        }
    }

    auto it = std::max_element(options.begin(), options.end(),
        [](const auto& a, const auto& b) { return a.pickup_priority < b.pickup_priority; });

    return { static_cast<size_t>(std::distance(options.begin(), it)), "no Li, highest priority" };
}

static CopperExchangeResult default_exchange_strategy(
    const RoguelikeCopper& new_copper,
    const std::vector<RoguelikeCopper>& existing)
{
    if (existing.empty()) {
        return { false, 0, "empty existing" };
    }

    auto worst = std::max_element(existing.begin(), existing.end(),
        [](const auto& a, const auto& b) {
            return a.get_copper_discard_priority() < b.get_copper_discard_priority();
        });

    if (worst->get_copper_discard_priority() >= new_copper.get_copper_discard_priority()) {
        return { true, static_cast<size_t>(std::distance(existing.begin(), worst)), "better" };
    }

    return { false, 0, "worse" };
}

static CopperExchangeResult findling_exchange_strategy(
    const RoguelikeCopper& new_copper,
    const std::vector<RoguelikeCopper>& existing)
{
    if (existing.empty()) {
        return { false, 0, "empty existing" };
    }

    if (new_copper.type == CopperType::Li) {
        size_t worst_idx = SIZE_MAX;
        int worst_priority = -1;

        for (size_t i = 0; i < existing.size(); ++i) {
            if (existing[i].type != CopperType::Li) {
                int priority = existing[i].get_copper_discard_priority();
                if (worst_idx == SIZE_MAX || priority > worst_priority) {
                    worst_idx = i;
                    worst_priority = priority;
                }
            }
        }

        if (worst_idx != SIZE_MAX) {
            return { true, worst_idx, "Li replace non-Li" };
        }

        auto worst = std::max_element(existing.begin(), existing.end(),
            [](const auto& a, const auto& b) {
                return a.get_copper_discard_priority() < b.get_copper_discard_priority();
            });
        return { true, static_cast<size_t>(std::distance(existing.begin(), worst)), "Li replace worst Li" };
    }

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i].type != CopperType::Li &&
            existing[i].get_copper_discard_priority() > new_copper.get_copper_discard_priority()) {
            return { true, i, "replace worse non-Li" };
        }
    }

    return { false, 0, "keep existing" };
}

CopperStrategy get_copper_strategy(CopperStrategyType type)
{
    if (type == CopperStrategyType::FindLingMode) {
        return { findling_pickup_strategy, findling_exchange_strategy };
    }
    return { default_pickup_strategy, default_exchange_strategy };
}

}
