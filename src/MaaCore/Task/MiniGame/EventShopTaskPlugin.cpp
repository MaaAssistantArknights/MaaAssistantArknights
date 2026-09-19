#include "EventShopTaskPlugin.h"

#include <algorithm>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"

bool asst::EventShopTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", std::string()) == "SS@Store@Begin";
}

void asst::EventShopTaskPlugin::set_blacklist(std::vector<std::string> blacklist)
{
    m_blacklist = std::move(blacklist);
}

bool asst::EventShopTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_blacklist.empty()) {
        return true;
    }

    int swipe_times = 0;
    int purchase_times = 0;
    while (!need_exit()) {
        const cv::Mat image = ctrler()->get_image();
        if (auto commodity = select_commodity(image)) {
            if (purchase_times >= MaxPurchaseTimes) {
                LogError << "Event shop purchase count exceeded the safety limit:" << MaxPurchaseTimes;
                return false;
            }

            ++purchase_times;
            LogInfo << "Event shop clicking commodity:" << commodity->rect;
            ctrler()->click(commodity->rect);
            if (const auto click_task = Task.get("SS@Store@ClickItem")) {
                sleep(click_task->post_delay);
            }

            switch (purchase_selected_commodity()) {
            case PurchaseResult::Continue:
                continue;
            case PurchaseResult::Finished:
                return true;
            case PurchaseResult::Failed:
            default:
                return false;
            }
        }

        if (swipe_times >= MaxSwipeTimes) {
            LogInfo << "Event shop reached the end after skipping blacklisted commodities";
            return true;
        }
        if (!swipe_store()) {
            LogError << "Event shop failed to swipe";
            return false;
        }
        ++swipe_times;
    }

    return false;
}

std::optional<asst::MatchRect> asst::EventShopTaskPlugin::select_commodity(const cv::Mat& image) const
{
    const auto click_task = Task.get("SS@Store@ClickItem");
    if (!click_task) {
        LogError << "Event shop click task not found";
        return std::nullopt;
    }

    MultiMatcher matcher(image);
    matcher.set_task_info(click_task);
    auto results = matcher.analyze();
    if (!results) {
        return std::nullopt;
    }

    sort_by_horizontal_(*results);
    for (const auto& commodity : *results) {
        auto commodity_name = recognize_commodity_name(image, commodity.rect);
        if (!commodity_name) {
            LogWarn << "Event shop skipping a commodity because its name could not be recognized, button:"
                    << commodity.rect;
            continue;
        }
        if (is_blacklisted(*commodity_name)) {
            LogInfo << "Event shop skipping blacklisted product:" << *commodity_name << "button:" << commodity.rect;
            continue;
        }

        LogInfo << "Event shop selected product:" << *commodity_name << "button:" << commodity.rect;
        return commodity;
    }
    return std::nullopt;
}

std::optional<std::string>
    asst::EventShopTaskPlugin::recognize_commodity_name(const cv::Mat& image, const Rect& commodity) const
{
    const auto name_task = Task.get<OcrTaskInfo>("SS@Store@ProductName");
    if (!name_task) {
        LogError << "Event shop product name OCR task not found";
        return std::nullopt;
    }

    const Rect raw_roi = commodity.move(name_task->rect_move);
    const int left = std::max({ raw_roi.x, name_task->roi.x, 0 });
    const int top = std::max({ raw_roi.y, name_task->roi.y, 0 });
    const int right = std::min({ raw_roi.x + raw_roi.width, name_task->roi.x + name_task->roi.width, image.cols });
    const int bottom = std::min({ raw_roi.y + raw_roi.height, name_task->roi.y + name_task->roi.height, image.rows });
    if (right <= left || bottom <= top) {
        LogError << "Event shop product name OCR region is invalid:" << raw_roi;
        return std::nullopt;
    }

    OCRer analyzer(image);
    analyzer.set_task_info(name_task);
    analyzer.set_roi({ left, top, right - left, bottom - top });
    auto result = analyzer.analyze();
    if (!result || result->empty() || result->front().text.empty()) {
        return std::nullopt;
    }

    return result->front().text;
}

bool asst::EventShopTaskPlugin::is_blacklisted(std::string_view commodity_name) const
{
    const auto& ocr_config = OcrConfig::get_instance();
    const std::string normalized_name = ocr_config.process_equivalence_class(std::string(commodity_name));
    return std::ranges::any_of(m_blacklist, [&](const std::string& item) {
        return normalized_name.find(ocr_config.process_equivalence_class(item)) != std::string::npos;
    });
}

asst::EventShopTaskPlugin::PurchaseResult asst::EventShopTaskPlugin::purchase_selected_commodity() const
{
    Matcher unlimited_analyzer(ctrler()->get_image());
    unlimited_analyzer.set_task_info("SS@Store@CheckUnlimited");
    if (unlimited_analyzer.analyze()) {
        LogInfo << "Event shop reached an unlimited commodity";
        return PurchaseResult::Finished;
    }

    ProcessTask purchase_task(*this, { "SS@Store@ChooseMaxAmount", "SS@Store@Purchase" });
    purchase_task.override_next("SS@Store@PurchasedConfirm", {});
    purchase_task.override_next("SS@Store@RecruitSkipped", {});
    purchase_task.override_next("SS@Store@Underfunded", {});
    if (!purchase_task.run()) {
        LogError << "Event shop purchase flow failed";
        return PurchaseResult::Failed;
    }

    const std::string& last_task = purchase_task.get_last_task_name();
    if (last_task == "SS@Store@UnderfundedOCR" || last_task == "SS@Store@Underfunded" || last_task == "Stop") {
        LogInfo << "Event shop currency is insufficient";
        return PurchaseResult::Finished;
    }
    if (last_task == "SS@Store@PurchasedConfirm" || last_task == "SS@Store@RecruitSkipped") {
        return PurchaseResult::Continue;
    }

    LogError << "Event shop purchase flow ended unexpectedly at:" << last_task;
    return PurchaseResult::Failed;
}

bool asst::EventShopTaskPlugin::swipe_store() const
{
    ProcessTask swipe_task(*this, { "SS@Store@Swipe" });
    if (!swipe_task.override_next("SS@Store@Swipe", {})) {
        return false;
    }
    return swipe_task.set_retry_times(0).run();
}
