#include "BlackFlowMovementTaskPlugin.h"

#include "BlackFlowMovementRecognition.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <utility>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view SelectMovementTrigger = "BlackFlow@Roguelike@SelectMovement";
constexpr std::string_view SelectionAction = "BlackFlow@Roguelike@SelectMovementAction";
constexpr std::string_view InventoryObservationTrigger = "BlackFlow@Roguelike@MovementInventoryObserve";
constexpr std::string_view InventoryObservationAction = "BlackFlow@Roguelike@MovementInventoryObservationAction";
constexpr std::string_view InventoryItemsTask = "BlackFlow@Roguelike@MovementInventoryItems";
constexpr std::string_view InventoryCloseTask = "BlackFlow@Roguelike@MovementInventoryClose";
constexpr std::string_view OpenPanelTask = "BlackFlow@Roguelike@MovementPanelOpenClick";
constexpr std::string_view PanelTitleTask = "BlackFlow@Roguelike@MovementPanelTitle";
constexpr std::string_view PanelItemsTask = "BlackFlow@Roguelike@MovementPanelItems";
constexpr std::string_view SwipePanelTask = "BlackFlow@Roguelike@MovementPanelSwipe";
constexpr std::string_view ClosePanelTask = "BlackFlow@Roguelike@MovementPanelBackClick";
constexpr std::string_view PanelTitle = "选择移动方式";
constexpr std::string_view LoadedText = "装载中";

constexpr int MaxOpenAttempts = 3;
constexpr int MaxFrameRecognitionAttempts = 2;
constexpr int MaxForwardSwipes = 3;
constexpr int LoadedMarkerMaximumGap = 32;
constexpr int SelectionSettleDelay = 500;
constexpr int RecognitionRetryDelay = 200;
constexpr int InventoryLoadedMarkerMaximumVerticalGap = 32;

void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}

int vertical_center(const Rect& rect) noexcept
{
    return rect.y + rect.height / 2;
}
} // namespace

bool BlackFlowMovementTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    if (task == SelectMovementTrigger) {
        m_pending = PendingWork::SelectMovement;
        return true;
    }
    if (task == InventoryObservationTrigger) {
        m_pending = PendingWork::ObserveInventory;
        return true;
    }
    return false;
}

void BlackFlowMovementTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
}

bool BlackFlowMovementTaskPlugin::_run()
{
    LogTraceFunction;
    const PendingWork work = m_pending;
    m_pending = PendingWork::None;
    if (work == PendingWork::None) {
        return true;
    }
    if (work == PendingWork::ObserveInventory) {
        return observe_inventory();
    }

    Task.set_task_base(std::string(SelectionAction), "BlackFlow@Roguelike@RecoveryFailed");

    if (m_session == nullptr || !m_session->pending_candidate().has_value()) {
        Log.error("BlackFlow movement selection has no pending route candidate");
        return true;
    }

    const MovementKind target = m_session->pending_candidate()->candidate.movement;
    const MovementSpec* target_spec = find_movement_spec(target);
    std::string error;
    const SelectionOutcome outcome = select_movement(target, &error);
    if (outcome == SelectionOutcome::Selected) {
        Task.set_task_base(std::string(SelectionAction), "BlackFlow@Roguelike@RoutingResume");
        Log.info("BlackFlow movement selected", target_spec == nullptr ? std::string_view("unknown") : target_spec->id);
    }
    else if (outcome == SelectionOutcome::Unavailable) {
        Task.set_task_base(std::string(SelectionAction), "BlackFlow@Roguelike@MapPrepare");
        Log.info(
            "BlackFlow movement unavailable; route will be replanned",
            target_spec == nullptr ? std::string_view("unknown") : target_spec->id);
    }
    else {
        Log.error("BlackFlow movement selection failed", error);
    }
    report_outputs();
    return true;
}

