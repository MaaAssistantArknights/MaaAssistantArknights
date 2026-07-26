#include "BlackFlowLifecycleTaskPlugin.h"

#include <algorithm>
#include <iterator>

#include "Config/TaskData.h"

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
bool BlackFlowLifecycleTaskPlugin::load_params(const json::value& params)
{
    if (!BlackFlowTaskPluginBase::load_params(params)) {
        return false;
    }

    std::string profile = params.get("blackflow_strategy", std::string {});
    if (profile.empty()) {
        if (m_config->get_mode() == RoguelikeMode::Investment) {
            profile = "investment";
        }
        else {
            profile = params.get("investment_enabled", true) ? "burn_with_investment" : "burn";
        }
    }

    std::string error;
    if (!m_session->initialize(std::move(profile), &error)) {
        Log.error("BlackFlow strategy initialization failed:", error);
        return false;
    }

    const std::string diagnostics_text = params.get("blackflow_diagnostics", std::string("normal"));
    const auto diagnostics = parse_diagnostic_level(diagnostics_text);
    const int image_limit = params.get("blackflow_diagnostic_image_limit", 3);
    if (!diagnostics.has_value() || image_limit < 0 || image_limit > 100) {
        Log.error("Invalid BlackFlow diagnostics parameters");
        return false;
    }
    const DiagnosticSettings settings { *diagnostics, static_cast<std::size_t>(image_limit) };
    if (!m_session->configure_diagnostics(settings, &error)) {
        Log.error("BlackFlow diagnostics initialization failed:", error);
        return false;
    }
    if (m_port != nullptr) {
        m_port->configure_diagnostics(settings);
    }
    return true;
}

bool BlackFlowLifecycleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@NextLevel") {
        m_pending = PendingWork::RecordCurrentFloor;
        m_pending_details = details;
        m_terminal_trigger.clear();
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@AbandonConfirm") {
        m_pending = PendingWork::ResetAfterAbandon;
        m_pending_details = {};
        m_terminal_trigger.clear();
        return true;
    }
    if (msg != AsstMsg::SubTaskStart || task != "BlackFlow@Roguelike@StrategyTerminated") {
        return false;
    }

    m_pending = PendingWork::ResolveTerminalAction;
    m_pending_details = {};
    m_terminal_trigger = task;
    return true;
}

void BlackFlowLifecycleTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", "RoguelikeControlTaskPlugin-ExitThenStop");
    if (m_session != nullptr && !m_session->profile().empty()) {
        m_session->reset_run();
    }
    if (m_port != nullptr) {
        m_port->reset_run();
    }
}

bool BlackFlowLifecycleTaskPlugin::_run()
{
    const PendingWork work = m_pending;
    const json::value details = std::move(m_pending_details);
    const std::string trigger = std::move(m_terminal_trigger);
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();

    if (work == PendingWork::RecordCurrentFloor) {
        if (m_session == nullptr) {
            Log.error("BlackFlow floor recognition has no active session");
            return false;
        }
        const std::string area_name = details.get("details", "result", "text", "");
        const auto task = Task.get<OcrTaskInfo>("BlackFlow@Roguelike@NextLevel");
        if (task == nullptr || area_name.empty()) {
            m_session->clear_current_floor();
            m_session->fail(
                "floor_recognition_failed",
                "NextLevel completed without a recognized floor name",
                FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel floor recognition result is missing");
            report_outputs();
            return true;
        }
        const auto& floor_names = task->text;
        const auto matched = std::ranges::find(floor_names, area_name);
        if (matched == floor_names.end()) {
            m_session->clear_current_floor();
            m_session->fail(
                "floor_recognition_failed",
                "NextLevel returned an unconfigured floor name: " + area_name,
                FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel returned an unconfigured floor name", area_name);
            report_outputs();
            return true;
        }
        const int floor = static_cast<int>(std::distance(floor_names.begin(), matched)) + 1;
        std::string error;
        if (!m_session->set_current_floor(floor, &error)) {
            m_session->fail("floor_recognition_failed", error, FailureDisposition::RestartRun);
            Log.error("BlackFlow NextLevel floor recognition failed", area_name, error);
            report_outputs();
            return true;
        }
        Log.info("BlackFlow current floor recognized", "floor", floor, "area", area_name);
        report_outputs();
        return true;
    }
    if (work == PendingWork::ResetAfterAbandon) {
        reset_in_run_variables();
        Log.info("BlackFlow run state reset after abandonment");
        return true;
    }
    if (work != PendingWork::ResolveTerminalAction) {
        return true;
    }

    if (m_session != nullptr && !m_session->terminated()) {
        m_session->fail(
            "internal_failure",
            "terminal action was requested without a strategy result",
            FailureDisposition::StopTask);
    }

    const std::string next_action =
        m_session != nullptr && m_session->result().has_value() ? m_session->result()->next_action : "stop_run";
    const std::string task = next_action == "restart_current_run" ? "BlackFlow@Roguelike@ExitThenAbandon"
                                                                  : "RoguelikeControlTaskPlugin-ExitThenStop";
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", task);
    Log.info("BlackFlow strategy terminal action", trigger, next_action, task);
    report_outputs();
    return true;
}

} // namespace asst::blackflow
