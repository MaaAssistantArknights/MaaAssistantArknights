#include "DepotRecognitionTask.h"

#include <algorithm>

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

    bool ret = swipe_and_analyze();

    // 材料页扫完后，切到「全部」标签页识别基础物品（源石、合成玉、龙门币、赤金、采购凭证）
    if (!need_exit()) {
        ret &= analyze_basic_items();
    }
    ret &= !need_exit();

    callback_analyze_result(true, ret);

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
        LogError << "Failed to match DepotAllTab";
        return false;
    }
    ctrler()->click(all_tab_result->rect);
    sleep(500);

    const auto image = ctrler()->get_image();
    Matcher material_tab(image);
    material_tab.set_task_info("DepotMaterialTab");
    if (!material_tab.analyze()) {
        LogWarn << "Depot basic item page was not confirmed";
        return false;
    }
    DepotImageAnalyzer analyzer(image);
    analyzer.set_item_ids({ "4002", "4003", "4001", "3003", "4006" }); // 源石 合成玉 龙门币 赤金 采购凭证（红票）
    analyzer.set_is_basic(true);
    const bool analyzed = analyzer.analyze();
    DepotImageAnalyzer::clear_cached_templates();
    if (!analyzed) {
        return false;
    }

    const auto& result = analyzer.get_result();
    bool complete = analyzer.is_quantity_recognition_complete();
    for (const auto& item_id : { "4002", "4003", "4001", "3003", "4006" }) {
        if (!result.contains(item_id)) {
            LogWarn << "Required depot basic item was not observed" << VAR(item_id);
            complete = false;
        }
    }
    for (const auto& [item_id, item_info] : result) {
        m_all_items.emplace(item_id, item_info);
    }

    callback_analyze_result(false);
    return complete;
}

bool asst::DepotRecognitionTask::swipe_and_analyze()
{
    LogTraceFunction;
    m_all_items.clear();

    // 重新切换筛选页，避免从上次停止的材料列表中段开始识别。
    if (!ProcessTask(*this, { "DepotAllTab" }).run()) {
        LogWarn << "Failed to reset depot material page";
        return false;
    }

    constexpr size_t max_pages = 100;
    size_t pre_pos = DepotImageAnalyzer::NPos;
    bool completed = false;
    for (size_t page = 0; page < max_pages && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        Matcher material_tab(image);
        material_tab.set_task_info("DepotMaterialTabClicked");
        if (!material_tab.analyze()) {
            LogWarn << "Depot material page was not confirmed" << VAR(page);
            break;
        }
        DepotImageAnalyzer analyzer(image);

        // 因为滑动不是完整的一页，有可能上一次识别过的物品，这次仍然在页面中
        // 所以这个 begin pos 不能设置
        // analyzer.set_match_begin_pos(pre_pos);
        const bool analyzed = analyzer.analyze();
        if (!analyzed || need_exit()) {
            LogWarn << "Depot page recognition failed" << VAR(page);
            break;
        }

        auto cur_result = analyzer.get_result();
        m_all_items.merge(std::move(cur_result));
        callback_analyze_result(false);

        if (!analyzer.is_quantity_recognition_complete()) {
            LogWarn << "Depot page contains an unrecognized quantity" << VAR(page);
            break;
        }
        if (analyzer.has_reached_last_item()) {
            completed = true;
            break;
        }
        if (const auto& unrecognized = analyzer.get_unrecognized_item_rect(); unrecognized) {
            // 信物在所有已支持的培养材料之后，但没有收录在物品模板字典中。
            // 未知槽位本身不能证明结束，必须打开详情并确认标题。
            if (!ctrler()->click(*unrecognized) || !sleep(500)) {
                break;
            }
            OCRer token_detail(ctrler()->get_image());
            token_detail.set_task_info("DepotTokenDetail");
            if (!token_detail.analyze() || !std::ranges::any_of(token_detail.get_result(), [](const TextRect& text) {
                    return text.score >= 0.9;
                })) {
                LogWarn << "Depot unrecognized item is not a confirmed token" << VAR(page);
                break;
            }
            completed = ProcessTask(*this, { "DepotTokenDetail" }).set_retry_times(0).run();
            break;
        }

        const size_t cur_pos = analyzer.get_match_begin_pos();
        if (cur_pos == DepotImageAnalyzer::NPos || (pre_pos != DepotImageAnalyzer::NPos && cur_pos <= pre_pos)) {
            LogWarn << "Depot page did not advance" << VAR(cur_pos) << VAR(pre_pos);
            break;
        }
        pre_pos = cur_pos;
        if (!swipe()) {
            LogWarn << "Depot swipe failed" << VAR(page);
            break;
        }
    }
    DepotImageAnalyzer::clear_cached_templates();
    if (!completed) {
        LogWarn << "Depot recognition did not reach the last page";
    }
    return completed && !need_exit();
}

void asst::DepotRecognitionTask::callback_analyze_result(bool done, bool success)
{
    LogTraceFunction;

    json::value info = basic_info_with_what("DepotInfo");
    auto& details = info["details"];

    // 最终回调区分识别结束与完整识别成功。
    // data 为 {"itemId": count} 格式
    json::object data_obj;
    for (const auto& [item_id, item_info] : m_all_items) {
        data_obj.emplace(item_id, item_info.quantity);
    }

    details["done"] = done;
    if (done) {
        details["success"] = success;
    }
    details["data"] = data_obj.to_string();

    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::DepotRecognitionTask::swipe()
{
    return ProcessTask(*this, { "DepotSlowlySwipeToTheRight" }).run();
}
