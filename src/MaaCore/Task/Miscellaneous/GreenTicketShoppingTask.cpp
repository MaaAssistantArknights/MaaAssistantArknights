#include "GreenTicketShoppingTask.h"

#include <algorithm>
#include <cstdint>
#include <ranges>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/FeatureMatcher.h"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

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
    int count = m_count;
    m_purchased_count = 0;
    constexpr int MaxSwipes = 8;
    const auto limits = Task.get("GreenTicket@Store@ReplenishItem-" + item_id);
    const auto click_task = Task.get<FeatureMatchTaskInfo>(
        item_id == "3302" ? "GreenTicket@Store@ReplenishClickSkillSummary" : "GreenTicket@Store@ClickItem");
    const auto shop_profile = Task.get("GreenTicket@Store@ReplenishShopProfile");
    if (!limits || !click_task || (limits->special_params.size() != 3 && limits->special_params.size() != 4) ||
        !shop_profile || shop_profile->special_params.size() != 1 || shop_profile->special_params.front() < 0 ||
        shop_profile->special_params.front() > 1) {
        LogWarn << __FUNCTION__ << "unknown green ticket material or shop profile" << item_id;
        return false;
    }
    const int profile = shop_profile->special_params.front();
    const int unit_price = limits->special_params[0];
    const int monthly_limit = limits->special_params[profile + 1];
    const int bundle_size = limits->special_params.size() == 4 ? limits->special_params[3] : 1;
    if (unit_price <= 0 || monthly_limit < 0 || bundle_size <= 0 || count <= 0) {
        LogWarn << __FUNCTION__ << "invalid green ticket shop request" << item_id << count << monthly_limit << profile;
        return false;
    }
    count = count / bundle_size + (count % bundle_size != 0);
    if (monthly_limit != 0) {
        count = std::min(count, monthly_limit);
    }
    // 不限购由商品配置明确指定；有限库存识别失败仍拒绝购买。
    const auto stock_sufficient = [&](std::optional<int> remaining) {
        return remaining ? *remaining >= count && (monthly_limit == 0 || *remaining <= monthly_limit)
                         : monthly_limit == 0;
    };
    if (need_exit() || ItemData.get_item_rarity(item_id) != 2 || !run_task("GreenTicket@Store@ReplenishStoreBegin") ||
        !run_task("GreenTicket@Store@ReplenishChooseTicketType")) {
        return false;
    }

    bool found = false;
    for (int swipe = 0; swipe <= MaxSwipes && !need_exit(); ++swipe) {
        FeatureMatcher matcher(ctrler()->get_image());
        matcher.set_task_info(click_task);
        matcher.set_templ(item_id);
        const auto matches = matcher.analyze().value_or(FeatureMatcher::ResultsVec {});
        const auto& roi = click_task->roi;
        const auto complete_match = [&](const FeatureMatchRect& match) {
            const auto& rect = match.rect;
            return rect.x >= roi.x && rect.y >= roi.y && rect.x + rect.width <= roi.x + roi.width &&
                   rect.y + rect.height <= roi.y + roi.height;
        };
        const auto match = std::ranges::find_if(matches, complete_match);
        if (match != matches.end()) {
            if (need_exit() || !ctrler()->click(match->rect) ||
                !run_task("GreenTicket@Store@ReplenishPurchasePage", 3)) {
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
    const auto initial_stock = read_remaining_stock(initial_image);
    if (initial_stock) {
        count = std::min(count, *initial_stock);
    }
    const auto initial_balance =
        MaterialSynthesisImageAnalyzer::read_number(initial_image, "GreenTicket@Store@ReplenishPurchaseBalance");
    if (count <= 0 || !recognize_name(initial_image) ||
        MaterialSynthesisImageAnalyzer::read_number(initial_image, "GreenTicket@Store@ReplenishPurchaseQuantity") !=
            1 ||
        !stock_sufficient(initial_stock) || !initial_balance ||
        static_cast<std::int64_t>(unit_price) * count > *initial_balance) {
        LogWarn << __FUNCTION__ << "purchase identity, remaining stock or balance could not be confirmed" << item_id
                << count << initial_stock.value_or(-1) << profile;
        return false;
    }
    // 只兑换缺口与现场剩余额度的较小值；付款前复核，数量被界面额外钳制时不付款。
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
    const auto remaining = read_remaining_stock(image);
    if (need_exit() || !recognize_name(image) || !selected || !price || !balance || *selected != count ||
        !stock_sufficient(remaining) || *price <= 0 || *balance < *price ||
        static_cast<std::int64_t>(unit_price) * count != *price) {
        LogWarn << __FUNCTION__ << "purchase identity, quantity, stock, price or balance could not be confirmed"
                << item_id << count << selected.value_or(-1) << remaining.value_or(-1) << price.value_or(-1)
                << balance.value_or(-1);
        return false;
    }
    // 保留默认识别重试以等待收据；付款节点只执行一次，任务链不会回到付款节点。
    if (!run_task("GreenTicket@Store@ReplenishPurchase") || need_exit()) {
        return false;
    }
    m_purchased_count = static_cast<std::int64_t>(count) * bundle_size;
    auto info = basic_info_with_what("GreenTicketPurchased");
    info["details"] = json::object {
        { "material", ItemData.get_item_name(item_id) },
        { "material_id", item_id },
        { "count", m_purchased_count },
        { "cost", *price },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

bool asst::GreenTicketShoppingTask::run_task(const std::string& name, int retry_times)
{
    return ProcessTask(*this, { name }).set_retry_times(retry_times).run();
}

std::optional<int> asst::GreenTicketShoppingTask::read_remaining_stock(const cv::Mat& image)
{
    const auto task = Task.get<OcrTaskInfo>("GreenTicket@Store@ReplenishPurchaseStock");
    if (!task) {
        LogWarn << __FUNCTION__ << "missing green ticket stock observation configuration";
        return std::nullopt;
    }
    RegionOCRer analyzer(image);
    analyzer.set_task_info(task);
    analyzer.set_use_raw(true);
    analyzer.set_required({});
    analyzer.set_replace({});
    const auto result = analyzer.analyze();
    if (!result || result->score < 0.9) {
        return std::nullopt;
    }
    auto text = result->text;
    utils::string_trim(text);
    for (const auto& prefix : task->text) {
        if (prefix.empty() || !text.starts_with(prefix)) {
            continue;
        }
        auto number = text.substr(prefix.size());
        utils::string_trim(number);
        if (number.empty() || number.front() < '0' || number.front() > '9') {
            continue;
        }
        int remaining = 0;
        if (utils::chars_to_number<int, true>(number, remaining)) {
            return remaining;
        }
    }
    return std::nullopt;
}
