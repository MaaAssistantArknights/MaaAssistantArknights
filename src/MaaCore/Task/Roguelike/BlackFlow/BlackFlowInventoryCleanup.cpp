#include "BlackFlowInventoryCleanup.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <utility>

#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view CleanupCompletedTask = "BlackFlow@Roguelike@InventoryCleanupCompleted";
constexpr std::string_view FullFlagTask = "BlackFlow@Roguelike@InventoryFullFlag";
constexpr std::string_view ItemsTask = "BlackFlow@Roguelike@InventoryCleanupItems";
constexpr std::string_view SwipeForwardTask = "BlackFlow@Roguelike@InventoryCleanupSwipe";
constexpr std::string_view SwipeBackTask = "BlackFlow@Roguelike@InventoryCleanupSwipeBack";
constexpr std::string_view DiscardReadyTask = "BlackFlow@Roguelike@InventoryCleanupDiscardReady";
constexpr std::string_view DiscardTask = "BlackFlow@Roguelike@InventoryCleanupDiscard";
constexpr std::string_view DiscardedTask = "BlackFlow@Roguelike@InventoryCleanupDiscarded";
constexpr std::string_view CloseTask = "BlackFlow@Roguelike@InventoryCleanupClose";

void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}
}

const InventoryCleanupPolicy* BlackFlowInventoryCleanup::resolve_policy(std::string* error) const
{
    if (Task.get<MatchTaskInfo>(std::string(FullFlagTask)) == nullptr ||
        Task.get<OcrTaskInfo>(std::string(ItemsTask)) == nullptr) {
        set_error(error, "inventory cleanup recognition tasks are missing");
        return nullptr;
    }
    return &BlackFlowStrategy.inventory_cleanup_policy();
}

bool BlackFlowInventoryCleanup::full_flag_visible(BlackFlowInventoryContext& context) const
{
    const auto task = Task.get<MatchTaskInfo>(std::string(FullFlagTask));
    const cv::Mat image = context.capture();
    if (task == nullptr || image.empty()) {
        return false;
    }
    Matcher matcher(image);
    matcher.set_task_info(task);
    return matcher.analyze().has_value();
}

std::vector<VisibleScrap>
    BlackFlowInventoryCleanup::recognize(BlackFlowInventoryContext& context, const InventoryCleanupPolicy& policy) const
{
    std::vector<VisibleScrap> visible;
    const auto task = Task.get<OcrTaskInfo>(std::string(ItemsTask));
    const cv::Mat image = context.capture();
    if (task == nullptr || image.empty()) {
        return visible;
    }

    OCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_required(policy.discard_priority);
    const auto results = analyzer.analyze();
    if (!results.has_value()) {
        return visible;
    }

    for (const TextRect& result : *results) {
        const auto found = std::ranges::find(policy.discard_priority, result.text);
        if (found == policy.discard_priority.end()) {
            continue;
        }
        VisibleScrap scrap;
        scrap.name = result.text;
        scrap.rect = result.rect;
        scrap.score = result.score;
        scrap.rank = static_cast<std::size_t>(std::distance(policy.discard_priority.begin(), found));
        scrap.center_x = result.rect.x + result.rect.width / 2;
        scrap.row = m_model.row_of(result.rect.y + result.rect.height / 2);
        visible.emplace_back(std::move(scrap));
    }
    std::ranges::sort(visible, {}, &VisibleScrap::center_x);
    return visible;
}

bool BlackFlowInventoryCleanup::same_view(const std::vector<VisibleScrap>& lhs, const std::vector<VisibleScrap>& rhs)
    const
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    auto left = lhs;
    auto right = rhs;
    const auto order = [](const VisibleScrap& first, const VisibleScrap& second) {
        if (first.row != second.row) {
            return first.row < second.row;
        }
        if (first.center_x != second.center_x) {
            return first.center_x < second.center_x;
        }
        return first.name < second.name;
    };
    std::ranges::sort(left, order);
    std::ranges::sort(right, order);

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].name != right[index].name || left[index].row != right[index].row ||
            std::abs(left[index].center_x - right[index].center_x) >= m_model.layout().settled_shift) {
            return false;
        }
    }
    return true;
}

