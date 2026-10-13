#include "ReclamationResourceTaskPlugin.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <iterator>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::ReclamationResourceTaskPlugin::load_params(const json::value&)
{
    reset();
    set_retry_times(0);
    const auto* mode = std::get_if<TalesMode>(&m_config->get_mode());
    return mode && *mode == TalesMode::ResourceInSave;
}

void asst::ReclamationResourceTaskPlugin::reset()
{
    m_failed = false;
    m_day = 0;
    m_expected_day.reset();
    m_selected_event.clear();
    m_dispatched_events.clear();
    m_smelting_recipe = 0;
    m_smelting_batches = 0;
    m_shop_item = ShopItem::Water;
    m_purchase_amount = 0;
    m_purchase_stock = 0;
    m_purchase_unit = 0;
    m_drinks_before_cooking = 0;
    m_prepared_drinks = -1;
    m_ingredient_holds = 0;
    m_stalled_holds = 0;
}

bool asst::ReclamationResourceTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", "") != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    static constexpr std::array<std::string_view, 19> Tasks = {
        "ReviewDay",      "ReviewEvents",   "ReviewOperators", "AfterExpedition", "ReviewExchange",
        "ReviewPurchase", "PurchaseResult", "ReviewSupplies",  "ReviewCooking",   "CookingResult",
        "AfterCooking",   "ErrorOperators", "ErrorPurchase",   "BeginSmelting",   "ReviewSmelting",
        "SmeltingResult", "AfterSmelting",  "BeginShopping",   "VerifySoldOut",
    };
    return std::ranges::any_of(Tasks, [&](std::string_view name) {
        return task == std::string("Tales@RA@RIS-") + std::string(name);
    });
}

bool asst::ReclamationResourceTaskPlugin::_run()
{
    LogTraceFunction;
    const auto process = dynamic_cast<ProcessTask*>(m_task_ptr);
    if (!process) {
        return fail("Recognition", "Missing process task");
    }
    const auto& task = process->get_last_task_name();
    if (task == "Tales@RA@RIS-ReviewDay") {
        return review_day();
    }
    if (task == "Tales@RA@RIS-ReviewEvents") {
        return review_events();
    }
    if (task == "Tales@RA@RIS-ReviewOperators") {
        return review_operators();
    }
    if (task == "Tales@RA@RIS-AfterExpedition") {
        m_dispatched_events.emplace(m_selected_event);
        return next("BeginEvents");
    }
    if (task == "Tales@RA@RIS-BeginSmelting") {
        return begin_smelting();
    }
    if (task == "Tales@RA@RIS-ReviewSmelting") {
        return review_smelting();
    }
    if (task == "Tales@RA@RIS-SmeltingResult") {
        return review_reward(350, "DismissSmelting", "Smelting", "SmeltingRewardOcr");
    }
    if (task == "Tales@RA@RIS-AfterSmelting") {
        return finish_smelting();
    }
    if (task == "Tales@RA@RIS-BeginShopping") {
        m_shop_item = ShopItem::Water;
        return next("ReviewExchange");
    }
    if (task == "Tales@RA@RIS-ReviewExchange") {
        return review_exchange();
    }
    if (task == "Tales@RA@RIS-ReviewPurchase") {
        return review_purchase_amount();
    }
    if (task == "Tales@RA@RIS-PurchaseResult") {
        return review_reward(m_purchase_amount * m_purchase_unit, "DismissPurchase", "Purchase");
    }
    if (task == "Tales@RA@RIS-VerifySoldOut") {
        return verify_purchase();
    }
    if (task == "Tales@RA@RIS-ReviewSupplies") {
        return review_supplies(false);
    }
    if (task == "Tales@RA@RIS-AfterCooking") {
        return review_supplies(true);
    }
    if (task == "Tales@RA@RIS-ReviewCooking") {
        return review_cooking();
    }
    if (task == "Tales@RA@RIS-CookingResult") {
        return review_reward(99, "DismissCooking", "Supplies");
    }
    return fail(task == "Tales@RA@RIS-ErrorOperators" ? "Operators" : "Purchase");
}

