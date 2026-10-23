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
        StartRunStarted,
        PickLastReward,
        CheckStartRunReward,
        StartRunRewardPageMissing,
    };

    // 按策略的支援顺序点第一张认出的卡
    bool pick_last_reward();
    bool check_start_run_reward();
    // rewards 是在支援页认出的奖励名，随回调上报给界面
    void report_start_run(std::string_view status, const std::vector<std::string>& rewards = {});
    void stop_start_run(std::string_view status, const std::vector<std::string>& rewards = {});
    void complete_start_run(const std::vector<std::string>& rewards = {});

    std::vector<std::string> m_last_reward_tasks;
    bool m_start_run = false;
    bool m_prepared_run_finished = false;
    std::string m_start_core_char;
    std::string m_target_squad;
    std::string m_preparation_squad;
    std::string m_start_roles;

    mutable PendingWork m_pending = PendingWork::None;
    mutable json::value m_pending_details;
    mutable std::string m_terminal_trigger;
    mutable std::string m_terminal_pre_task;
    bool m_stop_after_abandon = false;
};
} // namespace asst::blackflow
