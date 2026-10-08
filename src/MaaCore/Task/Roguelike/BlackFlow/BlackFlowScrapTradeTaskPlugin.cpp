#include "BlackFlowScrapTradeTaskPlugin.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>

#include "Config/Roguelike/RoguelikeShoppingConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/Roguelike/RoguelikeParameterAnalyzer.h"

namespace asst::blackflow
{
constexpr std::string_view ScrapTradeEnterTask = "BlackFlow@Roguelike@ScrapTradeEnter";
constexpr std::string_view ScrapTradeDecisionTask = "BlackFlow@Roguelike@ScrapTradeDecision";
constexpr std::string_view ScrapTradeContinueTask = "BlackFlow@Roguelike@ScrapTradeContinue";
constexpr std::string_view ScrapTradeAction = "BlackFlow@Roguelike@ScrapTradeAction";
constexpr std::string_view ScrapTradeToggleTask = "BlackFlow@Roguelike@ScrapTradeToggle";
constexpr std::string_view ScrapTradeBuyConfirmEntry = "BlackFlow@Roguelike@ScrapTradeBuyConfirm-Enter";
constexpr std::string_view ScrapTradeBuyConfirmTask = "BlackFlow@Roguelike@ScrapTradeBuyConfirm";
constexpr std::string_view ScrapTradeRefreshEntry = "BlackFlow@Roguelike@ScrapTradeRefresh-Enter";
constexpr std::string_view ScrapTradeRefreshConfirmTask = "BlackFlow@Roguelike@ScrapTradeRefreshConfirm";
constexpr std::string_view ScrapTradeLeaveEntry = "BlackFlow@Roguelike@ScrapTradeLeave-Enter";
constexpr std::string_view ScrapTradeCultivateEntry = "BlackFlow@Roguelike@ScrapTradeCultivate-Enter";
constexpr std::string_view ScrapTradeCultivateAtMostTask = "BlackFlow@Roguelike@ScrapTradeCultivateAtMost";
constexpr std::string_view ScrapTradeHeldSeedCountTask = "BlackFlow@Roguelike@CultivateHeldSeedCount";
constexpr std::string_view ScrapTradeItemsTask = "BlackFlow@Roguelike@ScrapTradeItems";
constexpr std::string_view ScrapTradeSellingFlagTask = "BlackFlow@Roguelike@CultivateSellingFlag";
constexpr std::string_view ScrapTradeWalletTask = "BlackFlow@Roguelike@CurrentIngots";

constexpr int ScrapTradeMaxToggleAttempts = 3;
constexpr int ScrapTradeSameSlotTolerance = 20;
constexpr int ScrapTradeAtMostClickTimes = 3;
constexpr int ScrapTradeRepeatedClickInterval = 100;

bool BlackFlowScrapTradeTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart) {
        if (task == ScrapTradeEnterTask) {
            m_pending = PendingWork::Enter;
            return true;
        }
        if (task == ScrapTradeDecisionTask) {
            m_pending = PendingWork::Decide;
            return true;
        }
        if (task == ScrapTradeCultivateAtMostTask) {
            m_pending = PendingWork::ClickAtMost;
            return true;
        }
    }
    if (msg == AsstMsg::SubTaskCompleted) {
        if (task == ScrapTradeBuyConfirmTask) {
            m_pending = PendingWork::BuyConfirmed;
            return true;
        }
        if (task == ScrapTradeRefreshConfirmTask) {
            m_pending = PendingWork::RefreshConfirmed;
            return true;
        }
    }
    return false;
}

void BlackFlowScrapTradeTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    enter();
}

bool BlackFlowScrapTradeTaskPlugin::_run()
{
    LogTraceFunction;
    const PendingWork work = m_pending;
    m_pending = PendingWork::None;

    switch (work) {
    case PendingWork::Enter:
        enter();
        break;
    case PendingWork::Decide:
        decide();
        break;
    case PendingWork::ClickAtMost:
        click_at_most();
        break;
    case PendingWork::BuyConfirmed:
        on_purchase_confirmed();
        break;
    case PendingWork::RefreshConfirmed:
        on_refresh_confirmed();
        break;
    case PendingWork::None:
        break;
    }
    return true;
}

