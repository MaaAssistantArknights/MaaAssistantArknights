#include "BlackFlowLifecycleTaskPlugin.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "Config/TaskData.h"

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
namespace
{
std::pair<std::string, std::string> classify_unreported_termination(const std::string& pre_task)
{
    if (pre_task == "BlackFlow@Roguelike@RecoveryFailed") {
        return { "state_machine_dead_end", "task chain reached RecoveryFailed without a strategy result" };
    }
    if (pre_task == "BlackFlow@Roguelike@RecoverMapFailed") {
        return { "map_recovery_exhausted", "map recovery was exhausted without a strategy result" };
    }
    return { "internal_failure", "terminal action was requested without a strategy result" };
}
} // namespace

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
    if (is_baby_animal_profile(profile)) {
        const std::string target_text = params.get("blackflow_cultivation_target", std::string("swaddled_cat"));
        const auto target = parse_cultivated_animal_type(target_text);
        if (!target.has_value()) {
            Log.error("Invalid BlackFlow cultivation target:", target_text);
            return false;
        }
        m_session->set_cultivation_target(*target);
        // 界面只传「刷襁褓动物」这一个模式，收工条件随目标动物变化，所以在这里分流到对应策略。
        profile = baby_animal_profile_for(*target);
    }
    const std::string selected_profile = profile;

    // 三项都直接读 params：分队要等真正在选择界面点中才会写回 RoguelikeConfig，此刻取不到。
    // 必须先于 initialize()，事实是在它末尾写入的。
    m_session->set_start_loadout(
        params.get("core_char", std::string()),
        params.get("squad", std::string()),
        params.get("roles", std::string()));

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

    Log.info("BlackFlow strategy initialized", "profile", selected_profile);
    auto info = basic_info_with_what("BlackFlowStrategyStarted");
    info["details"] = json::object { { "profile", selected_profile } };
    callback(AsstMsg::SubTaskExtraInfo, info);
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
        m_terminal_pre_task.clear();
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@AbandonConfirm") {
        m_pending = PendingWork::ResetAfterAbandon;
        m_pending_details = {};
        m_terminal_trigger.clear();
        m_terminal_pre_task.clear();
        return true;
    }
    if (msg != AsstMsg::SubTaskStart || task != "BlackFlow@Roguelike@StrategyTerminated") {
        return false;
    }

    m_pending = PendingWork::ResolveTerminalAction;
    m_pending_details = {};
    m_terminal_trigger = task;
    m_terminal_pre_task = details.get("pre_task", "");
    return true;
}

void BlackFlowLifecycleTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();
    m_terminal_pre_task.clear();
    m_stop_after_abandon = false;
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", "BlackFlow@Roguelike@ExitThenAbandon-Enter");
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
    const std::string pre_task = std::move(m_terminal_pre_task);
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_terminal_trigger.clear();
    m_terminal_pre_task.clear();

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
        const bool stop_after_abandon = m_stop_after_abandon;
        reset_in_run_variables();
        Log.info("BlackFlow run state reset after abandonment");
        if (stop_after_abandon && m_task_ptr != nullptr) {
            m_task_ptr->set_enable(false);
            Log.info("BlackFlow task stopped after abandonment");
        }
        return true;
    }
    if (work != PendingWork::ResolveTerminalAction) {
        return true;
    }

    if (m_session != nullptr && !m_session->terminated()) {
        auto [outcome, reason] = classify_unreported_termination(pre_task);
        Log.error("BlackFlow terminated without a strategy result", trigger, pre_task, outcome, reason);
        m_session->fail(std::move(outcome), std::move(reason), FailureDisposition::StopTask);
    }

    std::string profile;
    std::string outcome;
    std::string reason;
    if (m_session != nullptr && m_session->result().has_value()) {
        profile = m_session->result()->profile;
        outcome = m_session->result()->outcome;
        reason = m_session->result()->termination_reason;
    }

    const std::string next_action =
        m_session != nullptr && m_session->result().has_value() ? m_session->result()->next_action : "stop_run";
    // 两支都在主 ProcessTask 上完成放弃；停止一支在 AbandonConfirm 完成后、StartExplore 之前停用主任务。
    m_stop_after_abandon = next_action != "restart_current_run";
    const std::string task = "BlackFlow@Roguelike@ExitThenAbandon-Enter";
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", task);
    Log.info(
        "BlackFlow strategy terminal action",
        "profile",
        profile,
        trigger,
        pre_task,
        outcome,
        reason,
        next_action,
        task);
    report_outputs();
    return true;
}

} // namespace asst::blackflow
