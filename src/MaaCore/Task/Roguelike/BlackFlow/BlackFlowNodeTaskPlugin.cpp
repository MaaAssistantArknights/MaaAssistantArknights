#include "BlackFlowNodeTaskPlugin.h"

#include "Config/Roguelike/BlackFlow/BlackFlowNodeExecutionConfig.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst::blackflow
{
bool BlackFlowNodeTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart &&
        (task == "BlackFlow@Roguelike@RoutingAction" || task == "BlackFlow@Roguelike@NodeRedispatch") &&
        m_session != nullptr && m_session->page_context().has_value()) {
        m_pending = PendingWork::BindDispatch;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskStart && task == "BlackFlow@Roguelike@PageFailure") {
        m_pending = PendingWork::BeginRecovery;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@RecoverMapCompleted") {
        m_pending = PendingWork::RecoverMapCompleted;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@RecoverMapFailed") {
        m_pending = PendingWork::RecoverMapFailed;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && BlackFlowNodeExecution.get_task_result(task) != nullptr) {
        m_pending = PendingWork::ApplyResult;
        m_pending_details = details;
        return true;
    }
    return false;
}

bool BlackFlowNodeTaskPlugin::_run()
{
    LogTraceFunction;
    const PendingWork work = m_pending;
    m_pending = PendingWork::None;

    if (m_session == nullptr) {
        Task.set_task_base("BlackFlow@Roguelike@NodeDispatchAction", "BlackFlow@Roguelike@RecoveryFailed");
        return true;
    }

    if (work == PendingWork::BindDispatch) {
        if (!m_session->page_context().has_value()) {
            m_session->fail("node_dispatch_failed", "node dispatch has no active page context");
            Task.set_task_base("BlackFlow@Roguelike@NodeDispatchAction", "BlackFlow@Roguelike@RecoveryFailed");
            report_outputs();
            return true;
        }
        const PageExecutionContext& page = *m_session->page_context();
        const NodeExecutionContext context {
            page.floor,
            page.node_type,
            page.node_name,
            page.page_intent,
        };
        const NodeExecutionRoute* route = BlackFlowNodeExecution.resolve_route(context);
        std::string error;
        const bool initial_dispatch = page.stage == PageExecutionStage::PendingDispatch;
        const bool stage_valid =
            initial_dispatch ? m_session->mark_page_running(&error) : page.stage == PageExecutionStage::Running;
        if (route == nullptr || !stage_valid) {
            m_session->fail(
                "node_dispatch_failed",
                route == nullptr ? "node execution config has no matching route"
                                 : (error.empty() ? "node redispatch has invalid page stage" : error));
            Task.set_task_base("BlackFlow@Roguelike@NodeDispatchAction", "BlackFlow@Roguelike@RecoveryFailed");
            report_outputs();
            return true;
        }
        Task.set_task_base(route->alias, route->task);
        Log.info(
            "BlackFlow node dispatch",
            "floor",
            page.floor,
            "node",
            page.node,
            "event",
            page.node_name,
            "intent",
            page.page_intent,
            "task",
            route->task);
        report_outputs();
        return true;
    }

    if (work == PendingWork::BeginRecovery) {
        std::string error;
        if (!m_session->mark_page_recovery(&error)) {
            m_session->fail("page_recovery_failed", error);
            Task.set_task_base("BlackFlow@Roguelike@RecoverMap", "BlackFlow@Roguelike@RecoverMapFailed");
        }
        report_outputs();
        return true;
    }

    if (work == PendingWork::ApplyResult) {
        const std::string task = m_pending_details.get("details", "task", "");
        const NodeTaskResult* result = BlackFlowNodeExecution.get_task_result(task);
        std::string error;
        std::string next_task = "BlackFlow@Roguelike@RecoveryFailed";
        if (result == nullptr || !m_session->apply_node_task_result(*result, m_pending_details, &error)) {
            m_session->fail("node_result_failed", result == nullptr ? "node result task is not configured" : error);
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", next_task);
        }
        else if (result->kind == NodeTaskResultKind::PageCompleted) {
            next_task =
                m_session->terminated() ? "BlackFlow@Roguelike@StrategyTerminated" : "BlackFlow@Roguelike@Routing";
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", next_task);
        }
        else if (result->redispatch) {
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", "BlackFlow@Roguelike@NodeRedispatch");
        }
        report_outputs();
        return true;
    }

    if (work == PendingWork::RecoverMapCompleted) {
        Log.info("BlackFlow map page recovery completed");
        report_outputs();
        return true;
    }

    if (work == PendingWork::RecoverMapFailed) {
        m_session->fail("page_recovery_failed", "map recovery task reported failure");
        report_outputs();
        return true;
    }
    return true;
}
} // namespace asst::blackflow
