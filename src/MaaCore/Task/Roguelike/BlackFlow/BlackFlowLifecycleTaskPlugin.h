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
        CheckStartRunReward,
        StartRunRewardPageMissing,
        CheckShopStock,
        DecideShopTarget,
        TakeNoCollectDrops,
    };

    bool check_start_run_reward();
    // rewards 是在支援页认出的奖励名，随回调上报给界面
    void report_start_run(std::string_view status, const std::vector<std::string>& rewards = {});
    void stop_start_run(std::string_view status, const std::vector<std::string>& rewards = {});
    void complete_start_run(const std::vector<std::string>& rewards = {});

    // 无藏刷开局：第一层诡意行商进货时记下有没有目标藏品，离店回到地图后再决定保留还是重开
    bool check_shop_stock();
    bool decide_shop_target();

    // 无藏刷开局：战斗掉落是一排卡片，只拿源石锭和招募券，剩下的藏品点｢直接离开｣
    bool take_no_collect_drops();

    bool m_start_run = false;
    bool m_no_collect_start_run = false;
    // 界面上的 ｢投资源石锭｣：无藏刷开局没刷到目标藏品时先存钱再离店
    bool m_investment_enabled = false;
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
    bool m_shop_checked = false;
    bool m_shop_target_seen = false;
    mutable bool m_shop_decision_pending = false;
};
} // namespace asst::blackflow