void BlackFlowScrapTradeTaskPlugin::enter()
{
    m_phase = Phase::Cultivate;
    m_ledger = {};
    m_counted = false;
    m_inventory_names.clear();
    for (const auto& item : BlackFlowScrapMarket.items()) {
        m_inventory_names.emplace_back(item.name);
    }
    m_sell_table.clear();
    m_sell_backs.clear();
    m_inventory.invalidate();
    m_inventory_last_task.clear();
    m_pending_purchase.reset();
    m_pending_refresh = false;
    m_purchased_rects.clear();
    m_kept.clear();
    m_shop_type.reset();
    m_refresh_count = 0;
    m_toggle_attempts = 0;
    m_liquidating = false;

    const auto& table = m_config->status().shopping_sell_table;
    if (!table.empty()) {
        const auto& names = RoguelikeShopping.get_sell_goods(m_config->get_theme(), table);
        m_sell_table.insert(names.begin(), names.end());
    }
    // 卖表含有增长物，表示这家店是兑现店：交易后卖出全部可售自然物，并按期望刷新。
    m_liquidating = std::ranges::any_of(m_sell_table, [](const std::string& name) {
        const auto item = BlackFlowScrapMarket.find(name);
        return item && item->get().growth_per_acquisition > 0;
    });
}

void BlackFlowScrapTradeTaskPlugin::decide()
{
    if (!BlackFlowScrapMarket.available()) {
        LogWarn << __FUNCTION__ << "BlackFlow scrap market is unavailable";
        leave();
        return;
    }

    const cv::Mat image = ctrler()->get_image();
    const bool selling = on_selling_page(image);
    while (true) {
        switch (m_phase) {
        case Phase::Cultivate: {
            // 培育出的零件可能是板藤，先培育再计数。培育前后都停在买入页，只进行一次。
            if (switch_tab(false, selling)) {
                return;
            }
            m_phase = Phase::Observe;
            RoguelikeParameterAnalyzer analyzer(image);
            const int seeds = analyzer.get_number(image, std::string(ScrapTradeHeldSeedCountTask));
            if (seeds > 0) {
                LogInfo << __FUNCTION__ << "BlackFlow scrap trade cultivates seeds" << seeds;
                set_action(ScrapTradeCultivateEntry);
                return;
            }
            continue;
        }
        case Phase::Observe: {
            if (switch_tab(true, selling)) {
                return;
            }
            std::string error;
            const auto held = m_inventory.survey(*this, m_inventory_names, &error);
            if (!held && !error.empty()) {
                LogWarn << "BlackFlow scrap trade inventory incomplete" << error;
                leave();
                return;
            }
            m_counted = held.has_value();
            if (held) {
                m_ledger.reset(*held);
                LogInfo << __FUNCTION__ << "BlackFlow scrap trade holdings" << *held << "growth"
                        << m_ledger.growth(BlackFlowScrapMarket);
            }
            else {
                // 重复自然物的段间数量不用于倒转；加工品沿已识别的连续段计数并补缺。
                std::vector<std::string> processing;
                for (const auto& name : m_inventory.observed_items()) {
                    const auto item = BlackFlowScrapMarket.find(name);
                    if (item && item->get().category == ScrapCategory::Processing) {
                        processing.emplace_back(name);
                    }
                }
                m_ledger.reset(processing);
                LogInfo << "BlackFlow scrap trade processing holdings" << processing;
            }
            m_phase = Phase::Prepare;
            continue;
        }
        case Phase::Prepare:
            // 非兑现店卖出闲置自然物只为交易筹钱，没有增长物时不卖。
            if (!m_counted || (!m_liquidating && m_ledger.growth(BlackFlowScrapMarket) <= 1)) {
                m_phase = Phase::Trade;
                continue;
            }
            if (switch_tab(true, selling) || sell_matching(false)) {
                return;
            }
            m_phase = Phase::Trade;
            continue;
        case Phase::Trade:
            if (!m_sell_backs.empty()) {
                if (switch_tab(true, selling) || sell_back()) {
                    return;
                }
                continue;
            }
            if (switch_tab(false, selling)) {
                return;
            }
            {
                RoguelikeParameterAnalyzer analyzer(image);
                const int wallet = analyzer.get_number(image, std::string(ScrapTradeWalletTask));
                if (buy(image, wallet) || refresh(wallet)) {
                    return;
                }
            }
            m_phase = Phase::Liquidate;
            continue;
        case Phase::Liquidate:
            if (!m_liquidating) {
                m_phase = Phase::Leave;
                continue;
            }
            if (switch_tab(true, selling) || sell_matching(true)) {
                return;
            }
            m_phase = Phase::Leave;
            continue;
        case Phase::Leave:
            leave();
            return;
        }
    }
}

