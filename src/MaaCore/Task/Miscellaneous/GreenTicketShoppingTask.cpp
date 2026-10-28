#include "GreenTicketShoppingTask.h"

#include <algorithm>
#include <ranges>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/FeatureMatcher.h"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"
#include "Vision/OCRer.h"

asst::GreenTicketShoppingTask::GreenTicketShoppingTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain)
{
    // 整次购买不可重跑；付款后的等待由收据节点内部重试完成。
    m_retry_times = 0;
}

bool asst::GreenTicketShoppingTask::_run()
{
    const auto& item_id = m_item_id;
    const int count = m_count;
    constexpr int MaxPurchaseCount = 99;
    constexpr int MaxSwipes = 8;
    if (need_exit() || count <= 0 || count > MaxPurchaseCount || ItemData.get_item_rarity(item_id) != 2 ||
        !run_task("GreenTicket@Store@ReplenishStoreBegin") ||
        !run_task("GreenTicket@Store@ReplenishChooseTicketType")) {
        return false;
    }

    bool found = false;
    for (int swipe = 0; swipe <= MaxSwipes && !need_exit(); ++swipe) {
        FeatureMatcher matcher(ctrler()->get_image());
        matcher.set_task_info("GreenTicket@Store@ClickItem");
        matcher.set_templ(item_id);
        const auto matches = matcher.analyze();
        const auto& roi = Task.get("GreenTicket@Store@ClickItem")->roi;
        const auto complete_match = [&](const FeatureMatchRect& match) {
            const auto& rect = match.rect;
            return rect.x >= roi.x && rect.y >= roi.y && rect.x + rect.width <= roi.x + roi.width &&
                   rect.y + rect.height <= roi.y + roi.height;
        };
        if (matches && std::ranges::any_of(*matches, complete_match)) {
            const auto& match = *std::ranges::find_if(*matches, complete_match);
            if (!ctrler()->click(match.rect) || !run_task("GreenTicket@Store@ReplenishPurchasePage", 3)) {
                return false;
            }
            found = true;
            break;
        }
        if (swipe < MaxSwipes && !run_task("RedTicket@Store@Swipe")) {
            return false;
        }
    }
    if (!found || need_exit()) {
        LogWarn << __FUNCTION__ << "material unavailable in the green ticket store" << item_id;
        return false;
    }

    const auto recognize_name = [&](const cv::Mat& image) {
        OCRer analyzer(image);
        analyzer.set_task_info("GreenTicket@Store@ReplenishPurchaseName");
        analyzer.set_required({ ItemData.get_item_name(item_id) });
        const auto names = analyzer.analyze();
        return names && names->size() == 1 && names->front().score >= 0.9;
    };
    const auto initial_image = ctrler()->get_image();
    if (!recognize_name(initial_image) ||
        MaterialSynthesisImageAnalyzer::read_number(initial_image, "GreenTicket@Store@ReplenishPurchaseQuantity") !=
            1) {
        return false;
    }
    // 仅逐个增加至本次缺口，付款前复核实际数量，库存或余额导致数量被钳制时不付款。
    for (int selected = 1; selected < count; ++selected) {
        if (need_exit() || !run_task("Store@Increase")) {
            return false;
        }
    }
    const auto image = ctrler()->get_image();
    const auto selected =
        MaterialSynthesisImageAnalyzer::read_number(image, "GreenTicket@Store@ReplenishPurchaseQuantity");
    const auto price =
        MaterialSynthesisImageAnalyzer::read_number(image, "GreenTicket@Store@ReplenishPurchaseTotalPrice");
    const auto balance =
        MaterialSynthesisImageAnalyzer::read_number(image, "GreenTicket@Store@ReplenishPurchaseBalance");
    if (need_exit() || !recognize_name(image) || !selected || *selected != count || !price || *price <= 0 || !balance ||
        *balance < *price) {
        LogWarn << __FUNCTION__ << "purchase identity, quantity, price or balance could not be confirmed" << item_id;
        return false;
    }
    // 保留默认识别重试以等待收据；付款节点只执行一次，任务链不会回到付款节点。
    if (!run_task("GreenTicket@Store@ReplenishPurchase")) {
        return false;
    }
    auto info = basic_info_with_what("GreenTicketPurchased");
    info["details"] = json::object {
        { "material", ItemData.get_item_name(item_id) },
        { "material_id", item_id },
        { "count", count },
        { "cost", *price },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

bool asst::GreenTicketShoppingTask::run_task(const std::string& name, int retry_times)
{
    return ProcessTask(*this, { name }).set_retry_times(retry_times).run();
}