bool BlackFlowMovementTaskPlugin::observe_inventory()
{
    Task.set_task_base(std::string(InventoryObservationAction), std::string(InventoryCloseTask));
    if (m_session == nullptr) {
        Log.error("BlackFlow movement inventory observation has no active session");
        return true;
    }

    InventoryFrame frame;
    std::string error;
    if (!scan_inventory_frame(frame, &error) ||
        !m_session->apply_movement_inventory_observation(frame.movements, &error)) {
        m_session->fail(
            "movement_inventory_observation_failed",
            error.empty() ? "movement inventory OCR failed" : error,
            FailureDisposition::StopTask);
        Log.error("BlackFlow movement inventory observation failed", error);
        report_outputs();
        return true;
    }

    const MovementSpec* loaded =
        frame.loaded_movement.has_value() ? find_movement_spec(*frame.loaded_movement) : nullptr;
    Log.info(
        "BlackFlow movement inventory observed",
        "visible items",
        frame.movements.size(),
        "loaded marker",
        loaded == nullptr ? std::string_view("none") : loaded->id);
    report_outputs();
    return true;
}

bool BlackFlowMovementTaskPlugin::scan_inventory_frame(InventoryFrame& frame, std::string* error) const
{
    int no_candidate_frames = 0;
    std::string latest_error;
    for (int attempt = 0; attempt < MaxFrameRecognitionAttempts; ++attempt) {
        InventoryFrame candidate;
        const InventoryAnalysisOutcome outcome =
            analyze_inventory_frame(ctrler()->get_image(), candidate, &latest_error);
        if (outcome == InventoryAnalysisOutcome::Recognized) {
            frame = std::move(candidate);
            return true;
        }
        if (outcome == InventoryAnalysisOutcome::NoCandidate) {
            ++no_candidate_frames;
        }
        if (attempt + 1 < MaxFrameRecognitionAttempts) {
            sleep(RecognitionRetryDelay);
        }
    }
    if (no_candidate_frames == MaxFrameRecognitionAttempts) {
        frame = InventoryFrame {};
        return true;
    }
    set_error(error, latest_error.empty() ? "movement inventory OCR failed" : latest_error);
    return false;
}

BlackFlowMovementTaskPlugin::InventoryAnalysisOutcome BlackFlowMovementTaskPlugin::analyze_inventory_frame(
    const cv::Mat& image,
    InventoryFrame& frame,
    std::string* error) const
{
    if (image.empty()) {
        set_error(error, "movement inventory screenshot is empty");
        return InventoryAnalysisOutcome::Failed;
    }

    const auto task = Task.get<OcrTaskInfo>(std::string(InventoryItemsTask));
    if (task == nullptr) {
        set_error(error, "movement inventory OCR task is missing");
        return InventoryAnalysisOutcome::Failed;
    }

    OCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_required(inventory_ocr_candidates());
    const auto results = analyzer.analyze();
    if (!results.has_value()) {
        return InventoryAnalysisOutcome::NoCandidate;
    }

    std::vector<InventoryItem> items;
    std::vector<Rect> loaded_markers;
    for (const auto& result : *results) {
        if (const MovementSpec* movement = movement_from_name(result.text);
            movement != nullptr && movement->kind != MovementKind::Walk) {
            frame.movements.emplace(movement->kind);
            items.emplace_back(InventoryItem { movement->kind, result.rect });
        }
        else if (result.text == LoadedText) {
            loaded_markers.emplace_back(result.rect);
        }
    }

    int best_horizontal_gap = std::numeric_limits<int>::max();
    for (const Rect& marker : loaded_markers) {
        const int marker_center = vertical_center(marker);
        for (const InventoryItem& item : items) {
            if (std::abs(vertical_center(item.name_rect) - marker_center) > InventoryLoadedMarkerMaximumVerticalGap) {
                continue;
            }
            const int horizontal_gap = marker.x - (item.name_rect.x + item.name_rect.width);
            if (horizontal_gap >= 0 && horizontal_gap < best_horizontal_gap) {
                frame.loaded_movement = item.movement;
                best_horizontal_gap = horizontal_gap;
            }
        }
    }
    if (!loaded_markers.empty() && !frame.loaded_movement.has_value()) {
        Log.warn("BlackFlow movement inventory loaded marker has no recognized item on its left");
    }
    return InventoryAnalysisOutcome::Recognized;
}

