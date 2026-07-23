#include "BlackFlowLifecycleTaskPlugin.h"

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
        profile = m_config->get_mode() == RoguelikeMode::Investment ? "investment" : "burn";
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
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask" ||
        details.get("details", "task", "") != "BlackFlow@Roguelike@StrategyTerminated") {
        return false;
    }
    m_terminal_action_pending = true;
    return true;
}

void BlackFlowLifecycleTaskPlugin::reset_in_run_variables()
{
    m_terminal_action_pending = false;
    if (m_session != nullptr && !m_session->profile().empty()) {
        m_session->reset_run();
    }
    if (m_port != nullptr) {
        m_port->reset_run();
    }
}

bool BlackFlowLifecycleTaskPlugin::_run()
{
    if (!m_terminal_action_pending) {
        return true;
    }
    m_terminal_action_pending = false;
    const std::string next_action =
        m_session != nullptr && m_session->result().has_value() ? m_session->result()->next_action : "stop_run";
    const std::string task = next_action == "restart_current_run" ? "BlackFlow@Roguelike@ExitThenAbandon"
                                                                  : "RoguelikeControlTaskPlugin-ExitThenStop";
    Task.set_task_base("BlackFlow@Roguelike@StrategyTerminalAction", task);
    Log.info("BlackFlow strategy terminal action", next_action, task);
    report_outputs();
    return true;
}
} // namespace asst::blackflow
