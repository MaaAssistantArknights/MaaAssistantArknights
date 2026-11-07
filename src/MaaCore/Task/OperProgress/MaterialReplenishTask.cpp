#include "MaterialReplenishTask.h"

#include <algorithm>
#include <vector>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/MiniGame/EventShopTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/FeatureMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/MaterialSynthesisImageAnalyzer.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::MaterialReplenishTask::_run()
{
    m_acquired = replenish_material(m_item_id, m_count, m_options);
    return m_acquired >= m_count;
}

std::optional<std::int64_t>
    asst::MaterialReplenishTask::buy_material_with_tickets(const std::string& item_id, int count, bool parametric)
{
    const int max_swipes = parametric ? 20 : 8;
    const auto limits = ItemData.get_ticket_shop_item(item_id, parametric);
    const auto click_task = Task.get<FeatureMatchTaskInfo>(
        parametric          ? "Parametric@Store@ClickItem"
        : item_id == "3302" ? "GreenTicket@Store@ReplenishClickSkillSummary"
                            : "GreenTicket@Store@ClickItem");
    const auto shop_profile = Task.get("GreenTicket@Store@ReplenishShopProfile");
    if (!limits || !click_task || !shop_profile || shop_profile->special_params.size() != 1 ||
        shop_profile->special_params.front() < 0 || shop_profile->special_params.front() > 1) {
        LogWarn << __FUNCTION__ << "unknown ticket shop material or shop profile" << item_id;
        return std::nullopt;
    }
    const int profile = shop_profile->special_params.front();
    const int unit_price = limits->price;
    const int monthly_limit = limits->monthly_limits[profile];
    const int bundle_size = limits->quantity;
    if (unit_price <= 0 || monthly_limit < 0 || bundle_size <= 0 || count <= 0) {
        LogWarn << __FUNCTION__ << "invalid ticket shop request" << item_id << count << monthly_limit << profile;
        return std::nullopt;
    }
    if (need_exit() || !run_material_purchase_task("GreenTicket@Store@ReplenishStoreBegin") ||
        !run_material_purchase_task(
            parametric ? "Parametric@Store@ChooseTicketType" : "GreenTicket@Store@ReplenishChooseTicketType")) {
        return std::nullopt;
    }

    bool found = false;
    for (int swipe = 0; swipe <= max_swipes && !need_exit(); ++swipe) {
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
                !run_material_purchase_task("GreenTicket@Store@ReplenishPurchasePage", 3)) {
                return std::nullopt;
            }
            found = true;
            break;
        }
        if (swipe < max_swipes && !run_material_purchase_task("RedTicket@Store@Swipe")) {
            return std::nullopt;
        }
    }
    if (!found && !need_exit()) {
        return 0;
    }
    if (need_exit()) {
        LogWarn << __FUNCTION__ << "material unavailable in the ticket store" << item_id;
        return std::nullopt;
    }

    return purchase_selected_material(
        item_id,
        count,
        *limits,
        parametric ? MaterialPurchaseSource::ParametricModels : MaterialPurchaseSource::GreenTickets);
}

