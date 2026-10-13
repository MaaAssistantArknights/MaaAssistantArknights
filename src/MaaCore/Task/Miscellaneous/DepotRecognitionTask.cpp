#include "DepotRecognitionTask.h"
#include "DepotScanGuard.h"

#include <future>

#include "Common/DepotQuantity.h"
#include "Common/DepotTokenBoundary.h"
#include "MaaUtils/NoWarningCV.hpp"
#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/OCRer.h"

bool asst::DepotRecognitionTask::_run()
{
    LogTraceFunction;

    m_invalid_templates = false;
    m_scan_verified = false;
    bool ret = swipe_and_analyze();

    // 材料页扫完后，切到「全部」标签页识别基础物品（源石、合成玉、龙门币、赤金、采购凭证）
    if (!m_invalid_templates && (!m_strict_scan || ret)) {
        ret &= analyze_basic_items();
    }

    if (m_invalid_templates) {
        DepotImageAnalyzer::clear_cached_templates();
        return false;
    }

    m_scan_verified &= ret;
    callback_analyze_result(true);

    return ret;
}

bool asst::DepotRecognitionTask::analyze_basic_items()
{
    LogTraceFunction;

    // 识别并点击「全部」标签页（此时在材料页，「全部」为白色可选状态）
    Matcher all_tab_matcher(ctrler()->get_image());
    all_tab_matcher.set_task_info("DepotAllTab");
    auto all_tab_result = all_tab_matcher.analyze();
    if (!all_tab_result) {
        Log.error(__FUNCTION__, "failed to match DepotAllTab");
        return false;
    }
    ctrler()->click(all_tab_result->rect);
    sleep(500);

    const auto basic_frame = ctrler()->get_image();
    DepotImageAnalyzer analyzer(basic_frame);
    analyzer.set_item_ids({ "4002", "4003", "4001", "3003", "4006" }); // 源石 合成玉 龙门币 赤金 采购凭证（红票）
    analyzer.set_is_basic(true);
    const bool analyzed = analyzer.analyze();
    if (report_invalid_templates(analyzer) || !analyzed) {
        return false;
    }

    auto result = analyzer.get_result();
    if (m_strict_scan) {
        auto lmd = result.find("4001");
        if (lmd != result.end() && !lmd->second.quantity_verified && !read_lmd_detail(lmd->second, basic_frame)) {
            // A lost/cancelled page cannot authorize any inventory farming.
            return false;
        }
    }
    for (const auto& [item_id, item_info] : result) {
        m_all_items.emplace(item_id, item_info);
    }

    DepotImageAnalyzer::clear_cached_templates();
    callback_analyze_result(false);
    return true;
}

bool asst::DepotRecognitionTask::read_lmd_detail(ItemInfo& item, const cv::Mat& grid_frame)
{
    LogTraceFunction;
    const auto& rect = item.rect;
    if (item.item_id != "4001" || grid_frame.cols != 1280 || grid_frame.rows != 720 || rect.x < 0 || rect.y < 90 ||
        rect.width <= 0 || rect.height <= 0 || rect.x + rect.width > 1280 || rect.y + rect.height > 640 ||
        need_exit()) {
        return false;
    }
    const auto read = [](const cv::Mat& frame, const Rect& roi) {
        OCRer ocr(frame, roi);
        ocr.set_use_char_model(false);
        const auto results = ocr.analyze();
        if (!results || results->size() != 1 || results->front().score < 0.93) {
            return std::string {};
        }
        return results->front().text;
    };
    // Calibrated from the native LMD card, not inferred from the rounded tile.
    ctrler()->click(rect);
    std::optional<int> exact;
    bool verified = true;
    for (int sample = 0; sample < 2; ++sample) {
        if (!sleep(sample == 0 ? 650 : 300) || need_exit()) {
            return false;
        }
        const auto frame = ctrler()->get_image();
        if (frame.size() != grid_frame.size() || frame.type() != grid_frame.type()) {
            verified = false;
            break;
        }
        const auto quantity = depot_quantity::lmd_detail(
            read(frame, { 225, 168, 250, 48 }),
            read(frame, { 865, 170, 75, 40 }),
            read(frame, { 940, 168, 120, 48 }));
        if (!quantity || (sample == 1 && quantity != exact)) {
            verified = false;
            break;
        }
        exact = quantity;
    }
    if (need_exit()) {
        return false;
    }
    // Dismiss only through the empty top area; never click acquisition links.
    ctrler()->click(Rect(635, 75, 10, 10));
    if (!sleep(350)) {
        return false;
    }
    const auto restored = ctrler()->get_image();
    if (restored.size() != grid_frame.size() || restored.type() != grid_frame.type()) {
        return false;
    }
    const cv::Rect grid(0, 144, 1254, 480);
    const bool restored_same = cv::norm(restored(grid), grid_frame(grid), cv::NORM_L1) /
                                   (static_cast<double>(grid.area()) * restored.channels()) <=
                               1.0;
    auto info = basic_info_with_what("DepotExactDetail");
    info["details"]["item_id"] = "4001";
    info["details"]["verified"] = verified && restored_same && exact.has_value();
    info["details"]["restored_same_grid"] = restored_same;
    if (verified && restored_same && exact) {
        item.quantity = *exact;
        item.quantity_verified = true;
        info["details"]["quantity"] = *exact;
    }
    callback(AsstMsg::SubTaskExtraInfo, info);
    return restored_same;
}

