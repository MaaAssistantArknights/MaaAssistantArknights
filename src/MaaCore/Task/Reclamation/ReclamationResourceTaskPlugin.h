#pragma once

#include <optional>
#include <unordered_set>

#include "AbstractReclamationTaskPlugin.h"

namespace asst
{
class ReclamationResourceTaskPlugin : public AbstractReclamationTaskPlugin
{
public:
    using AbstractReclamationTaskPlugin::AbstractReclamationTaskPlugin;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;

    void reset();

    bool failed() const noexcept { return m_failed; }

protected:
    virtual bool _run() override;

    virtual bool on_run_fails() override { return false; }

private:
    enum class ShopItem
    {
        Water,
        Stone,
    };

    bool review_day();
    bool review_events();
    bool review_operators();
    bool begin_smelting();
    bool review_smelting();
    bool finish_smelting();
    bool review_exchange();
    bool review_purchase_amount();
    bool verify_purchase();
    bool finish_shop_item();
    bool review_supplies(bool after_cooking);
    bool review_cooking();
    bool review_reward(
        int expected,
        const std::string& next_task,
        const std::string& reason,
        const std::string& ocr_task = "RewardAmountOcr");

    std::optional<int> read_number(const std::string& task_name) const;
    bool next(const std::string& task_name);
    bool fail(const std::string& reason, const std::string& detail = {});

    bool m_failed = false;
    int m_day = 0;
    std::optional<int> m_expected_day;
    std::string m_selected_event;
    std::unordered_set<std::string> m_dispatched_events;
    int m_smelting_recipe = 0;
    int m_smelting_batches = 0;
    ShopItem m_shop_item = ShopItem::Water;
    int m_purchase_amount = 0;
    int m_purchase_stock = 0;
    int m_purchase_unit = 0;
    int m_drinks_before_cooking = 0;
    int m_prepared_drinks = -1;
    int m_ingredient_holds = 0;
    int m_stalled_holds = 0;
};
} // namespace asst
