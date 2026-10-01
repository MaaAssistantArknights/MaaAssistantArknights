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
        MapPrepared,
        ObserveAndPlan,
        ResumePendingMove,
        ReplanCurrentMap,
    };

    mutable PendingWork m_pending = PendingWork::None;
    bool m_page_recovery_attempted = false;
    // 当前选点阶段跨回调沿用更新后的地图，直到行动提交、背包清理、会话终止或重置。
    bool m_replan_current_map = false;
    int m_move_confirmation_dismiss_retries = 0;
};
} // namespace asst::blackflow