std::optional<BlackFlowInventoryCleanup::Shift> BlackFlowInventoryCleanup::measure_shift(
    const std::vector<VisibleScrap>& before,
    const std::vector<VisibleScrap>& after) const
{
    // 只取同名同行、且前后两屏各出现一次的项，同名多份会导致配对错误。
    std::vector<Shift> shifts;
    for (const VisibleScrap& current : after) {
        const auto same = [&current](const VisibleScrap& other) {
            return other.name == current.name && other.row == current.row;
        };
        if (std::ranges::count_if(before, same) != 1 || std::ranges::count_if(after, same) != 1) {
            continue;
        }
        const VisibleScrap& previous = *std::ranges::find_if(before, same);
        const int distance = previous.center_x - current.center_x;
        if (distance < 0 || distance >= m_model.layout().column_pitch * 2) {
            continue;
        }
        shifts.emplace_back(Shift { distance, current.name, current.row, static_cast<int>(shifts.size()) });
    }
    if (shifts.empty()) {
        return std::nullopt;
    }
    std::ranges::sort(shifts, {}, &Shift::distance);
    Shift median = shifts[shifts.size() / 2];
    median.measured = static_cast<int>(shifts.size());
    return median;
}

bool BlackFlowInventoryCleanup::swipe(BlackFlowInventoryContext& context, bool forward, std::string* error) const
{
    const std::string task(forward ? SwipeForwardTask : SwipeBackTask);
    if (!context.execute({ task }, error)) {
        return false;
    }
    context.wait(m_model.layout().settle_delay);
    return true;
}

bool BlackFlowInventoryCleanup::rewind_to_left(BlackFlowInventoryContext& context, std::string* error) const
{
    // 回左只覆盖背包的物理宽度，与包含右端空滑的调查预算分开配置。
    const int swipes = m_model.layout().rewind_swipes;
    LogInfo << std::format("BlackFlow inventory | event=rewind_started | swipes={}", swipes);
    for (int step = 0; step < swipes; ++step) {
        if (context.interrupted()) {
            set_error(error, "inventory cleanup interrupted");
            return false;
        }
        if (!swipe(context, false, error)) {
            return false;
        }
        LogInfo << std::format(
            "BlackFlow inventory | event=rewind_step | step={} | total={}",
            step + 1,
            swipes);
    }
    LogInfo << std::format("BlackFlow inventory | event=rewind_completed | swipes={}", swipes);
    return true;
}

bool BlackFlowInventoryCleanup::advance(
    BlackFlowInventoryContext& context,
    const std::vector<VisibleScrap>& visible,
    bool forward,
    std::string* error) const
{
    // 控制端不支持精准滑动时，退回任务配置中的固定距离。
    if (visible.empty() || !context.precise_swipe_supported()) {
        return swipe(context, forward, error);
    }

    const InventoryLayout& layout = m_model.layout();
    int distance = 0;
    if (forward) {
        // 把最右一列拉到落点，下一屏新露出的列即落在落点右侧一个列距处，稳态推进恰好一列。
        const int rightmost = std::ranges::max(visible, {}, &VisibleScrap::center_x).center_x;
        distance = layout.swipe_landing_x - rightmost;
    }
    else {
        // 向左单设落点：新露出的列落在落点左侧一个列距处，紧邻 roi 左边界，余量需大于向右。
        const int leftmost = std::ranges::min(visible, {}, &VisibleScrap::center_x).center_x;
        distance = layout.swipe_landing_back_x - leftmost;
    }
    if (distance == 0) {
        return swipe(context, forward, error);
    }

    const std::string task(forward ? SwipeForwardTask : SwipeBackTask);
    LogInfo << std::format(
        "BlackFlow inventory | event=swipe_command | direction={} | distance={} | landing={}",
        forward ? "forward" : "backward",
        distance,
        forward ? layout.swipe_landing_x : layout.swipe_landing_back_x);
    if (!context.swipe_by(task, distance, error)) {
        return false;
    }
    context.wait(layout.settle_delay);
    return true;
}

