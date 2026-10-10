#include "BlackFlowScrapTradeInventory.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <utility>

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view ItemsTask = "BlackFlow@Roguelike@ScrapTradeItems";
constexpr std::string_view SwipeForwardTask = "BlackFlow@Roguelike@ScrapTradeInventorySwipe";
constexpr std::string_view SwipeBackTask = "BlackFlow@Roguelike@ScrapTradeInventorySwipeBack";
constexpr int Columns = 4;
constexpr int ColumnPitch = 206;
constexpr int RowPitch = 210;
constexpr int FirstRowY = 195;
constexpr int SettledShift = 40;
constexpr int MaxSurveySteps = 12;
constexpr int MaxWalkSteps = 16;
constexpr unsigned SettleDelay = 300;
constexpr int MaxCardClickAttempts = 3;
constexpr std::string_view SaleReadyTask = "BlackFlow@Roguelike@ScrapTradeSaleReady";
constexpr std::string_view SaleConfirmTask = "BlackFlow@Roguelike@ScrapTradeSellConfirm";
constexpr std::string_view SoldTask = "ScrapTradeSold";

void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}
} // namespace

void ScrapTradeInventoryModel::reset(std::vector<std::string> names)
{
    m_items = std::move(names);
}

ScrapTradeInventoryItem ScrapTradeInventoryModel::item(std::size_t index) const
{
    const int slot = static_cast<int>(index) + 1;
    return { m_items.at(index), slot / Columns, slot % Columns };
}

void ScrapTradeInventoryModel::erase(std::size_t index)
{
    // 删除后 vector 的后继元素前移，跨行位置由 item() 统一换算。
    (void)m_items.at(index);
    m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
}

void BlackFlowScrapTradeInventory::invalidate()
{
    m_model.reset({});
    m_mode = LocationMode::Unknown;
    m_observed.clear();
    forget_position();
}

void BlackFlowScrapTradeInventory::forget_position()
{
    m_offset.reset();
}

std::optional<BlackFlowScrapTradeInventory::View> BlackFlowScrapTradeInventory::recognize(
    BlackFlowScrapTradeInventoryContext& context,
    const std::vector<std::string>& names,
    std::string* error) const
{
    if (context.interrupted()) {
        set_error(error, "scrap trade inventory interrupted");
        return std::nullopt;
    }
    const auto task = Task.get<OcrTaskInfo>(std::string(ItemsTask));
    const cv::Mat image = context.capture();
    if (task == nullptr || image.empty() || names.empty()) {
        set_error(error, "scrap trade inventory recognition input is missing");
        return std::nullopt;
    }
    OCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_required(names);
    const auto result = analyzer.analyze();
    if (!result.has_value() || result->empty()) {
        set_error(error, "scrap trade inventory recognized no items");
        return std::nullopt;
    }
    View view;
    for (const auto& text : *result) {
        // 名称左沿位于各卡片的左侧；首行第一格是园圃，不在零件名称表内。
        const int column = (text.rect.x - task->roi.x) / ColumnPitch;
        if (column < 0 || column >= Columns) {
            continue;
        }
        view.push_back({ text, column, text.rect.y + text.rect.height / 2 });
    }
    std::ranges::sort(view, [](const VisibleItem& left, const VisibleItem& right) {
        if (left.column != right.column) {
            return left.column < right.column;
        }
        return left.center_y < right.center_y;
    });
    if (view.empty()) {
        set_error(error, "scrap trade inventory recognized no grid items");
        return std::nullopt;
    }
    return view;
}