std::optional<std::int64_t> asst::MaterialReplenishTask::purchase_selected_material(
    const std::string& item_id,
    int count,
    const ItemConfig::TicketShopItem& limits,
    MaterialPurchaseSource source)
{
    const bool parametric = source == MaterialPurchaseSource::ParametricModels;
    const bool event = source == MaterialPurchaseSource::EventShop;
    const int unit_price = limits.price;
    const int bundle_size = limits.quantity;
    const int profile = Task.get("GreenTicket@Store@ReplenishShopProfile")->special_params.front();
    const int monthly_limit = limits.monthly_limits[profile];
    count = count / bundle_size + (count % bundle_size != 0);
    if (monthly_limit != 0) {
        count = std::min(count, monthly_limit);
    }
    const auto stock_sufficient = [&](std::optional<int> remaining) {
        return remaining ? *remaining >= count && (monthly_limit == 0 || *remaining <= monthly_limit)
                         : monthly_limit == 0;
    };
    const std::string balance_task = event        ? "MaterialPurchase@EventBalance"
                                     : parametric ? "Parametric@Store@PurchaseBalance"
                                                  : "GreenTicket@Store@ReplenishPurchaseBalance";
    const auto recognize_name = [&](const cv::Mat& image) {
        OCRer analyzer(image);
        analyzer.set_task_info(
            (parametric || event) ? "Parametric@Store@PurchaseName" : "GreenTicket@Store@ReplenishPurchaseName");
        analyzer.set_required({ ItemData.get_item_name(item_id) });
        const auto names = analyzer.analyze();
        return names && names->size() == 1 && names->front().score >= 0.9;
    };
    const auto initial_image = ctrler()->get_image();
    const auto initial_stock = read_remaining_stock(initial_image);
    if (initial_stock) {
        count = std::min(count, *initial_stock);
    }
    const auto initial_balance = MaterialSynthesisImageAnalyzer::read_number(initial_image, balance_task);
    if (initial_stock == 0) {
        return 0;
    }
    if (count <= 0 || !recognize_name(initial_image) ||
        MaterialSynthesisImageAnalyzer::read_number(initial_image, "GreenTicket@Store@ReplenishPurchaseQuantity") !=
            1 ||
        !stock_sufficient(initial_stock) || !initial_balance) {
        LogWarn << __FUNCTION__ << "purchase identity, remaining stock or balance could not be confirmed" << item_id
                << count << initial_stock.value_or(-1) << profile;
        return std::nullopt;
    }
    count = std::min(count, *initial_balance / unit_price);
    if (count == 0) {
        return 0;
    }
    // 兑换缺口、库存和余额允许的最大数量；付款前复核，数量被界面额外钳制时不付款。
    for (int selected = 1; selected < count; ++selected) {
        if (need_exit() || !run_material_purchase_task("Store@Increase")) {
            return std::nullopt;
        }
    }
    const auto image = ctrler()->get_image();
    const auto selected =
        MaterialSynthesisImageAnalyzer::read_number(image, "GreenTicket@Store@ReplenishPurchaseQuantity");
    const auto price = MaterialSynthesisImageAnalyzer::read_number(
        image,
        event ? "MaterialPurchase@EventTotalPrice" : "GreenTicket@Store@ReplenishPurchaseTotalPrice");
    const auto balance = MaterialSynthesisImageAnalyzer::read_number(image, balance_task);
    const auto remaining = read_remaining_stock(image);
    if (need_exit() || !recognize_name(image) || !selected || !price || !balance || *selected != count ||
        !stock_sufficient(remaining) || *price <= 0 || *balance < *price ||
        static_cast<std::int64_t>(unit_price) * count != *price) {
        LogWarn << __FUNCTION__ << "purchase identity, quantity, stock, price or balance could not be confirmed"
                << item_id << count << selected.value_or(-1) << remaining.value_or(-1) << price.value_or(-1)
                << balance.value_or(-1);
        return std::nullopt;
    }
    // 保留默认识别重试以等待收据；付款节点只执行一次，任务链不会回到付款节点。
    if (!run_material_purchase_task(event ? "MaterialPurchase@EventPurchase" : "GreenTicket@Store@ReplenishPurchase") ||
        need_exit()) {
        return std::nullopt;
    }
    const auto purchased_count = static_cast<std::int64_t>(count) * bundle_size;
    report_material_purchase(
        item_id,
        purchased_count,
        *price,
        event        ? "EventMaterialPurchased"
        : parametric ? "ParametricPurchased"
                     : "GreenTicketPurchased");
    return purchased_count;
}

