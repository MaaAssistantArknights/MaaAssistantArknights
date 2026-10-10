#include "BlackFlowScrapTrade.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace asst::blackflow
{
void ScrapLedger::reset(const std::vector<std::string>& held_names)
{
    m_entry.clear();
    for (const auto& name : held_names) {
        ++m_entry[name];
    }
    m_held = m_entry;
}

void ScrapLedger::record_purchase(const ScrapItem& item)
{
    ++m_held[item.name];
    for (const auto& [name, count] : item.bundle) {
        m_held[name] += count;
    }
}

void ScrapLedger::record_sale(const std::string& name)
{
    if (auto found = m_held.find(name); found != m_held.end() && found->second > 0) {
        --found->second;
    }
}

int ScrapLedger::held(const std::string& name) const
{
    const auto found = m_held.find(name);
    return found == m_held.end() ? 0 : found->second;
}

int ScrapLedger::growth(const BlackFlowScrapMarketConfig& market) const
{
    int total = 0;
    for (const auto& [name, count] : m_held) {
        if (const auto item = market.find(name)) {
            total += item->get().growth_per_acquisition * count;
        }
    }
    return total;
}

bool ScrapLedger::may_sell(const ScrapItem& item, const std::unordered_set<std::string>& sell_table) const
{
    if (item.category == ScrapCategory::Natural && sell_table.contains(item.name)) {
        return held(item.name) > 0;
    }
    const auto entry = m_entry.find(item.name);
    return held(item.name) > (entry == m_entry.end() ? 0 : entry->second);
}

int ScrapLedger::held_in(ScrapCategory category, const BlackFlowScrapMarketConfig& market) const
{
    int total = 0;
    for (const auto& [name, count] : m_held) {
        if (const auto item = market.find(name); item && item->get().category == category) {
            total += count;
        }
    }
    return total;
}

std::unordered_set<std::string> ScrapLedger::covered_shapes(const BlackFlowScrapMarketConfig& market) const
{
    std::unordered_set<std::string> shapes;
    for (const auto& [name, count] : m_held) {
        if (const auto item = market.find(name); item && count > 0) {
            shapes.insert(item->get().covers.begin(), item->get().covers.end());
        }
    }
    return shapes;
}

bool scrap_keep_wanted(const ScrapItem& item, const ScrapLedger& ledger, const BlackFlowScrapMarketConfig& market)
{
    if (item.category != ScrapCategory::Processing || !item.price ||
        ledger.held_in(ScrapCategory::Processing, market) >= market.processing_keep_limit()) {
        return false;
    }
    if (item.covers.empty()) {
        return true;
    }
    const auto covered = ledger.covered_shapes(market);
    return std::ranges::any_of(item.covers, [&](const std::string& shape) { return !covered.contains(shape); });
}

int scrap_acquisitions(const ScrapItem& item)
{
    int count = 1;
    for (const auto& [name, bundled] : item.bundle) {
        count += bundled;
    }
    return count;
}

int scrap_resale(const ScrapItem& item, const BlackFlowScrapMarketConfig& market)
{
    int value = item.value;
    for (const auto& [name, count] : item.bundle) {
        if (const auto bundled = market.find(name)) {
            value += bundled->get().value * count;
        }
    }
    return value;
}

std::optional<int> scrap_trade_net(const ScrapItem& item, int growth, const BlackFlowScrapMarketConfig& market)
{
    if (!item.price) {
        return std::nullopt;
    }
    return growth * scrap_acquisitions(item) - (*item.price - scrap_resale(item, market));
}

std::optional<std::string>
    infer_scrap_shop_type(const std::vector<std::string>& shelf_names, const BlackFlowScrapMarketConfig& market)
{
    std::array<int, 3> observed {};
    for (const auto& name : shelf_names) {
        if (const auto item = market.find(name)) {
            ++observed[static_cast<std::size_t>(item->get().category)];
        }
    }
    // OCR 可能漏读个别商品，所以只要求观察到的件数不超过布局件数，按相容布局的概率合计比较。
    // 概率是四舍五入后的统计值，同一店型的合计可能差万分之一，差距在此以内按无法区分处理。
    constexpr double ScoreTolerance = 1e-3;
    const ScrapShopType* best = nullptr;
    double best_score = 0;
    bool tied = false;
    for (const auto& type : market.shop_types()) {
        double score = 0;
        for (const auto& layout : type.layouts) {
            if (std::ranges::equal(observed, layout.counts, std::less_equal<> {})) {
                score += layout.probability;
            }
        }
        if (score > best_score + ScoreTolerance) {
            best = &type;
            best_score = score;
            tied = false;
        }
        else if (score > 0 && std::abs(score - best_score) <= ScoreTolerance) {
            tied = true;
        }
    }
    if (best == nullptr || tied) {
        return std::nullopt;
    }
    return best->id;
}

double expected_scrap_page_profit(const ScrapShopType& type, int growth, const BlackFlowScrapMarketConfig& market)
{
    double total = 0;
    for (const auto& [name, rate] : type.offer_rates) {
        const auto item = market.find(name);
        if (!item) {
            continue;
        }
        if (const auto net = scrap_trade_net(item->get(), growth, market); net && *net > 0) {
            total += rate * *net;
        }
    }
    return total;
}

std::optional<int>
    cheapest_profitable_price(const ScrapShopType& type, int growth, const BlackFlowScrapMarketConfig& market)
{
    std::optional<int> cheapest;
    for (const auto& [name, rate] : type.offer_rates) {
        const auto item = market.find(name);
        if (!item || rate <= 0) {
            continue;
        }
        if (const auto net = scrap_trade_net(item->get(), growth, market); net && *net > 0) {
            const int price = *item->get().price;
            cheapest = cheapest ? std::min(*cheapest, price) : price;
        }
    }
    return cheapest;
}
} // namespace asst::blackflow