bool BlackFlowInventoryCleanup::survey(
    BlackFlowInventoryContext& context,
    const InventoryCleanupPolicy& policy,
    std::string* error)
{
    m_model.clear();
    m_offset = 0;
    m_base_x = 0;
    m_rank_fallback = false;
    m_fallback_best.reset();
    m_view_left_column.reset();
    bool base_known = false;
    int consecutive_same = 0;
    std::vector<VisibleScrap> previous;
    const auto discardable = static_cast<std::size_t>(policy.discard_max_rank);

    for (int step = 0; step < m_model.layout().max_survey_steps; ++step) {
        if (context.interrupted()) {
            set_error(error, "inventory cleanup interrupted");
            return false;
        }
        std::vector<VisibleScrap> visible = recognize(context, policy);
        if (visible.empty()) {
            set_error(error, "inventory survey recognized nothing");
            return false;
        }

        bool best_updated = false;
        for (const VisibleScrap& scrap : visible) {
            if (scrap.rank >= discardable) {
                continue;
            }
            if (!m_fallback_best.has_value() || scrap.rank < m_fallback_best->rank) {
                m_fallback_best = InventoryCell { scrap.name, scrap.rank };
                best_updated = true;
            }
        }

        if (!base_known) {
            m_base_x = std::ranges::min(visible, {}, &VisibleScrap::center_x).center_x;
            base_known = true;
            LogInfo << std::format("BlackFlow inventory | event=survey_started | base_x={}", m_base_x);
        }

        bool stalled = false;
        bool fallback_activated = false;
        bool view_same = false;
        if (!previous.empty()) {
            view_same = same_view(previous, visible);
            if (!m_rank_fallback) {
                const auto shift = measure_shift(previous, visible);
                if (!shift.has_value()) {
                    m_rank_fallback = true;
                    fallback_activated = true;
                    consecutive_same = view_same ? 1 : 0;
                    LogWarn << std::format(
                        "BlackFlow inventory | event=rank_fallback_started | step={} | same_view={} | "
                        "consecutive_same={} | reason=swipe_unmeasured",
                        step,
                        view_same ? 1 : 0,
                        consecutive_same);
                }
                else {
                    m_offset += shift->distance;
                    LogInfo << std::format(
                        "BlackFlow inventory | event=swipe_measured | step={} | distance={} | anchor={} | row={} | "
                        "samples={}",
                        step,
                        shift->distance,
                        shift->anchor,
                        shift->row,
                        shift->measured);
                    stalled = shift->distance < m_model.layout().settled_shift;
                }
            }
            if (m_rank_fallback && !fallback_activated) {
                consecutive_same = view_same ? consecutive_same + 1 : 0;
                LogInfo << std::format(
                    "BlackFlow inventory | event=rank_fallback_view | step={} | same_view={} | consecutive_same={}",
                    step,
                    view_same ? 1 : 0,
                    consecutive_same);
                if (consecutive_same == 2) {
                    LogWarn << std::format(
                        "BlackFlow inventory | event=rank_fallback_confirmed | step={} | remaining_steps={}",
                        step,
                        m_model.layout().max_survey_steps - step - 1);
                }
            }
        }

        if (m_rank_fallback) {
            if (best_updated && m_fallback_best.has_value()) {
                LogInfo << std::format(
                    "BlackFlow inventory | event=best_candidate | name={} | rank={}",
                    m_fallback_best->name,
                    m_fallback_best->rank);
            }
            LogInfo << std::format(
                "BlackFlow inventory | event=survey_step | mode=rank_fallback | step={} | visible={}",
                step,
                visible.size());
            previous = std::move(visible);
            if (step + 1 >= m_model.layout().max_survey_steps) {
                if (!m_fallback_best.has_value()) {
                    set_error(error, "rank fallback found no discardable scrap");
                    return false;
                }
                LogInfo << std::format(
                    "BlackFlow inventory | event=rank_fallback_completed | steps={} | best_name={} | best_rank={}",
                    m_model.layout().max_survey_steps,
                    m_fallback_best->name,
                    m_fallback_best->rank);
                return true;
            }
            if (!advance(context, previous, true, error)) {
                return false;
            }
            continue;
        }

        int fresh = 0;
        for (VisibleScrap& scrap : visible) {
            scrap.column = m_model.column_of(scrap.center_x + m_offset, m_base_x);
            const InventorySlot slot { scrap.column, scrap.row };
            if (m_model.contains(slot)) {
                continue;
            }
            m_model.put(slot, InventoryCell { scrap.name, scrap.rank });
            ++fresh;
            LogInfo << std::format(
                "BlackFlow inventory | event=survey_cell | column={} | row={} | name={} | rank={} | rect={} | "
                "score={}",
                scrap.column,
                scrap.row,
                scrap.name,
                scrap.rank,
                scrap.rect.to_string(),
                scrap.score);
        }
        // 记下当前屏最左边是第几列，走位时用它在名字重复的几列之间按距离取舍。
        m_view_left_column = std::ranges::min(visible, {}, &VisibleScrap::column).column;
        LogInfo << std::format(
            "BlackFlow inventory | event=survey_step | mode=exact | step={} | visible={} | fresh={} | total={}",
            step,
            visible.size(),
            fresh,
            m_model.size());

        if (step > 0 && (fresh == 0 || stalled)) {
            LogInfo << std::format(
                "BlackFlow inventory | event=survey_completed | mode=exact | columns={} | cells={} | fresh={} | "
                "stalled={}",
                m_model.max_column() + 1,
                m_model.size(),
                fresh,
                stalled ? 1 : 0);
            return true;
        }
        previous = std::move(visible);
        if (step + 1 >= m_model.layout().max_survey_steps) {
            break;
        }
        if (!advance(context, previous, true, error)) {
            return false;
        }
    }

    set_error(error, "inventory survey did not reach the right end");
    LogWarn << std::format(
        "BlackFlow inventory | event=survey_failed | steps={} | cells={} | reason=max_steps_exhausted",
        m_model.layout().max_survey_steps,
        m_model.size());
    return false;
}