BlackFlowMovementTaskPlugin::SelectionOutcome
    BlackFlowMovementTaskPlugin::select_movement(MovementKind target, std::string* error)
{
    if (find_movement_spec(target) == nullptr) {
        set_error(error, "movement selection requested an unknown movement kind");
        return SelectionOutcome::Failed;
    }

    if (const auto loaded = recognize_loaded_movement(ctrler()->get_image()); loaded.has_value()) {
        MovementPanelObservation current;
        current.target = *loaded;
        current.target_found = true;
        if (!m_session->apply_movement_panel_observation(std::move(current), *loaded, error)) {
            return SelectionOutcome::Failed;
        }
        if (*loaded == target) {
            return SelectionOutcome::Selected;
        }
    }
    if (!ensure_panel_open(error)) {
        return SelectionOutcome::Failed;
    }

    for (int swipe_count = 0; swipe_count <= MaxForwardSwipes; ++swipe_count) {
        PanelFrame frame;
        if (!scan_current_frame(frame, error)) {
            close_panel(nullptr);
            return SelectionOutcome::Failed;
        }

        const auto target_item = std::find_if(frame.items.begin(), frame.items.end(), [target](const PanelItem& item) {
            return item.movement == target;
        });
        if (target_item != frame.items.end()) {
            if (target_item->loaded) {
                if (!report_target_observation(target, *target_item, swipe_count, target, error)) {
                    close_panel(nullptr);
                    return SelectionOutcome::Failed;
                }
                return close_panel(error) ? SelectionOutcome::Selected : SelectionOutcome::Failed;
            }

            ctrler()->click(target_item->name_rect);
            sleep(SelectionSettleDelay);

            PanelFrame verification;
            if (!scan_current_frame(verification, error)) {
                close_panel(nullptr);
                return SelectionOutcome::Failed;
            }
            const auto verified_item =
                std::find_if(verification.items.begin(), verification.items.end(), [target](const PanelItem& item) {
                    return item.movement == target;
                });
            if (verified_item == verification.items.end() || !verified_item->loaded ||
                verification.loaded_movement != target) {
                set_error(error, "movement selection did not move the loaded marker to the requested item");
                close_panel(nullptr);
                return SelectionOutcome::Failed;
            }
            if (!report_target_observation(target, *verified_item, swipe_count, target, error)) {
                close_panel(nullptr);
                return SelectionOutcome::Failed;
            }
            return close_panel(error) ? SelectionOutcome::Selected : SelectionOutcome::Failed;
        }

        if (swipe_count < MaxForwardSwipes && !run_fixed_task(SwipePanelTask)) {
            set_error(error, "movement panel could not advance to the next group of items");
            close_panel(nullptr);
            return SelectionOutcome::Failed;
        }
    }

    if (target == MovementKind::Walk) {
        set_error(error, "movement panel full scan did not recognize walking");
        close_panel(nullptr);
        return SelectionOutcome::Failed;
    }

    std::string report_error;
    const bool unavailable_reported = m_session->report_movement_unavailable(target, &report_error);
    const bool panel_closed = close_panel(&report_error);
    if (!unavailable_reported || !panel_closed) {
        set_error(
            error,
            report_error.empty() ? "movement absence could not be applied to the current session" : report_error);
        return SelectionOutcome::Failed;
    }
    return SelectionOutcome::Unavailable;
}

bool BlackFlowMovementTaskPlugin::ensure_panel_open(std::string* error)
{
    if (title_visible(ctrler()->get_image())) {
        return true;
    }
    for (int attempt = 0; attempt < MaxOpenAttempts; ++attempt) {
        if (!run_fixed_task(OpenPanelTask)) {
            continue;
        }
        if (title_visible(ctrler()->get_image())) {
            return true;
        }
    }
    set_error(error, "movement panel title did not appear after the open action");
    return false;
}