std::optional<int> BlackFlowScrapTradeInventory::measure_shift(const View& before, const View& after) const
{
    const auto task = Task.get<OcrTaskInfo>(std::string(ItemsTask));
    if (task == nullptr) {
        return std::nullopt;
    }
    std::vector<int> candidates;
    for (const auto& old : before) {
        for (const auto& current : after) {
            if (old.column == current.column && old.text.text == current.text.text) {
                const int shift = old.center_y - current.center_y;
                if (std::abs(shift) < RowPitch * 2) {
                    candidates.emplace_back(shift);
                }
            }
        }
    }
    std::ranges::sort(candidates);
    std::vector<int> resolved;
    for (std::size_t first = 0; first < candidates.size();) {
        std::size_t last = first + 1;
        while (last < candidates.size() && candidates[last] - candidates[first] < SettledShift) {
            ++last;
        }
        const int shift = candidates[first + (last - first) / 2];
        first = last;
        // 每个候选必须解释整个重叠区域；同名物品依照列和移动后的坐标一对一对应。
        const auto agrees = [&task](const View& source, const View& target, int delta) {
            return std::ranges::all_of(source, [&](const VisibleItem& item) {
                const int y = item.text.rect.y - delta;
                if (y < task->roi.y || y + item.text.rect.height > task->roi.y + task->roi.height) {
                    return true;
                }
                const auto matches = [&](const VisibleItem& other) {
                    return other.column == item.column && other.text.text == item.text.text &&
                           std::abs(item.center_y - delta - other.center_y) < SettledShift;
                };
                return std::ranges::count_if(target, matches) == 1;
            });
        };
        if (agrees(before, after, shift) && agrees(after, before, -shift)) {
            resolved.emplace_back(shift);
        }
    }
    // 整行完全重复时可能同时支持零位移和一行位移，不能据此累加件数。
    return resolved.size() == 1 ? std::optional<int>(resolved.front()) : std::nullopt;
}

bool BlackFlowScrapTradeInventory::same_view(const View& before, const View& after) const
{
    return std::ranges::equal(before, after, [](const VisibleItem& left, const VisibleItem& right) {
        return left.column == right.column && left.text.text == right.text.text &&
               std::abs(left.center_y - right.center_y) < SettledShift;
    });
}

bool BlackFlowScrapTradeInventory::advance(
    BlackFlowScrapTradeInventoryContext& context,
    const View& view,
    bool forward,
    std::string* error) const
{
    if (context.interrupted()) {
        set_error(error, "scrap trade inventory interrupted");
        return false;
    }
    std::optional<int> distance;
    if (context.precise_swipe_supported() && !view.empty()) {
        // 下翻时将下排名称拉到第一排，上翻时将上排名称拉到第二排。
        const int anchor = forward ? std::ranges::max(view, {}, &VisibleItem::center_y).center_y
                                   : std::ranges::min(view, {}, &VisibleItem::center_y).center_y;
        const int desired = (forward ? FirstRowY : FirstRowY + RowPitch) - anchor;
        if ((forward && desired < -SettledShift) || (!forward && desired > SettledShift)) {
            distance = desired;
        }
    }
    const std::string_view task = forward ? SwipeForwardTask : SwipeBackTask;
    LogInfo << "BlackFlow scrap inventory swipe" << "forward" << forward << "dynamic" << distance.has_value()
            << "distance" << distance.value_or(forward ? -RowPitch : RowPitch);
    if (!context.swipe_by(task, distance, error)) {
        return false;
    }
    context.wait(SettleDelay);
    return true;
}

std::optional<BlackFlowScrapTradeInventory::View> BlackFlowScrapTradeInventory::rewind(
    BlackFlowScrapTradeInventoryContext& context,
    const std::vector<std::string>& names,
    std::string* error,
    bool* at_top) const
{
    if (at_top != nullptr) {
        *at_top = false;
    }
    auto previous = recognize(context, names, error);
    if (!previous) {
        return std::nullopt;
    }
    int stationary = 0;
    for (int step = 0; step < MaxWalkSteps; ++step) {
        if (!advance(context, *previous, false, error)) {
            return std::nullopt;
        }
        auto current = recognize(context, names, error);
        if (!current) {
            return std::nullopt;
        }
        const auto shift = measure_shift(*previous, *current);
        stationary = shift && std::abs(*shift) < SettledShift && same_view(*previous, *current) ? stationary + 1 : 0;
        LogInfo << "BlackFlow scrap inventory rewind" << "step" << step << "measured" << shift.has_value() << "shift"
                << shift.value_or(0) << "stationary" << stationary;
        if (stationary >= 2) {
            if (at_top != nullptr) {
                *at_top = true;
            }
            return current;
        }
        previous = std::move(current);
    }
    return previous;
}

