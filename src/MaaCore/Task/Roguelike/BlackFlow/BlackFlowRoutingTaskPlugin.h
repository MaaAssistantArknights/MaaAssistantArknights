#pragma once

#include "BlackFlowTaskPluginBase.h"

namespace asst::blackflow
{
class BlackFlowRoutingTaskPlugin final : public BlackFlowTaskPluginBase
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
        ObserveAndPlan,
        ResumePendingMove,
    };

    mutable PendingWork m_pending = PendingWork::None;
    bool m_page_recovery_attempted = false;
    int m_move_confirmation_dismiss_retries = 0;
};
} // namespace asst::blackflow