bool asst::DepotRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    if (m_strict_scan) {
        return strict_swipe_and_analyze();
    }
    m_all_items.clear();

    size_t pre_pos = 0ULL;
    while (true) {
        DepotImageAnalyzer analyzer(ctrler()->get_image());

        auto future = std::async(std::launch::async, [&]() { swipe(); });

        // 因为滑动不是完整的一页，有可能上一次识别过的物品，这次仍然在页面中
        // 所以这个 begin pos 不能设置
        // analyzer.set_match_begin_pos(pre_pos);
        const bool analyzed = analyzer.analyze();
        if (report_invalid_templates(analyzer) || !analyzed) {
            break;
        }
        size_t cur_pos = analyzer.get_match_begin_pos();
        if (cur_pos == pre_pos || cur_pos == DepotImageAnalyzer::NPos) {
            break;
        }
        pre_pos = cur_pos;

        auto cur_result = analyzer.get_result();
        m_all_items.merge(std::move(cur_result));

        future.wait();
        callback_analyze_result(false);
    }
    DepotImageAnalyzer::clear_cached_templates();
    return !m_all_items.empty();
}

bool asst::DepotRecognitionTask::strict_swipe_and_analyze()
{
    LogTraceFunction;
    m_all_items.clear();
    DepotScanGuard guard;
    cv::Mat previous_page;
    // Ignore tabs, counters and the bottom toolbar; only the material grid
    // establishes stability/end-of-list, not an animated unrelated UI label.
    const auto unchanged = [](const cv::Mat& a, const cv::Mat& b) {
        if (a.empty() || b.empty() || a.size() != b.size() || a.type() != b.type()) {
            return false;
        }
        const cv::Rect grid(0, a.rows / 5, a.cols * 49 / 50, a.rows * 2 / 3);
        return cv::norm(a(grid), b(grid), cv::NORM_L1) / (static_cast<double>(grid.area()) * a.channels()) <= 1.0;
    };
    for (size_t page = 1; page <= DepotScanGuard::MaxPages && !need_exit(); ++page) {
        cv::Mat frame = ctrler()->get_image();
        bool accepted = false;
        for (int attempt = 1; attempt <= 3 && !need_exit(); ++attempt) {
            if (!sleep(300)) {
                break;
            }
            cv::Mat fresh = ctrler()->get_image();
            const bool settled = unchanged(frame, fresh);
            frame = std::move(fresh);
            if (!settled) {
                Log.info("Depot strict page moving", page, "attempt", attempt);
                continue;
            }
            DepotImageAnalyzer analyzer(frame);
            const bool analyzed = analyzer.analyze();
            const bool invalid = report_invalid_templates(analyzer);
            const size_t position = analyzer.get_match_begin_pos();
            const bool same_page = unchanged(previous_page, frame);
            auto info = basic_info_with_what("DepotScanInfo");
            auto& details = info["details"];
            details["page"] = static_cast<int>(page);
            details["attempt"] = attempt;
            details["analyzed"] = analyzed;
            details["position"] = static_cast<int64_t>(position);
            details["page_unchanged"] = same_page;
            details["items"] = static_cast<int>(analyzer.get_result().size());
            // Bounded geometry only: distinguish an unknown item from an
            // out-of-layout token-card refusal without relaxing scan safety.
            details["frame_width"] = frame.cols;
            details["frame_height"] = frame.rows;
            if (const auto& unmatched = analyzer.get_unmatched_rect()) {
                details["unmatched_rect"] =
                    json::array { unmatched->x, unmatched->y, unmatched->width, unmatched->height };
                details["boundary_geometry_admitted"] = depot_token::grid_geometry(
                    frame.cols,
                    frame.rows,
                    unmatched->x,
                    unmatched->y,
                    unmatched->width,
                    unmatched->height);
            }
            callback(AsstMsg::SubTaskExtraInfo, info);
            if (invalid) {
                break;
            }
            const bool token_boundary = analyzer.get_unmatched_rect() &&
                                        (!m_all_items.empty() || !analyzer.get_result().empty()) &&
                                        confirm_operator_token_boundary(*analyzer.get_unmatched_rect(), frame);
            if (analyzer.get_unmatched_rect() && !token_boundary) {
                // Do not skip an unknown owned material on a partial page.
                analyzer.save_img(utils::path("debug") / utils::path("depot-strict"));
                break;
            }
            if (!analyzed) {
                // Retry observation only. Never swipe away from the failed
                // page and never turn an OCR/template failure into EOF.
                if (token_boundary) {
                    m_scan_verified = true;
                    break;
                }
                analyzer.save_img(utils::path("debug") / utils::path("depot-strict"));
                continue;
            }
            const auto decision =
                token_boundary ? DepotScanGuard::Decision::Complete : guard.observe(true, position, same_page);
            if (decision == DepotScanGuard::Decision::Reject) {
                Log.error("Depot strict ordered/end proof rejected", page, position, same_page);
                analyzer.save_img(utils::path("debug") / utils::path("depot-strict"));
                break;
            }
            auto items = analyzer.get_result();
            for (auto& [item_id, item] : items) {
                auto [existing, inserted] = m_all_items.try_emplace(item_id, std::move(item));
                if (!inserted && !depot_quantity::agrees(
                                     existing->second.quantity,
                                     existing->second.quantity_verified,
                                     item.quantity,
                                     item.quantity_verified)) {
                    existing->second.quantity_verified = false;
                    auto conflict = basic_info_with_what("DepotQuantityConflict");
                    conflict["details"]["item_id"] = item_id;
                    conflict["details"]["first_quantity"] = existing->second.quantity;
                    conflict["details"]["observed_quantity"] = item.quantity;
                    callback(AsstMsg::SubTaskExtraInfo, conflict);
                }
            }
            if (decision == DepotScanGuard::Decision::Complete) {
                m_scan_verified = true;
                break;
            }
            previous_page = frame.clone();
            callback_analyze_result(false);
            // Strict mode intentionally does not overlap analysis with input.
            // A failed/unstable frame leaves the device at the diagnostic page.
            accepted = ProcessTask(*this, { "DepotSlowlySwipeToTheRight" }).run();
            break;
        }
        if (m_scan_verified || !accepted) {
            break;
        }
    }
    DepotImageAnalyzer::clear_cached_templates();
    return !m_all_items.empty() && m_scan_verified;
}