BlackFlowInventoryCleanup::WalkToResult BlackFlowInventoryCleanup::walk_to(
    BlackFlowInventoryContext& context,
    const InventoryCleanupPolicy& policy,
    InventorySlot slot)
{
    const InventoryCell* wanted = m_model.find(slot);
    if (wanted == nullptr) {
        return { WalkToStatus::TargetUnavailable, std::nullopt, "target slot is absent from the inventory model" };
    }
    std::optional<int> hint = m_view_left_column;

    for (int step = 0; step < m_model.layout().max_walk_steps; ++step) {
        if (context.interrupted()) {
            return { WalkToStatus::Interrupted, std::nullopt, "inventory cleanup interrupted" };
        }
        std::vector<VisibleScrap> visible = recognize(context, policy);
        const auto columns = m_model.resolve_columns(visible, hint);
        if (!columns.has_value()) {
            LogWarn << std::format(
                "BlackFlow inventory | event=viewport_unresolved | step={} | visible={}",
                step,
                visible.size());
            return { WalkToStatus::ViewportUnresolved, std::nullopt, "inventory viewport could not be resolved" };
        }
        hint = columns->front();
        m_view_left_column = *hint;
        LogInfo << std::format(
            "BlackFlow inventory | event=viewport_resolved | step={} | left_column={} | right_column={} | "
            "target_column={}",
            step,
            columns->front(),
            columns->back(),
            slot.first);

        if (std::ranges::find(*columns, slot.first) != columns->end()) {
            const auto found = std::ranges::find_if(visible, [slot](const VisibleScrap& scrap) {
                return scrap.column == slot.first && scrap.row == slot.second;
            });
            if (found == visible.end()) {
                LogWarn << std::format(
                    "BlackFlow inventory | event=target_slot_missing | column={} | row={}",
                    slot.first,
                    slot.second);
                return { WalkToStatus::TargetUnavailable, std::nullopt, "target slot is missing from the viewport" };
            }
            if (found->name != wanted->name) {
                LogWarn << std::format(
                    "BlackFlow inventory | event=target_slot_mismatch | column={} | row={} | expected={} | actual={}",
                    slot.first,
                    slot.second,
                    wanted->name,
                    found->name);
                return { WalkToStatus::TargetUnavailable, std::nullopt, "target slot contents changed" };
            }
            return { WalkToStatus::Reached, *found, {} };
        }

        const bool forward = slot.first > columns->back();
        std::string advance_error;
        if (!advance(context, visible, forward, &advance_error)) {
            return { WalkToStatus::Failed, std::nullopt, std::move(advance_error) };
        }
        // 一次推进正好一列，所以下一步的提示列取当前列加减一，避免两侧等距时无法取舍。
        hint = columns->front() + (forward ? 1 : -1);
        m_view_left_column = *hint;
    }
    LogWarn << std::format("BlackFlow inventory | event=walk_exhausted | column={} | row={}", slot.first, slot.second);
    return { WalkToStatus::Failed, std::nullopt, "inventory walk exhausted before reaching the target" };
}