bool BlackFlowScrapTradeTaskPlugin::switch_tab(bool want_selling, bool selling)
{
    if (want_selling == selling) {
        m_toggle_attempts = 0;
        return false;
    }
    if (++m_toggle_attempts > ScrapTradeMaxToggleAttempts) {
        LogWarn << __FUNCTION__ << "BlackFlow scrap trade cannot switch page" << "want selling" << want_selling;
        leave();
        return true;
    }
    m_inventory.forget_position();
    set_action(ScrapTradeToggleTask);
    return true;
}

// 预卖只卖不随零件获得增长的自然物，兑现时再卖增长物，交易期间保留它们。
bool BlackFlowScrapTradeTaskPlugin::sell_matching(bool liquidating)
{
    std::vector<std::string> names;
    for (const auto& name : m_sell_table) {
        const auto item = BlackFlowScrapMarket.find(name);
        if (item && item->get().category == ScrapCategory::Natural &&
            (liquidating || item->get().growth_per_acquisition == 0) &&
            (!m_counted || m_ledger.may_sell(item->get(), m_sell_table))) {
            names.emplace_back(name);
        }
    }
    std::string error;
    const auto item = m_inventory.find(*this, m_inventory_names, names, &error);
    if (!item) {
        if (!error.empty()) {
            LogWarn << "BlackFlow scrap trade sale search failed" << error;
            leave();
            return true;
        }
        return false;
    }
    return complete_sale(*item, false);
}

bool BlackFlowScrapTradeTaskPlugin::sell_back()
{
    const std::string name = m_sell_backs.front();
    const auto item = BlackFlowScrapMarket.find(name);
    std::string error;
    const auto target = item && m_ledger.may_sell(item->get(), m_sell_table)
                            ? m_inventory.find(*this, m_inventory_names, { name }, &error)
                            : std::optional<ScrapTradeInventoryTarget> {};
    if (!target) {
        if (!error.empty()) {
            LogWarn << "BlackFlow scrap trade sell-back search failed" << name << error;
            leave();
            return true;
        }
        // 已完整查找仍找不到时，沿用放弃卖回这一件的处理。
        LogWarn << __FUNCTION__ << "BlackFlow scrap trade cannot sell back" << name;
        m_sell_backs.pop_front();
        return false;
    }
    return complete_sale(*target, true);
}