bool BlackFlowMovementTaskPlugin::close_panel(std::string* error)
{
    if (!title_visible(ctrler()->get_image())) {
        return true;
    }
    for (int attempt = 0; attempt < MaxOpenAttempts; ++attempt) {
        if (!run_fixed_task(ClosePanelTask)) {
            continue;
        }
        if (!title_visible(ctrler()->get_image())) {
            return true;
        }
    }
    set_error(error, "movement panel title remained visible after the close action");
    return false;
}

bool BlackFlowMovementTaskPlugin::title_visible(const cv::Mat& image) const
{
    const auto task = Task.get<OcrTaskInfo>(std::string(PanelTitleTask));
    if (task == nullptr) {
        Log.error("BlackFlow movement panel title OCR task is missing", PanelTitleTask);
        return false;
    }
    OCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_required({ std::string(PanelTitle) });
    const auto result = analyzer.analyze();
    return result.has_value() && !result->empty();
}

bool BlackFlowMovementTaskPlugin::scan_current_frame(PanelFrame& frame, std::string* error) const
{
    std::string latest_error;
    for (int attempt = 0; attempt < MaxFrameRecognitionAttempts; ++attempt) {
        PanelFrame candidate;
        if (analyze_frame(ctrler()->get_image(), candidate, &latest_error)) {
            frame = std::move(candidate);
            return true;
        }
        if (attempt + 1 < MaxFrameRecognitionAttempts) {
            sleep(RecognitionRetryDelay);
        }
    }
    set_error(
        error,
        latest_error.empty() ? "movement panel item recognition returned no movement names" : latest_error);
    return false;
}

bool BlackFlowMovementTaskPlugin::analyze_frame(const cv::Mat& image, PanelFrame& frame, std::string* error) const
{
    const auto task = Task.get<OcrTaskInfo>(std::string(PanelItemsTask));
    if (task == nullptr) {
        set_error(error, "movement panel item OCR task is missing");
        return false;
    }

    OCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_required(ocr_candidates());
    const auto results = analyzer.analyze();
    if (!results.has_value()) {
        set_error(error, "movement panel item OCR failed");
        return false;
    }

    std::vector<Rect> loaded_markers;
    std::vector<std::pair<Rect, int>> remaining_markers;
    for (const auto& result : *results) {
        if (const MovementSpec* movement = movement_from_name(result.text); movement != nullptr) {
            const auto existing =
                std::find_if(frame.items.begin(), frame.items.end(), [movement](const PanelItem& item) {
                    return item.movement == movement->kind;
                });
            if (existing == frame.items.end()) {
                frame.items.emplace_back(
                    PanelItem {
                        movement->kind,
                        result.rect,
                        result.score,
                        std::nullopt,
                        false,
                    });
            }
            else {
                ++existing->name_match_count;
                if (result.score > existing->name_score) {
                    existing->name_rect = result.rect;
                    existing->name_score = result.score;
                }
            }
            continue;
        }
        if (result.text == LoadedText) {
            loaded_markers.emplace_back(result.rect);
            continue;
        }
        if (const auto remaining = remaining_uses_from_text(result.text); remaining.has_value()) {
            remaining_markers.emplace_back(result.rect, *remaining);
        }
    }

    if (frame.items.empty()) {
        set_error(error, "movement panel item OCR found no recognized movement names");
        return false;
    }

    std::sort(frame.items.begin(), frame.items.end(), [](const PanelItem& left, const PanelItem& right) {
        const int left_center = vertical_center(left.name_rect);
        const int right_center = vertical_center(right.name_rect);
        return left_center == right_center ? left.name_rect.x < right.name_rect.x : left_center < right_center;
    });

    PanelItem* loaded_item = nullptr;
    int loaded_gap = std::numeric_limits<int>::max();
    for (const Rect& marker : loaded_markers) {
        const int marker_bottom = marker.y + marker.height;
        for (auto& item : frame.items) {
            const int gap = item.name_rect.y - marker_bottom;
            if (gap >= 0 && gap <= LoadedMarkerMaximumGap && gap < loaded_gap) {
                loaded_item = &item;
                loaded_gap = gap;
            }
        }
    }
    if (loaded_item != nullptr) {
        loaded_item->loaded = true;
        frame.loaded_movement = loaded_item->movement;
    }

    const int roi_top = task->roi.y;
    const int roi_bottom = task->roi.y + task->roi.height;
    for (std::size_t index = 0; index < frame.items.size(); ++index) {
        const int current_center = vertical_center(frame.items[index].name_rect);
        const int interval_top =
            index == 0 ? roi_top : (vertical_center(frame.items[index - 1].name_rect) + current_center) / 2;
        const int interval_bottom = index + 1 == frame.items.size()
                                        ? roi_bottom
                                        : (current_center + vertical_center(frame.items[index + 1].name_rect)) / 2;

        int best_distance = std::numeric_limits<int>::max();
        for (const auto& [marker, remaining] : remaining_markers) {
            const int marker_center = vertical_center(marker);
            if (marker_center < interval_top || marker_center >= interval_bottom) {
                continue;
            }
            ++frame.items[index].remaining_match_count;
            const int distance = std::abs(marker_center - current_center);
            if (distance < best_distance) {
                frame.items[index].remaining_uses = remaining;
                best_distance = distance;
            }
        }
    }
    return true;
}

