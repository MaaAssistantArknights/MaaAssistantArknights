#include "RoguelikeCopperStrategy.h"
#include <algorithm>
#include <limits>

namespace asst
{

static CopperPickupResult default_pickup_strategy(
    const std::vector<RoguelikeCopper>& options,
    [[maybe_unused]] const std::vector<RoguelikeCopper>& existing)
{
    if (options.empty()) {
        return { 0, "empty options" };
    }

    auto it = std::max_element(options.begin(), options.end(), [](const auto& a, const auto& b) {
        return a.pickup_priority < b.pickup_priority;
    });

    return { static_cast<size_t>(std::distance(options.begin(), it)), "highest priority" };
}

static CopperPickupResult findplaytime_pickup_strategy(
    const std::vector<RoguelikeCopper>& options,
    [[maybe_unused]] const std::vector<RoguelikeCopper>& existing)
{
    if (options.empty()) {
        return { 0, "empty options" };
    }

    size_t best_li_idx = std::numeric_limits<size_t>::max();
    int best_li_priority = std::numeric_limits<int>::min();

    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i].type == CopperType::Li && options[i].pickup_priority > best_li_priority) {
            best_li_idx = i;
            best_li_priority = options[i].pickup_priority;
        }
    }

    if (best_li_idx != std::numeric_limits<size_t>::max()) {
        return { best_li_idx, "best Li by pickup priority" };
    }

    auto it = std::max_element(options.begin(), options.end(), [](const auto& a, const auto& b) {
        return a.pickup_priority < b.pickup_priority;
    });

    return { static_cast<size_t>(std::distance(options.begin(), it)), "no Li, highest priority" };
}

static CopperExchangeResult
    default_exchange_strategy(const RoguelikeCopper& new_copper, const std::vector<RoguelikeCopper>& existing)
{
    if (existing.empty()) {
        return { false, 0, "empty existing" };
    }

    auto worst = std::max_element(existing.begin(), existing.end(), [](const auto& a, const auto& b) {
        return a.get_copper_discard_priority() < b.get_copper_discard_priority();
    });

    if (worst->get_copper_discard_priority() >= new_copper.get_copper_discard_priority()) {
        return { true, static_cast<size_t>(std::distance(existing.begin(), worst)), "better" };
    }

    return { false, 0, "worse" };
}

static CopperExchangeResult
    findplaytime_exchange_strategy(const RoguelikeCopper& new_copper, const std::vector<RoguelikeCopper>& existing)
{
    if (existing.empty()) {
        return { false, 0, "empty existing" };
    }

    if (new_copper.type == CopperType::Li) {
        size_t worst_idx = std::numeric_limits<size_t>::max();
        int worst_priority = -1;

        for (size_t i = 0; i < existing.size(); ++i) {
            if (existing[i].type != CopperType::Li) {
                int priority = existing[i].get_copper_discard_priority();
                if (worst_idx == std::numeric_limits<size_t>::max() || priority > worst_priority) {
                    worst_idx = i;
                    worst_priority = priority;
                }
            }
        }

        if (worst_idx != std::numeric_limits<size_t>::max()) {
            return { true, worst_idx, "Li replace non-Li" };
        }

        auto worst = std::max_element(existing.begin(), existing.end(), [](const auto& a, const auto& b) {
            return a.get_copper_discard_priority() < b.get_copper_discard_priority();
        });
        const int worst_li_priority = worst->get_copper_discard_priority();
        const int new_li_priority = new_copper.get_copper_discard_priority();
        if (new_li_priority < worst_li_priority) {
            return { true, static_cast<size_t>(std::distance(existing.begin(), worst)), "Li replace worse Li" };
        }
        return { false, 0, "keep existing Li" };
    }

    size_t worst_non_li_idx = std::numeric_limits<size_t>::max();
    int worst_non_li_priority = new_copper.get_copper_discard_priority();

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i].type == CopperType::Li) {
            continue;
        }

        const int priority = existing[i].get_copper_discard_priority();
        if (priority > worst_non_li_priority) {
            worst_non_li_priority = priority;
            worst_non_li_idx = i;
        }
    }

    if (worst_non_li_idx != std::numeric_limits<size_t>::max()) {
        return { true, worst_non_li_idx, "replace worst non-Li" };
    }

    return { false, 0, "keep existing" };
}

CopperStrategy get_copper_strategy(CopperStrategyType type)
{
    if (type == CopperStrategyType::FindPlaytimeLingMode) {
        return { findplaytime_pickup_strategy, findplaytime_exchange_strategy };
    }
    return { default_pickup_strategy, default_exchange_strategy };
}

}