bool asst::DepotRecognitionTask::confirm_operator_token_boundary(const Rect& rect, const cv::Mat& grid_frame)
{
    // These ROIs are calibrated against the native 1280x720 Depot detail card.
    // Refuse another layout instead of guessing or touching the storage button.
    if (!depot_token::grid_geometry(grid_frame.cols, grid_frame.rows, rect.x, rect.y, rect.width, rect.height) ||
        need_exit()) {
        return false;
    }
    Matcher tab(grid_frame);
    tab.set_task_info("DepotMaterialTabClicked");
    if (!tab.analyze()) {
        return false;
    }
    const auto read = [](const cv::Mat& frame, const Rect& roi) {
        OCRer ocr(frame, roi);
        ocr.set_use_char_model(false);
        const auto results = ocr.analyze();
        if (!results || results->size() != 1 || results->front().score < 0.93) {
            return std::string {};
        }
        return results->front().text;
    };
    ctrler()->click(rect);
    std::string previous_name;
    bool verified = true;
    for (int sample = 0; sample < 2; ++sample) {
        if (!sleep(sample == 0 ? 650 : 300) || need_exit()) {
            verified = false;
            break;
        }
        const auto frame = ctrler()->get_image();
        const auto name = read(frame, { 225, 168, 250, 48 });
        const auto usage = read(frame, { 485, 230, 550, 35 });
        const auto stock = read(frame, { 865, 170, 75, 40 });
        const auto storage = read(frame, { 292, 455, 128, 38 });
        const auto source = read(frame, { 485, 385, 350, 40 });
        VisionHelper(frame).save_img(utils::path("debug") / utils::path("depot-boundary"));
        if ((!depot_token::detail(name, usage, stock, storage) && !depot_token::detail(name, usage, stock, source)) ||
            (sample == 1 && name != previous_name)) {
            verified = false;
            break;
        }
        previous_name = name;
    }
    // Only dismiss the native read-only popup by clicking the empty top area.
    // Never choose storage, next item, use, purchase or any resource action.
    ctrler()->click(Rect(635, 75, 10, 10));
    if (!sleep(350)) {
        return false;
    }
    const auto restored = ctrler()->get_image();
    Matcher restored_tab(restored);
    restored_tab.set_task_info("DepotMaterialTabClicked");
    if (!restored_tab.analyze() || restored.size() != grid_frame.size() || restored.type() != grid_frame.type()) {
        return false;
    }
    const cv::Rect grid(0, 144, 1254, 480);
    const bool restored_same = cv::norm(restored(grid), grid_frame(grid), cv::NORM_L1) /
                                   (static_cast<double>(grid.area()) * restored.channels()) <=
                               1.0;
    auto info = basic_info_with_what("DepotTokenBoundary");
    info["details"]["verified"] = verified && restored_same;
    info["details"]["name"] = previous_name;
    info["details"]["restored_same_grid"] = restored_same;
    callback(AsstMsg::SubTaskExtraInfo, info);
    return verified && restored_same;
}

