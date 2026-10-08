#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "AbstractRoguelikeTaskPlugin.h"

namespace asst::blackflow
{
class BlackFlowSession;
}

namespace asst
{
class RoguelikeShoppingTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeShoppingTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

    void set_blackflow_session(std::shared_ptr<blackflow::BlackFlowSession> session)
    {
        m_blackflow_session = std::move(session);
    }

protected:
    virtual bool _run() override;

private:
    enum class PendingWork
    {
        None,
        Buy,
        PurchaseConfirmed,
        ClearPurchase,
    };

    // 购买一次
    bool buy_once();
    // 读不到时返回空，此时不按价格筛选
    std::optional<int> read_wallet(const cv::Mat& image) const;

    std::shared_ptr<blackflow::BlackFlowSession> m_blackflow_session;
    std::optional<std::string> m_pending_purchase;
    mutable PendingWork m_pending = PendingWork::None;
};
}