std::optional<int> asst::ReclamationResourceTaskPlugin::read_number(const std::string& task_name) const
{
    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("Tales@RA@RIS-" + task_name);
    const auto result = analyzer.analyze();
    int value = 0;
    if (!result || !utils::chars_to_number<int, true>(result->text, value) || value < 0) {
        LogError << "Unable to read resource farming number" << task_name << (result ? result->text : "No OCR result");
        return std::nullopt;
    }
    return value;
}

bool asst::ReclamationResourceTaskPlugin::review_day()
{
    const auto day = read_number("DayOcr");
    if (!day || *day == 0 || *day >= INT_MAX || (m_expected_day && *day != *m_expected_day)) {
        return fail("Day", day ? std::to_string(*day) : "No OCR result");
    }
    m_day = *day;
    m_expected_day = m_day + 1;
    m_selected_event.clear();
    m_dispatched_events.clear();
    LogInfo << "Resource farming day" << m_day << "cycle day" << (m_day - 1) % 3 + 1;
    return next(m_day % 3 == 1 ? "BeginEvents" : "SkipActs");
}

bool asst::ReclamationResourceTaskPlugin::review_events()
{
    const auto info = Task.get<OcrTaskInfo>("Tales@RA@RIS-EventNames");
    const auto image = ctrler()->get_image();
    OCRer analyzer(image);
    analyzer.set_task_info(info);
    // 保留实际列表顺序，避免 required 过滤后跳过邮筒下方的第一项。
    analyzer.set_required({});
    const auto result = analyzer.analyze();
    if (!result) {
        return fail("EventList", "No event names");
    }

    // 半透明面板会透出地图文字；保留原图 OCR，仅剔除没有事件文字亮度的背景结果。
    cv::Mat gray;
    cv::Mat text_mask;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::inRange(gray, info->bin_threshold[0], info->bin_threshold[1], text_mask);

    auto normalize = [](std::string text) {
        for (const auto* token : { " ", "\t", "\n", "-", "—", "–", "·" }) {
            utils::string_replace_all_in_place(text, token, "");
        }
        return text;
    };
    auto rows = *result;
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) { return lhs.rect.y < rhs.rect.y; });
    OCRer::ResultsVec names;
    for (auto& row : rows) {
        // 排除窗口边缘不完整的文字。
        if (row.rect.y < info->roi.y + 8 || row.rect.y + row.rect.height > info->roi.y + info->roi.height - 8) {
            continue;
        }
        if (cv::countNonZero(text_mask(make_rect<cv::Rect>(row.rect))) == 0) {
            continue;
        }
        if (!names.empty() && std::abs(names.back().rect.y - row.rect.y) <= 10) {
            auto& last = names.back();
            last.text = last.rect.x <= row.rect.x ? last.text + row.text : row.text + last.text;
            last.rect = Rect::bounding_box(last.rect, row.rect);
        }
        else {
            names.emplace_back(std::move(row));
        }
    }

    if (info->text.size() != 6 || names.empty()) {
        return fail("EventList", "Unable to read the visible event list");
    }
    const auto mailbox =
        std::ranges::find_if(names, [&](const auto& name) { return normalize(name.text) == normalize(info->text[1]); });
    // 邮筒不在当前窗口时，从可见列表第一项继续，不滚动寻找隐藏的常驻事件。
    const auto candidate = mailbox == names.end() ? names.begin() : std::next(mailbox);
    if (candidate == names.end()) {
        return next("CloseEvents");
    }
    const auto text = normalize(candidate->text);
    const auto selected = std::find_if(info->text.begin() + 2, info->text.end(), [&](const auto& name) {
        return normalize(name) == text;
    });
    if (selected == info->text.end()) {
        LogInfo << "No supported expedition at the first visible event" << candidate->text;
        return next("CloseEvents");
    }
    if (m_dispatched_events.contains(*selected)) {
        return fail("EventList", "A dispatched event is still present");
    }
    m_selected_event = *selected;
    LogInfo << "Resource farming selected event" << m_selected_event;
    Task.get("Tales@RA@RIS-ClickEvent")->specific_rect = candidate->rect.center_zoom(0.8);
    Task.get<OcrTaskInfo>("Tales@RA@RIS-MapEvent")->text = { *selected };
    return next("ClickEvent");
}