bool BlackFlowScrapTradeTaskPlugin::buy(const cv::Mat& image, int wallet)
{
    std::vector<std::string> names;
    for (const auto& item : BlackFlowScrapMarket.items()) {
        if (item.price) {
            names.emplace_back(item.name);
        }
    }
    std::vector<TextRect> shelf = recognize(image, names);
    std::erase_if(shelf, [this](const TextRect& offer) { return already_purchased(offer.rect); });

    if (!m_shop_type && m_purchased_rects.empty()) {
        std::vector<std::string> shelf_names;
        for (const auto& offer : shelf) {
            shelf_names.emplace_back(offer.text);
        }
        m_shop_type = infer_scrap_shop_type(shelf_names, BlackFlowScrapMarket);
        LogInfo << __FUNCTION__ << "BlackFlow scrap shop type" << m_shop_type.value_or("unknown") << "shelf"
                << shelf_names;
    }

    // 价格按名称查表，不识别货架价格；已知的价格效果只会降价，因此表价不会让余额不足。
    // 保留购买先于倒转；非兑现店按买表积累玉米和雾滚草，加工品每种每次进店最多补一件。
    const auto& table = m_config->status().shopping_buy_table;
    for (const auto& goods : RoguelikeShopping.get_goods(m_config->get_theme(), table)) {
        const auto offer = std::ranges::find(shelf, goods.name, &TextRect::text);
        const auto item = BlackFlowScrapMarket.find(goods.name);
        if (offer == shelf.end() || !item || !item->get().price || *item->get().price > wallet) {
            continue;
        }
        const bool keep_natural = !m_liquidating && (goods.name == "回声玉米" || goods.name == "雾滚草");
        if (!keep_natural &&
            (m_kept.contains(goods.name) || !scrap_keep_wanted(item->get(), m_ledger, BlackFlowScrapMarket))) {
            continue;
        }
        LogInfo << __FUNCTION__ << "BlackFlow scrap trade buys to keep" << goods.name << "wallet" << wallet;
        ctrler()->click(offer->rect);
        m_pending_purchase = PendingPurchase { offer->text, offer->rect, true };
        set_action(ScrapTradeBuyConfirmEntry);
        return true;
    }

    if (!m_counted) {
        return false;
    }

    const int growth = m_ledger.growth(BlackFlowScrapMarket);
    const TextRect* best = nullptr;
    int best_net = 0;
    for (const auto& offer : shelf) {
        const auto item = BlackFlowScrapMarket.find(offer.text);
        if (!item || !item->get().price || *item->get().price > wallet) {
            continue;
        }
        const auto net = scrap_trade_net(item->get(), growth, BlackFlowScrapMarket);
        if (net && *net > best_net) {
            best = &offer;
            best_net = *net;
        }
    }
    if (best == nullptr) {
        return false;
    }
    LogInfo << __FUNCTION__ << "BlackFlow scrap trade buys" << best->text << "net" << best_net << "growth" << growth
            << "wallet" << wallet;
    ctrler()->click(best->rect);
    m_pending_purchase = PendingPurchase { best->text, best->rect, false };
    set_action(ScrapTradeBuyConfirmEntry);
    return true;
}

bool BlackFlowScrapTradeTaskPlugin::refresh(int wallet)
{
    const auto& costs = BlackFlowScrapMarket.refresh_costs();
    // 非兑现店只交易首页，攒下的估价留到兑现店卖出。
    if (!m_counted || !m_liquidating || !m_shop_type || m_refresh_count >= static_cast<int>(costs.size())) {
        return false;
    }
    const auto& types = BlackFlowScrapMarket.shop_types();
    const auto type = std::ranges::find(types, *m_shop_type, &ScrapShopType::id);
    if (type == types.end()) {
        return false;
    }
    const int cost = costs[static_cast<std::size_t>(m_refresh_count)];
    const int growth = m_ledger.growth(BlackFlowScrapMarket);
    const double expected = expected_scrap_page_profit(*type, growth, BlackFlowScrapMarket);
    const auto cheapest = cheapest_profitable_price(*type, growth, BlackFlowScrapMarket);
    const bool worth = cheapest && expected > cost && wallet >= cost + *cheapest;
    LogInfo << __FUNCTION__ << "BlackFlow scrap trade refresh" << "worth" << worth << "expected" << expected << "cost"
            << cost << "wallet" << wallet << "growth" << growth;
    if (!worth) {
        return false;
    }
    m_pending_refresh = true;
    set_action(ScrapTradeRefreshEntry);
    return true;
}

