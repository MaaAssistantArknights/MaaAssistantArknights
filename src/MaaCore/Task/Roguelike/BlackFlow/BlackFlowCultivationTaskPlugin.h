#pragma once

#include <optional>
#include <string>
#include <vector>

#include "BlackFlowTaskPluginBase.h"
#include "Common/AsstTypes.h"

namespace asst::blackflow
{
class BlackFlowCultivationTaskPlugin final : public BlackFlowTaskPluginBase
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
        SellDecision,
        BuyDecision,
        BuyConfirmed,
        RefreshCompleted,
        ClickAtMost,
        ReadHarvest,
        ApplyCultivationResult,
    };

    [[nodiscard]] std::string sell_items_task() const;
    [[nodiscard]] std::vector<TextRect> recognize(const cv::Mat& image, const std::string& task) const;
    [[nodiscard]] int read_number(const cv::Mat& image, const std::string& task) const;
    void bind_completion(const std::string& task) const;
    void apply_cultivation_result(const std::string& completion_task);
    [[nodiscard]] bool already_purchased(const Rect& rect) const;

    mutable PendingWork m_pending = PendingWork::None;
    mutable json::value m_pending_details;
    std::optional<Rect> m_pending_purchase;
    std::vector<Rect> m_purchased_shelf_rects;
    int m_refresh_count = 0;
    int m_cultivated_animals = 0;
    std::vector<CultivatedAnimalType> m_cultivated_animal_types;
};
} // namespace asst::blackflow
