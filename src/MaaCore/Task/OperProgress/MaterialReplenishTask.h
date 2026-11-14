#pragma once

#include "Task/AbstractTask.h"

#include <cstdint>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/ItemConfig.h"

namespace asst
{
enum class MaterialPurchaseSource
{
    GreenTickets,
    ParametricModels,
    PremiumVoucher,
    AdvancedVoucher,
};

struct MaterialPurchaseOptions
{
    std::vector<MaterialPurchaseSource> sources;

    bool enabled() const noexcept { return !sources.empty(); }

    bool can_buy(const std::string& item_id) const;
    bool supports(MaterialPurchaseSource source, const std::string& item_id) const;
    static std::optional<MaterialPurchaseOptions> parse(const json::value& params);
};

class MaterialReplenishTask final : public AbstractTask
{
public:
    MaterialReplenishTask(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::string& item_id,
        int count,
        const MaterialPurchaseOptions& options) :
        AbstractTask(callback, inst, task_chain),
        m_item_id(item_id),
        m_count(count),
        m_options(options)
    {
        set_retry_times(0);
    }

    std::optional<std::int64_t> get_acquired() const noexcept { return m_acquired; }

protected:
    bool _run() override;

private:
    std::optional<std::int64_t>
        replenish_material(const std::string& item_id, int count, const MaterialPurchaseOptions& options);
    bool run_material_purchase_task(const std::string& name, int retry_times = RetryTimesDefault);
    std::optional<std::int64_t> buy_material_with_tickets(const std::string& item_id, int count, bool parametric);
    std::optional<std::int64_t> redeem_material_voucher(const std::string& item_id, int count, bool premium);
    std::optional<std::int64_t> purchase_selected_material(
        const std::string& item_id,
        int count,
        const ItemConfig::TicketShopItem& limits,
        MaterialPurchaseSource source);
    void report_material_purchase(const std::string& item_id, std::int64_t count, int cost, const std::string& what);
    static std::optional<int> read_remaining_stock(const cv::Mat& image);

    std::string m_item_id;
    int m_count = 0;
    const MaterialPurchaseOptions& m_options;
    std::optional<std::int64_t> m_acquired;
};
}
