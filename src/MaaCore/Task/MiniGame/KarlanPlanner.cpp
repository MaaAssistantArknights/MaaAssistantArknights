#include "KarlanPlanner.h"

#include <algorithm>
#include <limits>

namespace asst::karlan
{
const Rules& rules(int stage)
{
    using enum Resource;
    static constexpr std::array early = { Mine, Wood, Gear };
    static constexpr std::array late = { Mine, Oil, Metal };
    static constexpr std::array stages = {
        Rules { .resources = early, .initial_yield = 1, .double_yield = true },
        Rules { .resources = early, .initial_yield = 2, .double_yield = true },
        Rules { .resources = early, .initial_yield = 2 },
        Rules { .resources = early, .initial_yield = 2, .locks = true },
        Rules { .resources = early, .initial_yield = 2, .lap_gain = 1 },
        Rules { .resources = late, .initial_yield = 3, .chests = true },
        Rules { .resources = late, .initial_yield = 3 },
        Rules { .resources = late, .initial_yield = 5 },
        Rules { .resources = late, .initial_yield = 5, .locks = true },
        Rules { .resources = late, .initial_yield = 3, .lap_gain = 2, .chests = true },
    };
    return stages.at(stage - 1);
}

// Replacement orders are unknown. Score only visible requirements and replan after each card.
static double score(const Orders& orders)
{
    // The top order awards four points. Within an order the least complete resource limits payout.
    constexpr std::array weights = { 4, 1, 1 };
    double total = 0;
    for (size_t i = 0; i < orders.size(); ++i) {
        double least = 1;
        double sum = 0;
        for (const auto& item : orders[i]) {
            const double progress = static_cast<double>(item.delivered) / item.target;
            least = std::min(least, progress);
            sum += progress;
        }
        total += weights[i] * ((least == 1 ? 1 : 0) + 0.325 * (least + sum / orders[i].size()));
    }
    return total;
}

static void deliver(Orders& orders, Resource resource, int amount, int combo_bonus = 0)
{
    for (auto& order : orders) {
        for (auto& item : order) {
            item.delivered =
                std::min(item.target, item.delivered + (item.resource == resource ? amount : 0) + combo_bonus);
        }
    }
}

static double chest_score(const Orders& orders, const Rules& rule)
{
    // A chest gives 15 of a random resource. Discount the expected progress until its contents are known.
    double sum = 0;
    for (auto kind : rule.resources) {
        auto outcomes = orders;
        deliver(outcomes, kind, 15);
        sum += score(outcomes);
    }
    return 0.60 * (sum / rule.resources.size() - score(orders)) + 0.015;
}

struct SearchResult
{
    double score;
    std::optional<Action> first;
};

static SearchResult search(
    const Board& board,
    const Rules& rule,
    bool collect,
    int previous,
    int depth,
    const std::vector<Action>& unavailable)
{
    if (depth == 0) {
        return { score(board.orders), std::nullopt };
    }
    SearchResult best { -std::numeric_limits<double>::infinity(), std::nullopt };
    for (int card = 1; card <= 5; ++card) {
        const auto slot = std::ranges::find(board.cards, card);
        if (slot == board.cards.end()) {
            continue;
        }
        const bool combo = board.combos.test(card) || (previous != 0 && card == previous + 1);
        for (bool harvest : { false, true }) {
            const Action action { card, harvest };
            const auto& tile = board.tiles[board.position];
            if ((harvest && ((!collect && !combo) || tile.locked)) ||
                std::ranges::find(unavailable, action) != unavailable.end()) {
                continue;
            }
            auto next = board;
            next.cards.erase(next.cards.begin() + (slot - board.cards.begin()));
            next.combos.reset();
            double bonus = 0;
            if (harvest) {
                deliver(next.orders, tile.resource, board.production * card * (tile.boosted ? 2 : 1), combo ? 2 : 0);
            }
            else {
                for (int step = 0; step < card; ++step) {
                    next.position = (next.position + 1) % static_cast<int>(next.tiles.size());
                    if (next.position == 0) {
                        next.production += rule.lap_gain;
                        bonus += 0.006 * rule.lap_gain;
                    }
                    if (next.chests.test(next.position)) {
                        next.chests.reset(next.position);
                        bonus += chest_score(board.orders, rule);
                    }
                }
                if (!next.tiles[next.position].locked) {
                    bonus += 0.0002;
                }
            }
            const auto future = search(next, rule, !harvest, harvest ? card : 0, depth - 1, {});
            const double value = future.score + bonus + 0.012 * score(next.orders);
            if (value > best.score + 1e-9) {
                best = { value, action };
            }
        }
    }
    return best;
}

std::optional<Action> choose(
    const Board& board,
    const Rules& rule,
    bool can_collect,
    int previous,
    const std::vector<Action>& unavailable)
{
    return search(board, rule, can_collect, previous, std::min(3, static_cast<int>(board.cards.size())), unavailable)
        .first;
}
}