std::optional<std::vector<std::string>> BlackFlowScrapTradeInventory::survey(
    BlackFlowScrapTradeInventoryContext& context,
    const std::vector<std::string>& names,
    std::string* error)
{
    set_error(error, {});
    invalidate();
    bool indexed = false;
    auto view = rewind(context, names, error, &indexed);
    if (!view) {
        return std::nullopt;
    }
    std::map<std::pair<int, int>, std::string> cells;
    std::map<std::string, int> observed;
    const auto save_segment = [&] {
        std::map<std::string, int> counts;
        int expected = indexed ? 1 : cells.begin()->first.first * Columns + cells.begin()->first.second;
        for (const auto& [slot, name] : cells) {
            if (slot.first * Columns + slot.second != expected++) {
                set_error(error, "scrap trade inventory contains an unrecognized slot");
                return false;
            }
            ++counts[name];
        }
        for (const auto& [name, count] : counts) {
            observed[name] = std::max(observed[name], count);
        }
        return true;
    };
    const auto use_names = [&] {
        if (!save_segment()) {
            return;
        }
        for (const auto& [name, count] : observed) {
            m_observed.insert(m_observed.end(), static_cast<std::size_t>(count), name);
        }
        m_mode = LocationMode::ByName;
        LogWarn << "BlackFlow scrap inventory position ambiguous, using names";
    };
    int offset = indexed ? 0 : FirstRowY - std::ranges::min(*view, {}, &VisibleItem::center_y).center_y;
    int stationary = 0;
    for (int step = 0; step < MaxSurveySteps; ++step) {
        for (const auto& item : *view) {
            const int absolute_y = item.center_y + offset;
            const int row = static_cast<int>(std::lround(static_cast<double>(absolute_y - FirstRowY) / RowPitch));
            const std::pair<int, int> slot { row, item.column };
            if (row < 0 || (indexed && slot == std::pair<int, int> { 0, 0 }) ||
                std::abs(absolute_y - (FirstRowY + row * RowPitch)) >= SettledShift) {
                set_error(error, "scrap trade inventory item does not align with the grid");
                return std::nullopt;
            }
            const auto [found, fresh] = cells.emplace(slot, item.text.text);
            if (!fresh && found->second != item.text.text) {
                set_error(error, "scrap trade inventory overlap disagrees with the recorded item");
                return std::nullopt;
            }
            if (fresh) {
                LogInfo << "BlackFlow scrap inventory item" << "row" << row << "column" << item.column << "name"
                        << item.text.text << "indexed" << indexed;
            }
        }
        LogInfo << "BlackFlow scrap inventory survey" << "step" << step << "offset" << offset << "total" << cells.size()
                << "indexed" << indexed;
        if (stationary >= 2) {
            if (!indexed) {
                use_names();
                return std::nullopt;
            }
            // 首格由园圃占用，其余格子按行连续；有空洞说明完整计数尚未建立。
            int expected = 1;
            std::vector<std::string> held;
            for (const auto& [slot, name] : cells) {
                if (slot.first * Columns + slot.second != expected++) {
                    set_error(error, "scrap trade inventory contains an unrecognized slot");
                    return std::nullopt;
                }
                held.emplace_back(name);
            }
            for (const auto& [slot, name] : cells) {
                context.on_inventory_item({ name, slot.first, slot.second });
            }
            m_model.reset(held);
            m_offset = offset;
            m_mode = LocationMode::Indexed;
            m_observed = held;
            return held;
        }
        if (step + 1 == MaxSurveySteps) {
            break;
        }
        if (!advance(context, *view, true, error)) {
            return std::nullopt;
        }
        auto next = recognize(context, names, error);
        if (!next) {
            return std::nullopt;
        }
        const auto shift = measure_shift(*view, *next);
        if (!shift) {
            // 继续读后面的加工品；不能确定段间重叠时，不累加同名物品数量。
            if (!save_segment()) {
                return std::nullopt;
            }
            cells.clear();
            indexed = false;
            stationary = 0;
            offset = FirstRowY - std::ranges::min(*next, {}, &VisibleItem::center_y).center_y;
        }
        else {
            if (*shift <= -SettledShift) {
                set_error(error, "scrap trade inventory moved opposite to the requested direction");
                return std::nullopt;
            }
            stationary = std::abs(*shift) < SettledShift && same_view(*view, *next) ? stationary + 1 : 0;
            offset += *shift;
        }
        view = std::move(next);
    }
    if (!indexed) {
        use_names();
    }
    else {
        set_error(error, "scrap trade inventory survey did not reach the bottom");
    }
    return std::nullopt;
}