bool asst::ReclamationResourceTaskPlugin::review_operators()
{
    MultiMatcher analyzer(ctrler()->get_image());
    analyzer.set_task_info("Tales@RA@RIS-OperatorCard");
    const auto result = analyzer.analyze();
    if (!result || result->size() < 3) {
        return fail("Operators", "Fewer than three available operator cards");
    }
    return next("SelectFirstOperator");
}

bool asst::ReclamationResourceTaskPlugin::begin_smelting()
{
    m_smelting_recipe = 0;
    m_smelting_batches = 0;
    return next("SelectIronGold");
}

bool asst::ReclamationResourceTaskPlugin::review_smelting()
{
    const auto batches = read_number("SmeltingBatchesOcr");
    if (!batches || *batches < 1 || *batches > 7 || *batches <= m_smelting_batches) {
        return fail("Smelting", "Unable to select seven batches");
    }
    m_smelting_batches = *batches;
    if (*batches == 7) {
        const auto output = read_number("SmeltingOutputOcr");
        if (!output || *output != 350) {
            return fail("Smelting", "Expected 350 gold from seven batches");
        }
        return next("SmeltGold");
    }
    return next("AddSmeltingBatch");
}

bool asst::ReclamationResourceTaskPlugin::finish_smelting()
{
    m_smelting_batches = 0;
    ++m_smelting_recipe;
    switch (m_smelting_recipe) {
    case 1:
        return next("SelectStoneGold");
    case 2:
        return next("SelectWoodGold");
    case 3:
        return next("EnterExchange");
    default:
        return fail("Smelting", "Unexpected smelting recipe index");
    }
}

bool asst::ReclamationResourceTaskPlugin::review_exchange()
{
    const bool water = m_shop_item == ShopItem::Water;
    const auto image = ctrler()->get_image();
    Matcher sold_out(image);
    sold_out.set_task_info(water ? "Tales@RA@RIS-WaterSoldOut" : "Tales@RA@RIS-SoldOut");
    if (sold_out.analyze()) {
        return finish_shop_item();
    }
    Matcher gold(image);
    gold.set_task_info(water ? "Tales@RA@RIS-WaterGoldPrice" : "Tales@RA@RIS-GoldPrice");
    if (!gold.analyze()) {
        return fail("Purchase", "The fixed shop slot does not use gold");
    }
    m_purchase_amount = 0;
    m_purchase_stock = 0;
    m_purchase_unit = 0;
    Task.get<OcrTaskInfo>("Tales@RA@RIS-PurchaseDialog")->text = { water ? "清水" : "澄亮石" };
    return next(water ? "SelectWater" : "SelectStone");
}

bool asst::ReclamationResourceTaskPlugin::review_purchase_amount()
{
    if (m_purchase_stock == 0) {
        const auto stock = read_number("PurchaseStockOcr");
        const auto unit = read_number("PurchaseUnitOcr");
        if (!stock || *stock < 1 || !unit || *unit < 1 || *stock > INT_MAX / *unit) {
            return fail("Purchase", "Unable to read the remaining stock or item quantity per unit");
        }
        m_purchase_stock = *stock;
        m_purchase_unit = *unit;
        LogInfo << "Resource farming purchase stock" << m_purchase_stock << "quantity per unit" << m_purchase_unit;
    }
    const auto amount = read_number("PurchaseAmountOcr");
    if (!amount || *amount < 1 || *amount > m_purchase_stock) {
        return fail("Purchase", "Invalid purchase quantity");
    }
    if (*amount <= m_purchase_amount) {
        return fail("Purchase", "Purchase quantity did not increase");
    }
    m_purchase_amount = *amount;
    return next(m_purchase_amount == m_purchase_stock ? "Pay" : "AddPurchase");
}