// 正常结束和页面异常都直接离店，沿用路由的普通完成；第五层目标按到访计数，异常离店不会挡住后面的商店。
void BlackFlowScrapTradeTaskPlugin::leave()
{
    m_phase = Phase::Leave;
    set_action(ScrapTradeLeaveEntry);
}

void BlackFlowScrapTradeTaskPlugin::on_purchase_confirmed()
{
    if (!m_pending_purchase) {
        return;
    }
    const PendingPurchase purchase = std::move(*m_pending_purchase);
    m_pending_purchase.reset();
    m_purchased_rects.emplace_back(purchase.rect);
    const auto item = BlackFlowScrapMarket.find(purchase.name);
    if (!item) {
        return;
    }
    m_ledger.record_purchase(item->get());
    // 新获得的物品可能插入不同分类，下一次出售前重建位置，保留进店账本。
    m_inventory.invalidate();
    if (purchase.keep) {
        m_kept.insert(purchase.name);
    }
    else {
        // 增长物留到兑现，其余买入物与附带零件立即卖回，零件总数回到买入前。
        if (item->get().growth_per_acquisition == 0) {
            m_sell_backs.emplace_back(purchase.name);
        }
        for (const auto& [name, count] : item->get().bundle) {
            m_sell_backs.insert(m_sell_backs.end(), static_cast<std::size_t>(count), name);
        }
    }
    LogInfo << __FUNCTION__ << "BlackFlow scrap trade purchase confirmed" << purchase.name << "keep" << purchase.keep
            << "pending sell backs" << m_sell_backs.size();
    if (m_counted) {
        LogInfo << "BlackFlow scrap trade held" << m_ledger.held(purchase.name) << "growth"
                << m_ledger.growth(BlackFlowScrapMarket);
    }
}

void BlackFlowScrapTradeTaskPlugin::click_at_most()
{
    const auto hit = get_hit_detail<Matcher::Result>();
    if (hit == nullptr) {
        return;
    }
    for (int times = 0; times < ScrapTradeAtMostClickTimes; ++times) {
        ctrler()->click(hit->rect);
        sleep(ScrapTradeRepeatedClickInterval);
    }
}

bool BlackFlowScrapTradeTaskPlugin::complete_sale(const ScrapTradeInventoryTarget& target, bool sell_back)
{
    std::string error;
    if (!m_inventory.sell(*this, target, &error)) {
        LogWarn << "BlackFlow scrap trade sale failed" << target.text.text << error;
        leave();
        return true;
    }
    m_ledger.record_sale(target.text.text);
    if (sell_back) {
        m_sell_backs.pop_front();
    }
    LogInfo << "BlackFlow scrap trade sale confirmed" << target.text.text;
    if (m_counted) {
        LogInfo << "BlackFlow scrap trade remaining" << m_ledger.held(target.text.text) << "growth"
                << m_ledger.growth(BlackFlowScrapMarket);
    }
    set_action(ScrapTradeContinueTask);
    return true;
}

void BlackFlowScrapTradeTaskPlugin::on_refresh_confirmed()
{
    if (!m_pending_refresh) {
        return;
    }
    m_pending_refresh = false;
    ++m_refresh_count;
    m_purchased_rects.clear();
    LogInfo << __FUNCTION__ << "BlackFlow scrap trade refresh confirmed" << "count" << m_refresh_count;
}

cv::Mat BlackFlowScrapTradeTaskPlugin::capture() const
{
    return ctrler()->get_image();
}

bool BlackFlowScrapTradeTaskPlugin::interrupted() const
{
    return need_exit();
}

bool BlackFlowScrapTradeTaskPlugin::precise_swipe_supported() const
{
    return ControlFeat::support(ctrler()->support_features(), ControlFeat::PRECISE_SWIPE);
}

