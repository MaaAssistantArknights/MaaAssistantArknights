#pragma once

#include "BlackFlowTaskPluginBase.h"

namespace asst::blackflow
{
class BlackFlowNodeTaskPlugin final : public BlackFlowTaskPluginBase
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
        BindDispatch,
        BeginRecovery,
        ApplyResult,
        RecoverMapCompleted,
        RecoverMapFailed,
    };

    void restore_legacy_stages();

    mutable PendingWork m_pending = PendingWork::None;
    mutable json::value m_pending_details;
};
} // namespace asst::blackflow
