#pragma once

#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstTypes.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace asst
{
class EventShopTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~EventShopTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_blacklist(std::vector<std::string> blacklist);

protected:
    virtual bool _run() override;

private:
    enum class PurchaseResult
    {
        Continue,
        Finished,
        Failed,
    };

    std::optional<MatchRect> select_commodity(const cv::Mat& image) const;
    std::optional<std::string> recognize_commodity_name(const cv::Mat& image, const Rect& commodity) const;
    bool is_blacklisted(std::string_view commodity_name) const;
    PurchaseResult purchase_selected_commodity() const;
    bool swipe_store() const;

    std::vector<std::string> m_blacklist;

    inline static constexpr int MaxSwipeTimes = 10;
    inline static constexpr int MaxPurchaseTimes = 100;
};
} // namespace asst
