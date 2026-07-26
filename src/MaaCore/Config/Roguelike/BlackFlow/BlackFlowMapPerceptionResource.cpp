#include "BlackFlowMapPerceptionResource.h"

#include <utility>

#include "Config/OnnxSessions.h"
#include "Utils/Logger.hpp"

namespace asst
{
namespace
{
void update_current_path(
    std::optional<std::filesystem::path>& current,
    std::optional<std::filesystem::path>& pending,
    const std::filesystem::path& value)
{
    current = value;
    pending.reset();
}

bool update_pending_path(
    const std::optional<std::filesystem::path>& current,
    std::optional<std::filesystem::path>& pending,
    const std::filesystem::path& value)
{
    if (current.has_value() && value == *current) {
        const bool changed = pending.has_value();
        pending.reset();
        return changed;
    }
    const bool changed = !pending.has_value() || value != *pending;
    pending = value;
    return changed;
}

void apply_pending_path(std::optional<std::filesystem::path>& current, std::optional<std::filesystem::path>& pending)
{
    if (!pending.has_value()) {
        return;
    }
    current = std::move(*pending);
    pending.reset();
}
} // namespace

bool BlackFlowMapPerceptionResource::load(const std::filesystem::path& path)
{
    return load(path, std::nullopt);
}

bool BlackFlowMapPerceptionResource::load(
    const std::filesystem::path& path,
    const std::optional<std::filesystem::path>& model_path)
{
    LogTraceFunction;
    std::lock_guard lock(m_mutex);

    const auto template_manifest_path = path / "templates" / "manifest.json";
    const auto edge_config_path = path / "edge.json";
    const auto runtime_manifest_path = path / "corridor_net.runtime.json";
    const bool template_manifest_available = std::filesystem::is_regular_file(template_manifest_path);
    const bool edge_config_available = std::filesystem::is_regular_file(edge_config_path);
    const bool runtime_manifest_available = std::filesystem::is_regular_file(runtime_manifest_path);
    const bool model_available = model_path.has_value() && std::filesystem::is_regular_file(*model_path);
    if (!template_manifest_available && !edge_config_available && !runtime_manifest_available && !model_available) {
        Log.trace(
            __FUNCTION__,
            "BlackFlow map perception resource layer contains no applicable component",
            "path",
            path);
        return true;
    }

    if (model_available) {
        OnnxSessions::get_instance().load(*model_path);
    }

    if (m_analyzer.expired()) {
        if (template_manifest_available) {
            update_current_path(m_template_manifest_path, m_pending_template_manifest_path, template_manifest_path);
        }
        if (edge_config_available) {
            update_current_path(m_edge_config_path, m_pending_edge_config_path, edge_config_path);
        }
        if (runtime_manifest_available) {
            update_current_path(m_runtime_manifest_path, m_pending_runtime_manifest_path, runtime_manifest_path);
        }
        if (model_available) {
            update_current_path(m_model_path, m_pending_model_path, *model_path);
        }
        return true;
    }

    const bool template_manifest_changed =
        template_manifest_available &&
        update_pending_path(m_template_manifest_path, m_pending_template_manifest_path, template_manifest_path);
    const bool edge_config_changed =
        edge_config_available && update_pending_path(m_edge_config_path, m_pending_edge_config_path, edge_config_path);
    const bool runtime_manifest_changed =
        runtime_manifest_available &&
        update_pending_path(m_runtime_manifest_path, m_pending_runtime_manifest_path, runtime_manifest_path);
    const bool model_changed = model_available && update_pending_path(m_model_path, m_pending_model_path, *model_path);
    if (template_manifest_changed || edge_config_changed || runtime_manifest_changed || model_changed) {
        Log.warn(
            __FUNCTION__,
            "BlackFlow map perception resources changed while the analyzer is in use; "
            "the component updates will take effect together after release",
            "template manifest changed",
            template_manifest_changed,
            "edge config changed",
            edge_config_changed,
            "runtime manifest changed",
            runtime_manifest_changed,
            "model changed",
            model_changed);
    }
    return true;
}

void BlackFlowMapPerceptionResource::set_dependency_status(std::string component, bool available, std::string error)
{
    std::lock_guard lock(m_mutex);
    if (available) {
        m_dependency_errors.erase(component);
        return;
    }
    m_dependency_errors.insert_or_assign(std::move(component), std::move(error));
}

std::shared_ptr<const blackflow::perception::BlackFlowMapAnalyzer>
    BlackFlowMapPerceptionResource::acquire(std::string& error)
{
    std::lock_guard lock(m_mutex);
    if (!m_dependency_errors.empty()) {
        const auto& [component, detail] = *m_dependency_errors.begin();
        error = "BlackFlow resource component is unavailable: " + component;
        if (!detail.empty()) {
            error += ": " + detail;
        }
        return nullptr;
    }
    if (const auto current = m_analyzer.lock(); current != nullptr) {
        return current;
    }

    apply_pending_path(m_template_manifest_path, m_pending_template_manifest_path);
    apply_pending_path(m_edge_config_path, m_pending_edge_config_path);
    apply_pending_path(m_runtime_manifest_path, m_pending_runtime_manifest_path);
    apply_pending_path(m_model_path, m_pending_model_path);
    if (!m_template_manifest_path.has_value()) {
        error = "BlackFlow map template manifest is not registered";
        return nullptr;
    }
    if (!m_edge_config_path.has_value()) {
        error = "BlackFlow map edge config is not registered";
        return nullptr;
    }
    if (!m_runtime_manifest_path.has_value()) {
        error = "BlackFlow map runtime manifest is not registered";
        return nullptr;
    }
    if (!m_model_path.has_value()) {
        error = "BlackFlow map model is not registered";
        return nullptr;
    }

    auto analyzer = std::make_shared<blackflow::perception::BlackFlowMapAnalyzer>();
    if (!analyzer->load(*m_template_manifest_path, *m_edge_config_path, *m_runtime_manifest_path, error)) {
        return nullptr;
    }
    m_analyzer = analyzer;
    return analyzer;
}
} // namespace asst