bool asst::DepotRecognitionTask::report_invalid_templates(const DepotImageAnalyzer& analyzer)
{
    const auto& item_ids = analyzer.get_invalid_template_ids();
    if (item_ids.empty()) {
        return false;
    }

    json::value info = basic_info_with_what("DepotTemplateLoadError");
    info["details"]["item_ids"] = json::array(item_ids);
    callback(AsstMsg::SubTaskError, info);
    m_invalid_templates = true;
    return true;
}

void asst::DepotRecognitionTask::callback_analyze_result(bool done)
{
    LogTraceFunction;

    json::value info = basic_info_with_what("DepotInfo");
    auto& details = info["details"];

    // 简化格式：只保留 done 和 data
    // data 为 {"itemId": count} 格式
    json::object data_obj;
    json::object verified_obj;
    for (const auto& [item_id, item_info] : m_all_items) {
        data_obj.emplace(item_id, item_info.quantity);
        if (item_info.quantity_verified) {
            verified_obj.emplace(item_id, item_info.quantity);
        }
    }

    details["done"] = done;
    if (m_strict_scan) {
        details["strict_material_scan_verified"] = done && m_scan_verified;
    }
    details["data"] = data_obj.to_string();
    // Keep the old display/cache payload compatible. A fresh quota decision
    // consumes this separate strict set: OCR failure and abbreviations are
    // not exact zero/stock, even when the legacy display has a numeric value.
    details["verified_data"] = verified_obj.to_string();

    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::DepotRecognitionTask::swipe()
{
    ProcessTask(*this, { "DepotSlowlySwipeToTheRight" }).run();
}
