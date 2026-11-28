#include "KarlanTaskPlugin.h"

#include <algorithm>

#include "Controller/Controller.h"
#include "KarlanPlanner.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MiniGame/KarlanBoardAnalyzer.h"

namespace asst
{
static constexpr std::string_view OrderBoardTask = "MiniGame@OS@OrderBoard";

bool KarlanTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    return msg == AsstMsg::SubTaskStart && details.get("subtask", std::string()) == "ProcessTask" &&
           details.get("details", "task", "") == OrderBoardTask;
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

bool KarlanTaskPlugin::_run()
{
    auto& process = static_cast<ProcessTask&>(*m_task_ptr); // verify() matches only this process task callback.
    process.remove_override_next(OrderBoardTask);
    const auto stage = karlan::BoardAnalyzer(ctrler()->get_image()).stage();
    if (need_exit()) {
        return false;
    }
    if (!stage) {
        LogWarn << __FUNCTION__ << "stage not recognized, using legacy card selection";
        return true;
    }
    // ProcessTask runs its next node even if a plugin fails. After a click, only a verified
    // end-of-turn or settlement screen may take over while hand consumption is uncertain.
    if (!process.override_next(OrderBoardTask, { "MiniGame@OS@CheckPointsUsed", "MiniGame@OS@ClickToContinue" })) {
        return false;
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
            stable_orders.reset();
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
        if (stable_orders != board->orders) {
            // Quantities can keep changing after the new order's resource and target appear.
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
            LogWarn << __FUNCTION__ << "card consumption not confirmed, waiting for a turn boundary";
            return false;
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
        // No action is outstanding at this point; the legacy selector can safely take over.
        process.remove_override_next(OrderBoardTask);
        LogWarn << __FUNCTION__ << "unrecognized hand, using legacy card selection";
    }
    return !need_exit();
}
}
