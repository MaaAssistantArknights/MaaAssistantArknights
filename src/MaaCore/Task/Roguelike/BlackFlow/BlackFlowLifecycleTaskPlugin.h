#pragma once

#include "BlackFlowTaskPluginBase.h"

namespace asst::blackflow
{
class BlackFlowLifecycleTaskPlugin final : public BlackFlowTaskPluginBase
{
public:
    using BlackFlowTaskPluginBase::BlackFlowTaskPluginBase;

    virtual bool load_params(const json::value& params) override;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    enum class PendingWork
    {
        None,
        RecordCurrentFloor,
        ResolveTerminalAction,
        ResetAfterAbandon,
    };

    mutable PendingWork m_pending = PendingWork::None;
    mutable json::value m_pending_details;
    mutable std::string m_terminal_trigger;
};
} // namespace asst::blackflow
