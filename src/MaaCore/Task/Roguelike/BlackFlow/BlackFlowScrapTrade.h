#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Config/Roguelike/BlackFlow/BlackFlowScrapMarketConfig.h"

namespace asst::blackflow
{
// 秘境行商店内的零件台账。进店时按卖出页计数，此后只随确认完成的买卖变化。
class ScrapLedger
{
public:
    void reset(const std::vector<std::string>& held_names);
    void record_purchase(const ScrapItem& item);
    void record_sale(const std::string& name);

    [[nodiscard]] int held(const std::string& name) const;
    // 当前持有的增长物每获得一个零件带来的估价增长合计。
    [[nodiscard]] int growth(const BlackFlowScrapMarketConfig& market) const;
    // 卖表内的自然物可以出售；其余零件只能卖回本次买入的件数，离店时持有件数不少于进店时。
    [[nodiscard]] bool may_sell(const ScrapItem& item, const std::unordered_set<std::string>& sell_table) const;
    [[nodiscard]] int held_in(ScrapCategory category, const BlackFlowScrapMarketConfig& market) const;
    // 当前持有的加工品已经覆盖的移动形状。
    [[nodiscard]] std::unordered_set<std::string> covered_shapes(const BlackFlowScrapMarketConfig& market) const;

private:
    std::unordered_map<std::string, int> m_entry;
    std::unordered_map<std::string, int> m_held;
};

// 用途购买是否需要这件加工品：持有数未到上限，且它不限形状，或能补上缺少的形状。
[[nodiscard]] bool
    scrap_keep_wanted(const ScrapItem& item, const ScrapLedger& ledger, const BlackFlowScrapMarketConfig& market);

// 获得该商品计入的零件获得次数，包括附带零件。
[[nodiscard]] int scrap_acquisitions(const ScrapItem& item);

// 买入后立即卖回能收回的估价，包括附带零件。
[[nodiscard]] int scrap_resale(const ScrapItem& item, const BlackFlowScrapMarketConfig& market);

// 买入后卖回的净收益：增长物获得的估价减去现金差额。不在货架出售的零件没有结果。
[[nodiscard]] std::optional<int>
    scrap_trade_net(const ScrapItem& item, int growth, const BlackFlowScrapMarketConfig& market);

// 按首页的类别件数判断店型，无法区分时不给结果。
[[nodiscard]] std::optional<std::string>
    infer_scrap_shop_type(const std::vector<std::string>& shelf_names, const BlackFlowScrapMarketConfig& market);

// 按出现率估计刷新后一页的交易利润，只计正收益商品，不计钱包约束。
[[nodiscard]] double
    expected_scrap_page_profit(const ScrapShopType& type, int growth, const BlackFlowScrapMarketConfig& market);

// 店型中收益为正的商品里最低的购入价，用于判断刷新后是否还买得起。
[[nodiscard]] std::optional<int>
    cheapest_profitable_price(const ScrapShopType& type, int growth, const BlackFlowScrapMarketConfig& market);
} // namespace asst::blackflow