bool
    BlackFlowScrapTradeTaskPlugin::swipe_by(std::string_view task_name, std::optional<int> distance, std::string* error)
{
    const auto task = Task.get(std::string(task_name));
    bool succeeded = false;
    if (task != nullptr) {
        if (!distance) {
            succeeded = ProcessTask(*this, { std::string(task_name) }).run();
        }
        else {
            const Rect& start = task->specific_rect;
            const Rect end { start.x, start.y + *distance, start.width, start.height };
            const auto& params = task->special_params;
            succeeded = ctrler()->swipe(
                start,
                end,
                params.empty() ? 0 : params.at(0),
                params.size() < 2 ? SwipeExtraDirection::None : to_swipe_extra_direction(params.at(1)),
                params.size() < 3 ? 1 : params.at(2) / 10.0,
                params.size() < 4 ? 1 : params.at(3) / 10.0);
            if (succeeded) {
                sleep(task->post_delay);
            }
        }
    }
    if (!succeeded && error != nullptr) {
        *error = "scrap trade inventory swipe failed: " + std::string(task_name);
    }
    return succeeded;
}

bool BlackFlowScrapTradeTaskPlugin::click(const Rect& rect)
{
    return ctrler()->click(rect);
}

bool BlackFlowScrapTradeTaskPlugin::execute(std::string_view task, std::string* error)
{
    ProcessTask process(*this, { std::string(task) });
    process.set_retry_times(0);
    m_inventory_last_task.clear();
    const bool succeeded = process.run();
    m_inventory_last_task = process.get_last_task_name();
    if (!succeeded && error != nullptr) {
        *error = "scrap trade sale task failed: " + std::string(task);
    }
    return succeeded;
}

std::string BlackFlowScrapTradeTaskPlugin::last_task() const
{
    return m_inventory_last_task;
}

void BlackFlowScrapTradeTaskPlugin::wait(unsigned milliseconds) const
{
    sleep(milliseconds);
}

void BlackFlowScrapTradeTaskPlugin::on_inventory_item(const ScrapTradeInventoryItem& observed)
{
    const auto item = BlackFlowScrapMarket.find(observed.name);
    if (item && item->get().growth_per_acquisition > 0) {
        LogInfo << "BlackFlow scrap trade growth item observed" << observed.name << "row" << observed.row << "column"
                << observed.column << "growth per acquisition" << item->get().growth_per_acquisition;
    }
}

bool BlackFlowScrapTradeTaskPlugin::on_selling_page(const cv::Mat& image) const
{
    Matcher analyzer(image);
    analyzer.set_task_info(std::string(ScrapTradeSellingFlagTask));
    return analyzer.analyze().has_value();
}

std::vector<TextRect>
    BlackFlowScrapTradeTaskPlugin::recognize(const cv::Mat& image, const std::vector<std::string>& names) const
{
    // 空名单交给 OCR 会匹配任意文字。
    if (names.empty()) {
        return {};
    }
    OCRer analyzer(image);
    analyzer.set_task_info(std::string(ScrapTradeItemsTask));
    analyzer.set_required(names);
    const auto results = analyzer.analyze();
    return results.has_value() ? std::move(*results) : std::vector<TextRect> {};
}

bool BlackFlowScrapTradeTaskPlugin::already_purchased(const Rect& rect) const
{
    const int center_x = rect.x + rect.width / 2;
    const int center_y = rect.y + rect.height / 2;
    return std::ranges::any_of(m_purchased_rects, [&](const Rect& bought) {
        return std::abs(center_x - (bought.x + bought.width / 2)) <= ScrapTradeSameSlotTolerance &&
               std::abs(center_y - (bought.y + bought.height / 2)) <= ScrapTradeSameSlotTolerance;
    });
}

void BlackFlowScrapTradeTaskPlugin::set_action(std::string_view task) const
{
    Task.set_task_base(std::string(ScrapTradeAction), std::string(task));
}
} // namespace asst::blackflow