std::optional<VisibleScrap> BlackFlowInventoryCleanup::walk_to_name(
    BlackFlowInventoryContext& context,
    const InventoryCleanupPolicy& policy,
    std::string_view name,
    std::string* error) const
{
    for (int step = 0; step < m_model.layout().max_walk_steps; ++step) {
        if (context.interrupted()) {
            set_error(error, "inventory cleanup interrupted");
            return std::nullopt;
        }
        std::vector<VisibleScrap> visible = recognize(context, policy);
        const auto found =
            std::ranges::find_if(visible, [name](const VisibleScrap& scrap) { return scrap.name == name; });
        LogInfo << std::format(
            "BlackFlow inventory | event=fallback_search | step={} | name={} | visible={} | found={}",
            step,
            name,
            visible.size(),
            found != visible.end() ? 1 : 0);
        if (found != visible.end()) {
            return *found;
        }
        if (visible.empty()) {
            set_error(error, "rank fallback recognized nothing while searching for: " + std::string(name));
            return std::nullopt;
        }
        if (step + 1 >= m_model.layout().max_walk_steps) {
            break;
        }
        if (!advance(context, visible, false, error)) {
            return std::nullopt;
        }
    }
    set_error(error, "rank fallback could not find: " + std::string(name));
    return std::nullopt;
}

bool BlackFlowInventoryCleanup::discard(
    BlackFlowInventoryContext& context,
    const VisibleScrap& scrap,
    std::string* error)
{
    // 丢弃按钮出现即视为卡片已打开，未出现才重新点击，不以固定等待推断。
    bool opened = false;
    for (int attempt = 0; attempt < m_model.layout().max_card_click_attempts; ++attempt) {
        if (!context.click(scrap.rect)) {
            set_error(error, "failed to click the scrap card: " + scrap.name);
            return false;
        }
        context.wait(m_model.layout().settle_delay);
        if (context.execute({ std::string(DiscardReadyTask) }, nullptr)) {
            opened = true;
            LogInfo << std::format(
                "BlackFlow inventory | event=card_opened | name={} | attempt={}",
                scrap.name,
                attempt + 1);
            break;
        }
        LogWarn << std::format(
            "BlackFlow inventory | event=card_open_failed | name={} | attempt={}",
            scrap.name,
            attempt + 1);
    }
    if (!opened) {
        set_error(error, "discard button never appeared for: " + scrap.name);
        return false;
    }

    if (!context.execute({ std::string(DiscardTask) }, error)) {
        return false;
    }
    if (!context.last_task().ends_with(DiscardedTask)) {
        set_error(error, "discard was not confirmed for: " + scrap.name);
        return false;
    }
    return true;
}

bool BlackFlowInventoryCleanup::finish(BlackFlowInventoryContext& context, bool report, std::string* error)
{
    if (!context.execute({ std::string(CloseTask) }, error)) {
        return abandon(context, error);
    }
    if (!context.last_task().ends_with(CleanupCompletedTask)) {
        set_error(error, "failed to close the scrap box and return to the map");
        return abandon(context, error);
    }
    // 未超载时未做任何操作，不上报清理完成。
    if (report) {
        context.report_cleanup_status("completed");
    }
    LogInfo << std::format("BlackFlow inventory | event=cleanup_completed | discarded={}", m_discarded);
    return true;
}

bool BlackFlowInventoryCleanup::abandon(BlackFlowInventoryContext& context, std::string* error) const
{
    // 主动停止时按成功返回，以免日志上留下一条移动事务失败。
    if (context.interrupted()) {
        LogInfo << std::format("BlackFlow inventory | event=cleanup_interrupted | discarded={}", m_discarded);
        return true;
    }
    const std::string reason = error != nullptr && !error->empty() ? *error : std::string("unknown error");
    LogError
        << std::format("BlackFlow inventory | event=cleanup_failed | discarded={} | reason={}", m_discarded, reason);
    context.report_cleanup_status("failed");
    return false;
}

