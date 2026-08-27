#include "BlackFlowRoutingTaskPlugin.h"

#include "BlackFlowRoutingLoop.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst::blackflow
{
namespace
{
// 首次完整确认耗尽时关闭预览面板并重新选择；再次耗尽时重开当前局。
constexpr int MoveConfirmationDismissRetryLimit = 1;
}

bool BlackFlowRoutingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (task == "BlackFlow@Roguelike@Routing") {
        m_pending = PendingWork::ObserveAndPlan;
        return true;
    }
    if (task == "BlackFlow@Roguelike@RoutingResume") {
        m_pending = PendingWork::ResumePendingMove;
        return true;
    }
    return false;
}

void BlackFlowRoutingTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_page_recovery_attempted = false;
    m_move_confirmation_dismiss_retries = 0;
}

bool BlackFlowRoutingTaskPlugin::_run()
{
    LogTraceFunction;
    const PendingWork work = m_pending;
    m_pending = PendingWork::None;
    if (work == PendingWork::None) {
        return true;
    }
    Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@RecoveryFailed");

    if (m_session == nullptr || m_port == nullptr) {
        if (m_session != nullptr) {
            m_session->fail("perception_port_missing", "BlackFlow perception and task port is not attached");
        }
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated-Enter");
        report_outputs();
        return true;
    }

    const RoutingCycleOutcome cycle = work == PendingWork::ResumePendingMove
                                          ? execute_pending_routing_cycle(*m_session, *m_port)
                                          : execute_routing_cycle(*m_session, *m_port);
    if (cycle.status == RoutingCycleStatus::NeedsPageRecovery) {
        const bool completed_page = m_session->page_context().has_value() &&
                                    m_session->page_context()->stage == PageExecutionStage::Resolved &&
                                    m_session->transaction() != nullptr &&
                                    m_session->transaction()->stage() == MoveTransactionStage::PageResolved;
        if (completed_page && !m_page_recovery_attempted) {
            m_page_recovery_attempted = true;
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@RecoverMap-Enter");
        }
        else {
            m_page_recovery_attempted = false;
            m_session->fail("map_rebuild_failed", cycle.error, FailureDisposition::RestartRun);
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated-Enter");
        }
        report_outputs();
        return true;
    }
    m_page_recovery_attempted = false;
    if (cycle.status == RoutingCycleStatus::MovementInventoryObservationRequired) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@MovementInventoryCheck");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::MovementSelectionRequired) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@SelectMovement-Enter");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::ReplanRequired) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@MapPrepare");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::PreviewNeedsDismiss) {
        if (!cycle.failure_code.empty() || !cycle.error.empty()) {
            Log.info("BlackFlow move preview dismissed", cycle.failure_code, cycle.error);
        }
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@CancelNodeSelection");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::ConfirmationNeedsDismiss) {
        if (m_move_confirmation_dismiss_retries < MoveConfirmationDismissRetryLimit) {
            ++m_move_confirmation_dismiss_retries;
            Log.info(
                "BlackFlow move confirmation exhausted; dismissing preview before retry",
                "attempt",
                m_move_confirmation_dismiss_retries,
                "of",
                MoveConfirmationDismissRetryLimit,
                cycle.error);
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@CancelNodeSelection");
        }
        else {
            m_session->fail(cycle.failure_code, cycle.error, FailureDisposition::RestartRun);
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated-Enter");
        }
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::MoveCommittedToMap) {
        m_move_confirmation_dismiss_retries = 0;
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@MapPrepare");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::MoveCommitted) {
        m_move_confirmation_dismiss_retries = 0;
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::SessionTerminated) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated-Enter");
        report_outputs();
        return true;
    }

    m_session->fail(cycle.failure_code, cycle.error, FailureDisposition::RestartRun);
    Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated-Enter");
    report_outputs();
    return true;
}
} // namespace asst::blackflow