bool asst::ReclamationResourceTaskPlugin::verify_purchase()
{
    Matcher sold_out(ctrler()->get_image());
    sold_out.set_task_info(m_shop_item == ShopItem::Water ? "Tales@RA@RIS-WaterSoldOut" : "Tales@RA@RIS-SoldOut");
    if (!sold_out.analyze()) {
        return fail("Purchase", "The purchased item is not sold out");
    }
    return finish_shop_item();
}

bool asst::ReclamationResourceTaskPlugin::finish_shop_item()
{
    if (m_shop_item == ShopItem::Water) {
        m_shop_item = ShopItem::Stone;
        return next("ReviewExchange");
    }
    return next("LeaveExchange");
}

bool asst::ReclamationResourceTaskPlugin::review_supplies(bool after_cooking)
{
    const auto amount = read_number("HeldDrinksOcr");
    if (!amount || *amount > 1'000'000) {
        return fail("Supplies", "Unable to read the energy drink inventory");
    }
    if (after_cooking) {
        if (*amount != m_drinks_before_cooking + 99) {
            return fail("Supplies", "Cooking did not add exactly 99 energy drinks");
        }
        return next("StartToday");
    }
    if (*amount > 99) {
        return next("StartToday");
    }
    m_drinks_before_cooking = *amount;
    m_prepared_drinks = -1;
    m_ingredient_holds = 0;
    m_stalled_holds = 0;
    return next("OpenCooking");
}

bool asst::ReclamationResourceTaskPlugin::review_cooking()
{
    const auto prepared = read_number("PreparedDrinksOcr");
    if (!prepared || *prepared > 99) {
        return fail("Ingredients", "Unable to read the prepared drink quantity");
    }
    if (*prepared == 99) {
        return next("Cook");
    }
    if (m_ingredient_holds == 0) {
        const auto available = read_number("FirstIngredientOcr");
        if (!available || *available == 0) {
            return fail("Ingredients", "No ingredient in the first slot");
        }
    }
    if (*prepared <= m_prepared_drinks) {
        ++m_stalled_holds;
    }
    else {
        m_stalled_holds = 0;
    }
    if (m_stalled_holds >= 2 || ++m_ingredient_holds > 30) {
        return fail("Ingredients", "The first ingredient cannot produce 99 energy drinks");
    }
    m_prepared_drinks = *prepared;
    return next("AddIngredient");
}

bool asst::ReclamationResourceTaskPlugin::review_reward(
    int expected,
    const std::string& next_task,
    const std::string& reason,
    const std::string& ocr_task)
{
    const auto amount = read_number(ocr_task);
    if (!amount || *amount != expected) {
        return fail(reason, "Unexpected reward quantity");
    }
    return next(next_task);
}

bool asst::ReclamationResourceTaskPlugin::next(const std::string& task_name)
{
    const auto process = dynamic_cast<ProcessTask*>(m_task_ptr);
    if (!process || !process->override_next(process->get_last_task_name(), { "Tales@RA@RIS-" + task_name })) {
        return fail("Recognition", "Unable to select the next resource farming task");
    }
    return true;
}

bool asst::ReclamationResourceTaskPlugin::fail(const std::string& reason, const std::string& detail)
{
    m_failed = true;
    LogError << "Resource farming stopped" << reason << detail << "day" << m_day;
    if (m_task_ptr) {
        m_task_ptr->set_enable(false);
    }
    auto info = basic_info_with_what("ReclamationResourceError");
    info["details"] = json::object { { "reason", reason }, { "detail", detail }, { "day", m_day } };
    callback(AsstMsg::SubTaskExtraInfo, info);
    return false;
}