std::optional<int> BlackFlowScrapTradeInventory::resolve_offset(const View& view) const
{
    if (view.empty()) {
        return std::nullopt;
    }
    const auto& anchor = view.front();
    std::optional<int> best;
    int best_distance = 0;
    bool ambiguous = false;
    for (std::size_t index = 0; index < m_model.items().size(); ++index) {
        const auto cell = m_model.item(index);
        if (cell.column != anchor.column || cell.name != anchor.text.text) {
            continue;
        }
        const int offset = FirstRowY + cell.row * RowPitch - anchor.center_y;
        if (offset < -SettledShift) {
            continue;
        }
        const bool matches = std::ranges::all_of(view, [&](const VisibleItem& visible) {
            const int absolute_y = visible.center_y + offset;
            const int row = static_cast<int>(std::lround(static_cast<double>(absolute_y - FirstRowY) / RowPitch));
            const int slot = row * Columns + visible.column;
            return row >= 0 && slot > 0 && slot <= static_cast<int>(m_model.items().size()) &&
                   std::abs(absolute_y - (FirstRowY + row * RowPitch)) < SettledShift &&
                   m_model.items()[slot - 1] == visible.text.text;
        });
        if (!matches) {
            continue;
        }
        const int distance = m_offset ? std::abs(offset - *m_offset) : 0;
        if (!best || distance < best_distance) {
            best = offset;
            best_distance = distance;
            ambiguous = false;
        }
        else if (distance == best_distance && offset != *best) {
            ambiguous = true;
        }
    }
    return ambiguous ? std::nullopt : best;
}

std::optional<ScrapTradeInventoryTarget> BlackFlowScrapTradeInventory::find(
    BlackFlowScrapTradeInventoryContext& context,
    const std::vector<std::string>& names,
    const std::vector<std::string>& wanted,
    std::string* error)
{
    set_error(error, {});
    if (wanted.empty()) {
        return std::nullopt;
    }
    if (m_mode == LocationMode::Unknown && !survey(context, names, error) && m_mode != LocationMode::ByName) {
        return std::nullopt;
    }
    if (m_mode == LocationMode::ByName) {
        const auto target = find_by_name(context, names, wanted, error);
        if (target) {
            return ScrapTradeInventoryTarget { *target, std::nullopt };
        }
        return std::nullopt;
    }
    const auto first = std::ranges::find_if(m_model.items(), [&wanted](const std::string& name) {
        return std::ranges::find(wanted, name) != wanted.end();
    });
    if (first == m_model.items().end()) {
        return std::nullopt;
    }
    const auto target_index = static_cast<std::size_t>(first - m_model.items().begin());
    auto view = recognize(context, names, error);
    for (int step = 0; view && step <= MaxWalkSteps; ++step) {
        const auto offset = resolve_offset(*view);
        if (!offset) {
            m_mode = LocationMode::ByName;
            // 数量账本仍由交易确认维护；位置不确定时只查当前可见名称。
            LogWarn << "BlackFlow scrap inventory position unresolved, searching by name";
            const auto target = find_by_name(context, names, wanted, error);
            if (target) {
                return ScrapTradeInventoryTarget { *target, std::nullopt };
            }
            return std::nullopt;
        }
        m_offset = *offset;
        for (const auto& visible : *view) {
            const int row =
                static_cast<int>(std::lround(static_cast<double>(visible.center_y + *offset - FirstRowY) / RowPitch));
            const auto index = static_cast<std::size_t>(row * Columns + visible.column - 1);
            if (index == target_index) {
                return ScrapTradeInventoryTarget { visible.text, index };
            }
        }
        if (step == MaxWalkSteps) {
            break;
        }
        const int top_y = std::ranges::min(*view, {}, &VisibleItem::center_y).center_y;
        const int bottom_y = std::ranges::max(*view, {}, &VisibleItem::center_y).center_y;
        const int target_y = FirstRowY + m_model.item(target_index).row * RowPitch - *offset;
        if (target_y >= top_y - SettledShift && target_y <= bottom_y + SettledShift) {
            set_error(error, "scrap trade inventory target is missing from its visible slot");
            return std::nullopt;
        }
        if (!advance(context, *view, target_y > bottom_y, error)) {
            return std::nullopt;
        }
        auto next = recognize(context, names, error);
        if (!next) {
            return std::nullopt;
        }
        const auto shift = measure_shift(*view, *next);
        m_offset = shift ? std::optional<int>(*offset + *shift) : std::nullopt;
        view = std::move(next);
    }
    if (view) {
        set_error(error, "scrap trade inventory could not reach the target slot");
    }
    return std::nullopt;
}