bool BlackFlowInventoryCleanup::run(BlackFlowInventoryContext& context, std::string* error)
{
    m_failed.clear();
    m_attempts.clear();
    m_discarded = 0;
    m_rescans = 0;
    m_rank_fallback = false;
    m_fallback_best.reset();

    const InventoryCleanupPolicy* policy = resolve_policy(error);
    if (policy == nullptr) {
        return abandon(context, error);
    }
    m_model = InventoryModel(BlackFlowStrategy.inventory_layout());

    if (!full_flag_visible(context)) {
        LogInfo << "BlackFlow inventory | event=cleanup_skipped | reason=not_overloaded";
        return finish(context, false, error);
    }
    context.report_cleanup_status("started");
    LogInfo << "BlackFlow inventory | event=cleanup_started";

    if (!survey(context, *policy, error)) {
        return abandon(context, error);
    }

    const auto discardable = static_cast<std::size_t>(policy->discard_max_rank);
    // 背包格数有限，逐格丢完仍未解除即属本模块异常，此上限只用于防止死循环。
    const int attempts_limit = static_cast<int>(m_model.size()) + 1;
    // 超载解除后按策略再多丢若干件留出富余；解除前不赋值。
    std::optional<int> remaining_after_clear;

    for (int attempt = 0; attempt < attempts_limit; ++attempt) {
        if (context.interrupted()) {
            set_error(error, "inventory cleanup interrupted");
            return abandon(context, error);
        }
        if (!remaining_after_clear.has_value() && !full_flag_visible(context)) {
            remaining_after_clear = policy->extra_discards_after_clear;
            LogInfo << std::format(
                "BlackFlow inventory | event=overload_cleared | discarded={} | extra_remaining={}",
                m_discarded,
                *remaining_after_clear);
        }
        if (remaining_after_clear.has_value() && *remaining_after_clear <= 0) {
            return finish(context, true, error);
        }

        InventoryCell cell;
        InventorySlot slot {};
        std::optional<VisibleScrap> reached;
        const bool fallback_target = m_rank_fallback;
        if (fallback_target) {
            if (!m_fallback_best.has_value()) {
                set_error(error, "rank fallback has no discardable candidate");
                return abandon(context, error);
            }
            cell = *m_fallback_best;
            LogInfo << std::format(
                "BlackFlow inventory | event=discard_target | mode=rank_fallback | name={} | rank={}",
                cell.name,
                cell.rank);
            std::string search_error;
            reached = walk_to_name(context, *policy, cell.name, &search_error);
            if (!reached.has_value()) {
                if (context.interrupted()) {
                    set_error(error, "inventory cleanup interrupted");
                }
                else {
                    set_error(error, std::move(search_error));
                }
                return abandon(context, error);
            }
        }
        else {
            // 同一 rank 试满次数后不再尝试其余实例，避免在同名零件之间反复走位。
            const auto candidates = m_model.ranked_candidates(discardable, m_failed);
            const InventoryCell* picked = nullptr;
            for (const InventorySlot candidate : candidates) {
                const InventoryCell* probe = m_model.find(candidate);
                if (probe == nullptr) {
                    continue;
                }
                const auto tried = m_attempts.find(probe->rank);
                if (tried != m_attempts.end() && tried->second >= policy->max_attempts_per_rank) {
                    continue;
                }
                slot = candidate;
                picked = probe;
                break;
            }
            if (picked == nullptr) {
                if (m_rescans > 0) {
                    set_error(error, "no discardable scrap left after a rescan");
                    return abandon(context, error);
                }
                ++m_rescans;
                LogWarn << std::format(
                    "BlackFlow inventory | event=rescan_started | failed_slots={} | reason=no_candidate",
                    m_failed.size());
                m_failed.clear();
                m_attempts.clear();
                if (!survey(context, *policy, error)) {
                    return abandon(context, error);
                }
                continue;
            }

            cell = *picked;
            if (!m_failed.empty() && cell.rank > static_cast<std::size_t>(policy->rescan_after_rank)) {
                if (m_rescans > 0) {
                    set_error(error, "candidates exhausted past rescan_after_rank");
                    return abandon(context, error);
                }
                ++m_rescans;
                LogWarn << std::format(
                    "BlackFlow inventory | event=rescan_started | rank={} | rank_limit={} | failed_slots={} | "
                    "reason=candidates_degraded",
                    cell.rank,
                    policy->rescan_after_rank,
                    m_failed.size());
                m_failed.clear();
                m_attempts.clear();
                if (!survey(context, *policy, error)) {
                    return abandon(context, error);
                }
                continue;
            }

            LogInfo << std::format(
                "BlackFlow inventory | event=discard_target | mode=exact | name={} | rank={} | column={} | row={} | "
                "failed_slots={}",
                cell.name,
                cell.rank,
                slot.first,
                slot.second,
                m_failed.size());

            WalkToResult walk = walk_to(context, *policy, slot);
            if (walk.status == WalkToStatus::Reached && walk.scrap.has_value()) {
                reached = std::move(walk.scrap);
            }
            else if (walk.status == WalkToStatus::Interrupted) {
                set_error(error, "inventory cleanup interrupted");
                return abandon(context, error);
            }
            else if (walk.status == WalkToStatus::ViewportUnresolved) {
                if (m_rescans > 0) {
                    set_error(error, "inventory viewport remained unresolved after a rescan");
                    return abandon(context, error);
                }
                ++m_rescans;
                LogWarn << std::format(
                    "BlackFlow inventory | event=rescan_started | failed_slots={} | reason=viewport_unresolved",
                    m_failed.size());
                m_failed.clear();
                m_attempts.clear();
                if (!rewind_to_left(context, error) || !survey(context, *policy, error)) {
                    return abandon(context, error);
                }
                continue;
            }
            else if (walk.status == WalkToStatus::TargetUnavailable) {
                m_failed.insert(slot);
                ++m_attempts[cell.rank];
                continue;
            }
            else {
                set_error(error, walk.error.empty() ? "inventory walk failed" : std::move(walk.error));
                return abandon(context, error);
            }
        }

        std::string discard_error;
        if (!discard(context, *reached, &discard_error)) {
            if (context.interrupted()) {
                set_error(error, "inventory cleanup interrupted");
                return abandon(context, error);
            }
            LogWarn << std::format(
                "BlackFlow inventory | event=discard_failed | mode={} | name={} | rank={} | column={} | row={} | "
                "reason={}",
                fallback_target ? "rank_fallback" : "exact",
                cell.name,
                cell.rank,
                slot.first,
                slot.second,
                discard_error);
            if (fallback_target) {
                set_error(error, std::move(discard_error));
                return abandon(context, error);
            }
            m_failed.insert(slot);
            ++m_attempts[cell.rank];
            continue;
        }

        ++m_discarded;
        context.report_cleanup_status("discarded", cell.name);
        LogInfo << std::format(
            "BlackFlow inventory | event=discard_succeeded | mode={} | name={} | rank={} | column={} | row={} | "
            "discarded={}",
            fallback_target ? "rank_fallback" : "exact",
            cell.name,
            cell.rank,
            slot.first,
            slot.second,
            m_discarded);

        if (fallback_target) {
            context.wait(m_model.layout().settle_delay);
            if (remaining_after_clear.has_value()) {
                --*remaining_after_clear;
                LogInfo << std::format(
                    "BlackFlow inventory | event=extra_discard_completed | remaining={}",
                    *remaining_after_clear);
            }
            if (remaining_after_clear.has_value() && *remaining_after_clear <= 0) {
                continue;
            }
            if (!rewind_to_left(context, error)) {
                return abandon(context, error);
            }
            LogInfo << "BlackFlow inventory | event=model_reset | reason=rank_fallback_discard";
            m_failed.clear();
            m_attempts.clear();
            if (!survey(context, *policy, error)) {
                return abandon(context, error);
            }
            continue;
        }

        std::set<InventorySlot> shifted;
        for (const InventorySlot failed : m_failed) {
            const auto moved = m_model.shifted_by_removal(failed, slot);
            if (moved.has_value()) {
                shifted.insert(*moved);
            }
        }
        m_failed = std::move(shifted);
        if (m_view_left_column.has_value()) {
            // 锚点被删除时 shifted_by_removal 返回空，后继零件补入同一格，左列编号保持不变。
            const InventorySlot view_anchor { *m_view_left_column, 0 };
            if (const auto moved = m_model.shifted_by_removal(view_anchor, slot); moved.has_value()) {
                m_view_left_column = moved->first;
            }
        }
        m_model.collapse(slot);
        context.wait(m_model.layout().settle_delay);

        if (remaining_after_clear.has_value()) {
            --*remaining_after_clear;
            LogInfo << std::format(
                "BlackFlow inventory | event=extra_discard_completed | remaining={}",
                *remaining_after_clear);
        }
        LogInfo << std::format("BlackFlow inventory | event=model_size | cells={}", m_model.size());
    }

    set_error(error, "inventory is still overloaded after discarding every candidate");
    return abandon(context, error);
}
}
