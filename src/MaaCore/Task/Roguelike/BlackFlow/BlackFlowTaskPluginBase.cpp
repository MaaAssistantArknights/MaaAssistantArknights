#include "BlackFlowTaskPluginBase.h"

#include "Utils/Logger.hpp"

namespace asst::blackflow
{
BlackFlowTaskPluginBase::BlackFlowTaskPluginBase(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    const std::shared_ptr<RoguelikeConfig>& config,
    const std::shared_ptr<RoguelikeControlTaskPlugin>& control,
    std::shared_ptr<BlackFlowSession> session,
    std::shared_ptr<IBlackFlowTaskPort> port) :
    AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config, control),
    m_session(std::move(session)),
    m_port(std::move(port))
{
    set_block(true);
}

bool BlackFlowTaskPluginBase::load_params([[maybe_unused]] const json::value& params)
{
    return m_config->get_theme() == RoguelikeTheme::BlackFlow && m_session != nullptr;
}

void BlackFlowTaskPluginBase::report_outputs()
{
    if (m_session == nullptr) {
        return;
    }
    for (auto& event : m_session->take_telemetry_events()) {
        auto info = basic_info_with_what(event.what);
        info["details"] = std::move(event.details);
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
    if (m_port != nullptr) {
        for (const auto& request : m_session->take_diagnostic_requests()) {
            std::string error;
            if (!m_port->persist_diagnostics(request, &error)) {
                Log.warn("BlackFlow diagnostic artifact persistence failed:", error);
            }
        }
    }
    if (m_session->claim_result_report()) {
        auto info = basic_info_with_what("BlackFlowStrategyResult");
        info["details"] = m_session->result()->to_json();
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
}
} // namespace asst::blackflow
