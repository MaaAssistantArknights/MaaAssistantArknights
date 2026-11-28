#include "KarlanTaskPlugin.h"

#include <algorithm>

#include "Controller/Controller.h"
#include "KarlanPlanner.h"
#include "Vision/Matcher.h"
#include "Vision/MiniGame/KarlanBoardAnalyzer.h"

namespace asst
{
bool KarlanTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    return msg == AsstMsg::SubTaskStart && details.get("subtask", std::string()) == "ProcessTask" &&
           details.get("details", "task", "") == "MiniGame@OS@OrderBoard";
}

static std::optional<Rect> action_button(const cv::Mat& frame, bool collect)
{
    Matcher matcher(frame);
    matcher.set_task_info(collect ? "MiniGame@OS@CollectAction" : "MiniGame@OS@MoveAction");
    if (auto result = matcher.analyze()) {
        return result->rect;
    }
    return std::nullopt;
}

static bool same_targets(const karlan::Orders& a, const karlan::Orders& b)
{
    for (size_t row = 0; row < a.size(); ++row) {
        if (!std::ranges::equal(a[row], b[row], [](const auto& left, const auto& right) {
                return left.target == right.target && left.resource == right.resource;
            })) {
            return false;
        }
    }
    return true;
}

bool KarlanTaskPlugin::_run()
{
    // Play one hand. The task graph owns end-of-turn OCR, settlement and legacy fallback.
    const auto first = ctrler()->get_image();
    const auto stage = karlan::BoardAnalyzer(first).stage();
    if (!stage || need_exit()) {
        return true;
    }
    const auto& rule = karlan::rules(*stage);
    int production = rule.initial_yield;
    int previous = 0;
    bool can_collect = true;
    std::optional<int> position;
    std::vector<karlan::Action> unavailable;
    std::optional<karlan::Orders> stable_orders;
    std::vector<int> last_hand;
    int retries = 0;
    const auto click_at = [&](int x, int y) {
        return !need_exit() && ctrler()->click(Point { cvRound(x * 4.0 / 3), cvRound(y * 4.0 / 3) });
    };
    while (!need_exit() && retries < 5) {
        const auto image = ctrler()->get_image();
        const karlan::BoardAnalyzer view(image);
        if (view.rules_visible() && view.stage() == stage) {
            if (!click_at(452, 342) || !sleep(500)) {
                return false;
            }
            ++retries;
            continue;
        }
        auto board = view.read(rule, position, production);
        if (!board) {
            if (view.hand().empty()) {
                return true;
            }
            // A selected card can obscure the current tile if the task was started mid-turn.
            if (!position && retries == 0) {
                const auto hand = view.hand();
                for (size_t i = 0; i < hand.size(); ++i) {
                    if (view.selected(static_cast<int>(i))) {
                        click_at(karlan::BoardAnalyzer::slot_x(static_cast<int>(i)), 452);
                        break;
                    }
                }
            }
            ++retries;
            sleep(400);
            continue;
        }
        if (!stable_orders || !same_targets(*stable_orders, board->orders)) {
            // Replacement labels slide in. Require two matching reads before spending a card.
            stable_orders = board->orders;
            unavailable.clear();
            ++retries;
            sleep(250);
            continue;
        }
        if (last_hand != board->cards || position != board->position) {
            unavailable.clear();
            last_hand = board->cards;
        }
        position = board->position;
        production = board->production;
        const auto action = choose(*board, rule, can_collect, previous, unavailable);
        if (!action) {
            break;
        }
        const auto slot = static_cast<int>(std::ranges::find(board->cards, action->card) - board->cards.begin());
        if (!view.selected(slot) && (!click_at(karlan::BoardAnalyzer::slot_x(slot), 458) || !sleep(430))) {
            return false;
        }
        const auto selected = ctrler()->get_image();
        const karlan::BoardAnalyzer selection(selected);
        if (selection.hand() != board->cards || !selection.selected(slot)) {
            ++retries;
            continue;
        }
        const auto button = action_button(selected, action->collect);
        if (!button) {
            unavailable.emplace_back(*action);
            if (action->collect && !board->combos.test(action->card)) {
                can_collect = false;
            }
            continue;
        }
        if (need_exit() || !ctrler()->click(*button) || !sleep(900)) {
            return false;
        }
        auto remaining = board->cards;
        remaining.erase(remaining.begin() + slot);
        bool consumed = false;
        for (int i = 0; i < 5 && !need_exit(); ++i) {
            if (karlan::BoardAnalyzer(ctrler()->get_image()).hand() == remaining) {
                consumed = true;
                break;
            }
            sleep(250);
        }
        if (!consumed) {
            LogWarn << __FUNCTION__ << "card consumption not confirmed";
            return true;
        }
        LogInfo << "Karlan stage" << *stage << "tile" << *position << (action->collect ? "collect" : "move")
                << action->card;
        if (!action->collect) {
            const int destination = *position + action->card;
            if (destination >= static_cast<int>(board->tiles.size())) {
                production += rule.lap_gain;
            }
            position = destination % static_cast<int>(board->tiles.size());
        }
        previous = action->collect ? action->card : 0;
        can_collect = !action->collect;
        retries = 0;
        if (remaining.empty()) {
            return true;
        }
    }
    if (!need_exit()) {
        LogWarn << __FUNCTION__ << "unrecognized hand, resuming task graph";
    }
    return !need_exit();
}
}