bool BlackFlowScrapTradeInventory::sell(
    BlackFlowScrapTradeInventoryContext& context,
    const ScrapTradeInventoryTarget& target,
    std::string* error)
{
    set_error(error, {});
    if (target.index && (m_mode != LocationMode::Indexed || *target.index >= m_model.items().size() ||
                         m_model.items()[*target.index] != target.text.text)) {
        set_error(error, "scrap trade inventory sale target no longer matches the model");
        return false;
    }
    const auto ready = [&context] {
        if (context.interrupted()) {
            return false;
        }
        const cv::Mat image = context.capture();
        if (image.empty()) {
            return false;
        }
        Matcher analyzer(image);
        analyzer.set_task_info(std::string(SaleReadyTask));
        return analyzer.analyze().has_value();
    };
    bool opened = false;
    for (int attempt = 0; attempt < MaxCardClickAttempts; ++attempt) {
        if (context.interrupted()) {
            set_error(error, "scrap trade inventory interrupted");
            return false;
        }
        // 延迟出现的确认框先检查，避免再次点击卡片位置。
        if (attempt > 0 && ready()) {
            opened = true;
            break;
        }
        if (context.interrupted() || !context.click(target.text.rect)) {
            set_error(error, "scrap trade inventory could not click the sale item");
            return false;
        }
        context.wait(SettleDelay);
        if (ready()) {
            opened = true;
            break;
        }
        LogWarn << "BlackFlow scrap trade sale dialog not opened" << target.text.text << "attempt" << attempt + 1;
    }
    if (context.interrupted()) {
        set_error(error, "scrap trade inventory interrupted");
        return false;
    }
    if (!opened) {
        set_error(error, "scrap trade sale confirmation never appeared: " + target.text.text);
        return false;
    }
    if (!context.execute(SaleConfirmTask, error)) {
        return false;
    }
    if (context.interrupted() || !context.last_task().ends_with(SoldTask)) {
        set_error(error, "scrap trade sale was not completed: " + target.text.text);
        return false;
    }
    if (target.index) {
        m_model.erase(*target.index);
        // 确认出售后列表回到顶部；物品顺序继续沿用补位后的记录。
        m_offset = 0;
    }
    else {
        m_mode = LocationMode::ByName;
        m_offset = 0;
    }
    LogInfo << "BlackFlow scrap inventory sale completed" << target.text.text << "indexed"
            << (m_mode == LocationMode::Indexed);
    if (m_mode == LocationMode::Indexed) {
        LogInfo << "BlackFlow scrap inventory remaining" << m_model.items().size();
    }
    return true;
}

std::optional<TextRect> BlackFlowScrapTradeInventory::find_by_name(
    BlackFlowScrapTradeInventoryContext& context,
    const std::vector<std::string>& names,
    const std::vector<std::string>& wanted,
    std::string* error) const
{
    set_error(error, {});
    if (wanted.empty()) {
        return std::nullopt;
    }
    // 尽量回顶后按行查找；回顶位置不明仍可使用当前可见的允许出售项。
    auto view = m_offset == 0 ? recognize(context, names, error) : rewind(context, names, error);
    if (!view) {
        return std::nullopt;
    }
    int stationary = 0;
    for (int step = 0; step <= MaxWalkSteps; ++step) {
        const VisibleItem* first = nullptr;
        for (const auto& item : *view) {
            if (std::ranges::find(wanted, item.text.text) == wanted.end()) {
                continue;
            }
            if (first == nullptr || item.center_y < first->center_y - SettledShift ||
                (std::abs(item.center_y - first->center_y) < SettledShift && item.column < first->column)) {
                first = &item;
            }
        }
        if (first != nullptr) {
            return first->text;
        }
        if (stationary >= 2) {
            return std::nullopt;
        }
        if (step == MaxWalkSteps) {
            break;
        }
        if (!advance(context, *view, true, error)) {
            return std::nullopt;
        }
        auto next = recognize(context, names, error);
        if (!next) {
            return std::nullopt;
        }
        const auto shift = measure_shift(*view, *next);
        stationary = shift && std::abs(*shift) < SettledShift && same_view(*view, *next) ? stationary + 1 : 0;
        LogInfo << "BlackFlow scrap inventory search" << "step" << step << "name fallback" << !shift.has_value()
                << "stationary" << stationary;
        view = std::move(next);
    }
    set_error(error, "scrap trade inventory search exhausted before confirming the bottom");
    return std::nullopt;
}
} // namespace asst::blackflow