void asst::MaterialReplenishTask::report_material_purchase(
    const std::string& item_id,
    std::int64_t count,
    int cost,
    const std::string& what)
{
    auto info = basic_info_with_what(what);
    info["details"] = json::object {
        { "material", ItemData.get_item_name(item_id) },
        { "material_id", item_id },
        { "count", count },
        { "cost", cost },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::MaterialReplenishTask::run_material_purchase_task(const std::string& name, int retry_times)
{
    return ProcessTask(*this, { name }).set_retry_times(retry_times).run();
}

std::optional<int> asst::MaterialReplenishTask::read_remaining_stock(const cv::Mat& image)
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

bool asst::MaterialPurchaseOptions::supports(MaterialPurchaseSource source, const std::string& item_id) const
{
    const auto rarity = ItemData.get_item_rarity(item_id);
    const bool elite_material = item_id.starts_with("30") || item_id.starts_with("31");
    switch (source) {
    case MaterialPurchaseSource::GreenTickets:
        return ItemData.get_ticket_shop_item(item_id).has_value();
    case MaterialPurchaseSource::ParametricModels:
        return ItemData.get_ticket_shop_item(item_id, true).has_value();
    case MaterialPurchaseSource::PremiumVoucher:
        return rarity == 3 && elite_material;
    case MaterialPurchaseSource::AdvancedVoucher:
        return rarity == 2 && elite_material;
    case MaterialPurchaseSource::EventShop:
        return !event_shop_task().empty() && std::ranges::find(ItemData.get_ordered_material_item_id(), item_id) !=
                                                 ItemData.get_ordered_material_item_id().end();
    }
    return false;
}

std::string asst::MaterialPurchaseOptions::event_shop_task() const
{
    for (const auto& [_, stage] : activity_stages) {
        const auto name = stage.substr(0, stage.find('-')) + "@MaterialPurchase@EventShopBegin";
        const auto task = Task.get(name);
        if (task && task->special_params == std::vector<int> { 1 }) {
            return name;
        }
    }
    return {};
}

bool asst::MaterialPurchaseOptions::can_buy(const std::string& item_id) const
{
    return std::ranges::any_of(sources, [&](auto source) { return supports(source, item_id); });
}

std::optional<asst::MaterialPurchaseOptions> asst::MaterialPurchaseOptions::parse(const json::value& params)
{
    MaterialPurchaseOptions options;
    if (params.contains("material_purchase_sources")) {
        const auto sources = params.find<std::vector<std::string>>("material_purchase_sources");
        if (!sources) {
            return std::nullopt;
        }
        const std::unordered_map<std::string, MaterialPurchaseSource> names {
            { "GreenTickets", MaterialPurchaseSource::GreenTickets },
            { "ParametricModels", MaterialPurchaseSource::ParametricModels },
            { "PremiumVoucher", MaterialPurchaseSource::PremiumVoucher },
            { "AdvancedVoucher", MaterialPurchaseSource::AdvancedVoucher },
            { "EventShop", MaterialPurchaseSource::EventShop },
        };
        for (const auto& name : *sources) {
            const auto found = names.find(name);
            if (found == names.end() || std::ranges::find(options.sources, found->second) != options.sources.end()) {
                LogError << __FUNCTION__ << "unknown or duplicate material source" << name;
                return std::nullopt;
            }
            options.sources.push_back(found->second);
        }
    }
    else {
        // 保留已有调用方的独立开关；新来源列表存在时以列表为准，空列表即全部关闭。
        for (const auto& [key, source] : {
                 std::pair { "buy_materials_with_green_tickets", MaterialPurchaseSource::GreenTickets },
                 std::pair { "buy_materials_with_parametric_models", MaterialPurchaseSource::ParametricModels },
             }) {
            const auto value = params.find_value(key);
            if (value && !value->is<bool>()) {
                return std::nullopt;
            }
            if (params.get(key, false)) {
                options.sources.push_back(source);
            }
        }
    }
    if (params.contains("activity_material_stages")) {
        const auto stages = params.find<std::unordered_map<std::string, std::string>>("activity_material_stages");
        if (!stages) {
            return std::nullopt;
        }
        for (const auto& [id, stage] : *stages) {
            if (ItemData.get_item_name(id).empty() || stage.empty()) {
                return std::nullopt;
            }
        }
        options.activity_stages = *stages;
    }
    return options;
}

std::int64_t asst::MaterialReplenishTask::replenish_material(
    const std::string& item_id,
    int count,
    const MaterialPurchaseOptions& options)
{
    if (count <= 0 || !options.can_buy(item_id)) {
        return 0;
    }
    std::int64_t acquired = 0;
    for (const auto source : options.sources) {
        if (need_exit()) {
            return 0;
        }
        if (!options.supports(source, item_id)) {
            continue;
        }
        std::optional<std::int64_t> result;
        switch (source) {
        case MaterialPurchaseSource::GreenTickets:
            result = buy_material_with_tickets(item_id, count, false);
            break;
        case MaterialPurchaseSource::ParametricModels:
            result = buy_material_with_tickets(item_id, count, true);
            break;
        case MaterialPurchaseSource::PremiumVoucher:
            result = redeem_material_voucher(item_id, count, true);
            break;
        case MaterialPurchaseSource::AdvancedVoucher:
            result = redeem_material_voucher(item_id, count, false);
            break;
        case MaterialPurchaseSource::EventShop:
            result = buy_event_material(item_id, count, options.event_shop_task());
            break;
        }
        if (source == MaterialPurchaseSource::PremiumVoucher || source == MaterialPurchaseSource::AdvancedVoucher) {
            for (const auto* page : { "MaterialPurchase@VoucherSelect", "MaterialPurchase@VoucherDetail" }) {
                if (need_exit()) {
                    return 0;
                }
                OCRer popup(ctrler()->get_image());
                popup.set_task_info(page);
                if (popup.analyze() && (!ctrler()->press_esc() || !sleep(500))) {
                    return 0;
                }
            }
        }
        // 结果未知时停止补料；已确认的部分数量从缺口扣除后交给下一来源。
        if (!result) {
            return acquired;
        }
        acquired += *result;
        if (*result >= count) {
            return acquired;
        }
        count -= static_cast<int>(*result);
    }
    return acquired;
}

std::optional<std::int64_t>
    asst::MaterialReplenishTask::buy_event_material(const std::string& item_id, int count, const std::string& shop_task)
{
    if (!run_material_purchase_task(shop_task)) {
        return std::nullopt;
    }
    EventShopTaskPlugin shop(m_callback, m_inst, m_task_chain);
    shop.set_task_id(m_task_id);
    const auto item = shop.find_material(ItemData.get_item_name(item_id));
    if (!item) {
        return std::nullopt;
    }
    if (item->empty()) {
        return 0;
    }
    if (need_exit() || !ctrler()->click(*item) ||
        !run_material_purchase_task("MaterialPurchase@EventPurchasePage", 3)) {
        return std::nullopt;
    }
    const auto image = ctrler()->get_image();
    const auto quantity =
        MaterialSynthesisImageAnalyzer::read_number(image, "GreenTicket@Store@ReplenishPurchaseQuantity");
    const auto price = MaterialSynthesisImageAnalyzer::read_number(image, "MaterialPurchase@EventTotalPrice");
    const auto stock = read_remaining_stock(image);
    if (quantity != 1 || !price || *price <= 0 || !stock) {
        return std::nullopt;
    }
    if (*stock == 0) {
        return 0;
    }
    return purchase_selected_material(
        item_id,
        count,
        { *price, 1, { *stock, *stock } },
        MaterialPurchaseSource::EventShop);
}

std::optional<std::int64_t>
    asst::MaterialReplenishTask::redeem_material_voucher(const std::string& item_id, int count, bool premium)
{
    if (!run_material_purchase_task("MaterialPurchase@DepotBegin")) {
        return std::nullopt;
    }
    const std::string kind = premium ? "premium" : "advanced";
    const std::vector<std::string> ids { kind + "_material_issue_voucher", kind + "_material_voucher_perm" };
    std::optional<ItemInfo> voucher;
    cv::Mat previous;
    DepotImageAnalyzer::clear_cached_templates();
    for (int page = 0; page < 20 && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        const auto viewport = image(cv::Rect(0, 100, 1280, 540));
        if (!previous.empty() && cv::norm(previous, viewport, cv::NORM_L1) / viewport.total() < 1.0) {
            return 0;
        }
        DepotImageAnalyzer analyzer(image);
        analyzer.set_item_ids(ids);
        analyzer.set_is_basic(true);
        analyzer.analyze();
        DepotImageAnalyzer::clear_cached_templates();
        // 同页优先用限时券；两类券不混用，也不打开其他材料箱。
        for (const auto& id : ids) {
            const auto found = analyzer.get_result().find(id);
            if (found != analyzer.get_result().end()) {
                voucher = found->second;
                break;
            }
        }
        if (voucher) {
            break;
        }
        previous = viewport.clone();
        if (!run_material_purchase_task("DepotSlowlySwipeToTheRight")) {
            return std::nullopt;
        }
    }
    if (!voucher || need_exit() || !ctrler()->click(voucher->rect) ||
        !run_material_purchase_task("MaterialPurchase@VoucherDetail", 3)) {
        return std::nullopt;
    }
    OCRer title(ctrler()->get_image());
    title.set_task_info("MaterialPurchase@VoucherName");
    title.set_required({ ItemData.get_item_name(voucher->item_id) });
    if (!title.analyze() || !run_material_purchase_task("MaterialPurchase@VoucherUse")) {
        return std::nullopt;
    }
    const auto initial = ctrler()->get_image();
    const auto owned = MaterialSynthesisImageAnalyzer::read_number(initial, "MaterialPurchase@VoucherOwned");
    if (!owned || *owned <= 0 ||
        MaterialSynthesisImageAnalyzer::read_number(initial, "MaterialPurchase@VoucherSelected") != 0) {
        return std::nullopt;
    }
    count = std::min(count, *owned);
    std::optional<Rect> target;
    previous.release();
    for (int page = 0; page < 10 && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        FeatureMatcher matcher(image);
        matcher.set_task_info("MaterialPurchase@VoucherItem");
        matcher.set_templ(item_id);
        const auto matches = matcher.analyze();
        if (matches && matches->size() == 1) {
            target = matches->front().rect;
            break;
        }
        const auto viewport = image(cv::Rect(190, 190, 900, 365));
        if (!previous.empty() && cv::norm(previous, viewport, cv::NORM_L1) / viewport.total() < 1.0) {
            return 0;
        }
        previous = viewport.clone();
        if (!run_material_purchase_task("MaterialPurchase@VoucherSwipe")) {
            return std::nullopt;
        }
    }
    if (!target) {
        return std::nullopt;
    }
    // 点击图标中心增加同一种材料，避开右上角减号。
    for (int selected = 0; selected < count; ++selected) {
        if (need_exit() || !ctrler()->click(target->center_zoom(0.2)) || !sleep(150)) {
            return std::nullopt;
        }
    }
    const auto image = ctrler()->get_image();
    if (need_exit() ||
        MaterialSynthesisImageAnalyzer::read_number(image, "MaterialPurchase@VoucherSelected") != count ||
        MaterialSynthesisImageAnalyzer::read_number(image, "MaterialPurchase@VoucherOwned") != owned) {
        return std::nullopt;
    }
    if (!run_material_purchase_task("MaterialPurchase@VoucherConfirm") || need_exit()) {
        return std::nullopt;
    }
    report_material_purchase(item_id, count, count, premium ? "PremiumVoucherRedeemed" : "AdvancedVoucherRedeemed");
    return count;
}
