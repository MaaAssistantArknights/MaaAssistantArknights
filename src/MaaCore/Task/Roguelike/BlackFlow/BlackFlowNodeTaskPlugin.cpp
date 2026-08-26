#include "BlackFlowNodeTaskPlugin.h"

#include "Config/Roguelike/BlackFlow/BlackFlowNodeExecutionConfig.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst::blackflow
{
namespace
{
// 只有这两种阶段才有东西可派发：PendingDispatch 是首次派发，Running 是重派发。
// 页面已 Resolved 却还没销毁，只出现在对账失败、路由正准备走地图恢复的那一拍；
// 那一拍属于路由，抢过来既掐死了恢复，又会用假原因盖掉真正的失败。
bool dispatchable(PageExecutionStage stage) noexcept
{
    return stage == PageExecutionStage::PendingDispatch || stage == PageExecutionStage::Running;
}
} // namespace

void BlackFlowNodeTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_details = {};
    restore_node_completion_action();
}

void BlackFlowNodeTaskPlugin::restore_node_completion_action()
{
    Task.set_task_base("BlackFlow@Roguelike@NodeCompletionAction", "BlackFlow@Roguelike@MapPrepare");
}

bool BlackFlowNodeTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart &&
        (task == "BlackFlow@Roguelike@RoutingAction" || task == "BlackFlow@Roguelike@NodeRedispatch") &&
        m_session != nullptr && m_session->page_context().has_value() &&
        dispatchable(m_session->page_context()->stage)) {
        m_pending = PendingWork::BindDispatch;
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
    if (msg == AsstMsg::SubTaskCompleted && BlackFlowNodeExecution.get_task_result(task)) {
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
        const auto route = BlackFlowNodeExecution.resolve_route(context);
        std::string error;
        const bool initial_dispatch = page.stage == PageExecutionStage::PendingDispatch;
        const bool stage_valid =
            initial_dispatch ? m_session->mark_page_running(&error) : page.stage == PageExecutionStage::Running;
        if (!route || !stage_valid) {
            m_session->fail(
                "node_dispatch_failed",
                !route ? "node execution config has no matching route"
                       : (error.empty() ? "node redispatch has invalid page stage" : error));
            Task.set_task_base("BlackFlow@Roguelike@NodeDispatchAction", "BlackFlow@Roguelike@RecoveryFailed");
            report_outputs();
            return true;
        }
        Task.set_task_base("BlackFlow@Roguelike@NodeCompletionAction", route->get().completion_task);
        Task.set_task_base(route->get().alias, route->get().task);
        LogInfo << __FUNCTION__ << "BlackFlow node dispatch" << "floor" << page.floor << "node" << page.node << "event"
                << page.node_name << "intent" << page.page_intent << "task" << route->get().task;
        report_outputs();
        return true;
    }

    if (work == PendingWork::ApplyResult) {
        restore_node_completion_action();
        const std::string task = m_pending_details.get("details", "task", "");
        const auto result = BlackFlowNodeExecution.get_task_result(task);
        std::string error;
        std::string next_task = "BlackFlow@Roguelike@RecoveryFailed";
        if (!result || !m_session->apply_node_task_result(result->get(), m_pending_details, &error)) {
            m_session->fail("node_result_failed", !result ? "node result task is not configured" : error);
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", next_task);
        }
        else if (result->get().kind == NodeTaskResultKind::PageCompleted) {
            if (m_session->terminated()) {
                next_task = "BlackFlow@Roguelike@StrategyTerminated-Enter";
            }
            else if (m_session->completed_page_changes_floor()) {
                m_session->clear_current_floor();
                // 经 -Enter 转发，让 NextLevel 以自己的名字执行，楼层识别插件才会收到回调。
                next_task = "BlackFlow@Roguelike@NextLevel-Enter";
            }
            else {
                next_task = "BlackFlow@Roguelike@MapPrepare";
            }
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", next_task);
        }
        else if (result->get().redispatch) {
            Task.set_task_base("BlackFlow@Roguelike@NodeResultAction", "BlackFlow@Roguelike@NodeRedispatch-Enter");
        }
        report_outputs();
        return true;
    }

    if (work == PendingWork::RecoverMapCompleted) {
        restore_node_completion_action();
        Log.info("BlackFlow map page recovery completed");
        report_outputs();
        return true;
    }

    if (work == PendingWork::RecoverMapFailed) {
        restore_node_completion_action();
        m_session->fail("page_recovery_failed", "map recovery task reported failure", FailureDisposition::RestartRun);
        report_outputs();
        return true;
    }
    return true;
}
} // namespace asst::blackflow
