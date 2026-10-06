#pragma once

#include <deque>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "BlackFlowScrapTrade.h"
#include "BlackFlowScrapTradeInventory.h"
#include "BlackFlowTaskPluginBase.h"
#include "Common/AsstTypes.h"

namespace asst::blackflow
{
// 秘境行商交易：持有板藤或多生苔藓时买入后卖回让它们增值；兑现店最后卖出全部可售自然物。
class BlackFlowScrapTradeTaskPlugin final : public BlackFlowTaskPluginBase, private BlackFlowScrapTradeInventoryContext
{
public:
    using BlackFlowTaskPluginBase::BlackFlowTaskPluginBase;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    enum class PendingWork
    {
        None,
        Enter,
        Decide,
        ClickAtMost,
        BuyConfirmed,
        RefreshConfirmed,
    };

    enum class Phase
    {
        Cultivate,
        Observe,
        Prepare,
        Trade,
        Liquidate,
        Leave,
    };

    struct PendingPurchase
    {
        std::string name;
        Rect rect;
        bool keep = false;
    };

    void enter();
    void decide();
    void on_purchase_confirmed();
    void on_refresh_confirmed();

    // 当前页面与所需页面不同时切换，返回是否已安排切换动作。
    [[nodiscard]] bool switch_tab(bool want_selling, bool selling);
    [[nodiscard]] bool sell_matching(bool liquidating);
    [[nodiscard]] bool sell_back();
    [[nodiscard]] bool buy(const cv::Mat& image, int wallet);
    bool complete_sale(const ScrapTradeInventoryTarget& target, bool sell_back);
    void click_at_most();
    [[nodiscard]] bool refresh(int wallet);
    void leave();

    [[nodiscard]] bool on_selling_page(const cv::Mat& image) const;
    [[nodiscard]] std::vector<TextRect> recognize(const cv::Mat& image, const std::vector<std::string>& names) const;
    [[nodiscard]] bool already_purchased(const Rect& rect) const;
    void set_action(std::string_view task) const;

    [[nodiscard]] cv::Mat capture() const override;
    bool interrupted() const override;
    bool precise_swipe_supported() const override;
    bool swipe_by(std::string_view task, std::optional<int> distance, std::string* error) override;
    bool click(const Rect& rect) override;
    bool execute(std::string_view task, std::string* error) override;
    std::string last_task() const override;
    void wait(unsigned milliseconds) const override;
    void on_inventory_item(const ScrapTradeInventoryItem& item) override;

    BlackFlowScrapTradeInventory m_inventory;
    std::vector<std::string> m_inventory_names;
    mutable PendingWork m_pending = PendingWork::None;
    Phase m_phase = Phase::Cultivate;
    ScrapLedger m_ledger;
    bool m_counted = false;
    std::unordered_set<std::string> m_sell_table;
    bool m_liquidating = false;
    std::deque<std::string> m_sell_backs;
    std::string m_inventory_last_task;
    std::optional<PendingPurchase> m_pending_purchase;
    bool m_pending_refresh = false;
    std::vector<Rect> m_purchased_rects;
    std::unordered_set<std::string> m_kept;
    std::optional<std::string> m_shop_type;
    int m_refresh_count = 0;
    int m_toggle_attempts = 0;
};
} // namespace asst::blackflow