bool BlackFlowMovementTaskPlugin::run_fixed_task(std::string_view task)
{
    return ProcessTask(*this, { std::string(task) }).set_retry_times(0).run();
}

bool BlackFlowMovementTaskPlugin::report_target_observation(
    MovementKind target,
    const PanelItem& item,
    int completed_swipes,
    std::optional<MovementKind> active_movement,
    std::string* error)
{
    if (m_session == nullptr) {
        set_error(error, "movement panel observation has no active session");
        return false;
    }
    MovementPanelObservation panel;
    panel.target = target;
    panel.completed_swipes = completed_swipes;
    panel.target_found = true;
    panel.unique_record = item.name_match_count == 1 && item.remaining_match_count == 1;
    panel.remaining_charges = item.remaining_uses;
    return m_session->apply_movement_panel_observation(std::move(panel), active_movement, error);
}

const MovementSpec* BlackFlowMovementTaskPlugin::movement_from_name(std::string_view name) noexcept
{
    const auto movement =
        std::find_if(movement_specs().begin(), movement_specs().end(), [name](const MovementSpec& spec) {
            return spec.name == name;
        });
    return movement == movement_specs().end() ? nullptr : &*movement;
}

std::optional<int> BlackFlowMovementTaskPlugin::remaining_uses_from_text(std::string_view text) noexcept
{
    if (text == "剩余1次") {
        return 1;
    }
    if (text == "剩余2次") {
        return 2;
    }
    if (text == "剩余3次") {
        return 3;
    }
    return std::nullopt;
}

std::vector<std::string> BlackFlowMovementTaskPlugin::ocr_candidates()
{
    std::vector<std::string> candidates;
    candidates.reserve(movement_specs().size() + 4);
    for (const MovementSpec& movement : movement_specs()) {
        candidates.emplace_back(movement.name);
    }
    candidates.emplace_back(LoadedText);
    candidates.emplace_back("剩余1次");
    candidates.emplace_back("剩余2次");
    candidates.emplace_back("剩余3次");
    return candidates;
}

std::vector<std::string> BlackFlowMovementTaskPlugin::inventory_ocr_candidates()
{
    std::vector<std::string> candidates;
    candidates.reserve(movement_specs().size());
    for (const MovementSpec& movement : movement_specs()) {
        if (movement.kind != MovementKind::Walk) {
            candidates.emplace_back(movement.name);
        }
    }
    candidates.emplace_back(LoadedText);
    return candidates;
}
} // namespace asst::blackflow
