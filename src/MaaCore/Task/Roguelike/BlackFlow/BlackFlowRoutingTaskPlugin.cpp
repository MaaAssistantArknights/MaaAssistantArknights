#include "BlackFlowRoutingTaskPlugin.h"

#include "BlackFlowRoutingLoop.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst::blackflow
{
bool BlackFlowRoutingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask" ||
        details.get("details", "task", "") != "BlackFlow@Roguelike@Routing") {
        return false;
    }
    m_routing_pending = true;
    return true;
}

void BlackFlowRoutingTaskPlugin::reset_in_run_variables()
{
    m_routing_pending = false;
    m_page_recovery_attempted = false;
}

bool BlackFlowRoutingTaskPlugin::_run()
{
    LogTraceFunction;
    if (!m_routing_pending) {
        return true;
    }
    m_routing_pending = false;
    Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@RecoveryFailed");

    if (m_session == nullptr || m_port == nullptr) {
        if (m_session != nullptr) {
            m_session->fail("perception_port_missing", "BlackFlow perception and task port is not attached");
        }
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated");
        report_outputs();
        return true;
    }

    const RoutingCycleOutcome cycle = execute_routing_cycle(*m_session, *m_port);
    if (cycle.status == RoutingCycleStatus::NeedsPageRecovery) {
        if (!m_page_recovery_attempted) {
            m_page_recovery_attempted = true;
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@RecoverMap");
        }
        else {
            m_page_recovery_attempted = false;
            m_session->fail("map_rebuild_failed", cycle.error);
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated");
        }
        report_outputs();
        return true;
    }
    m_page_recovery_attempted = false;
    if (cycle.status == RoutingCycleStatus::PreviewNeedsDismiss) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@DismissMovePreview");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::MoveCommitted) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
        report_outputs();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::SessionTerminated) {
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated");
        report_outputs();
        return true;
    }

    m_session->fail(cycle.failure_code, cycle.error);
    Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@StrategyTerminated");
    report_outputs();
    return true;
}
} // namespace asst::blackflow
