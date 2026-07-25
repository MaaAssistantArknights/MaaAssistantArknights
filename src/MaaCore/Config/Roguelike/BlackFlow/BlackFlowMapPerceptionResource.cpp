#include "BlackFlowMapPerceptionResource.h"

#include <utility>

#include "Config/OnnxSessions.h"
#include "Utils/Logger.hpp"

namespace asst
{
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

    const bool model_available = model_path.has_value();
    if (model_available) {
        OnnxSessions::get_instance().load(*model_path);
    }

    if (m_analyzer.expired()) {
        m_resource_root = path;
        m_model_available = model_available;
        m_pending_resource_root.reset();
        m_pending_model_available = false;
    }
    else if (path != m_resource_root || model_available != m_model_available) {
        const bool pending_matches = m_pending_resource_root.has_value() && *m_pending_resource_root == path &&
                                     m_pending_model_available == model_available;
        if (!pending_matches) {
            Log.warn(
                __FUNCTION__,
                "BlackFlow map perception resources changed while the analyzer is in use; "
                "the update will take effect after release",
                "old path",
                m_resource_root,
                "new path",
                path,
                "old model available",
                m_model_available,
                "new model available",
                model_available);
        }
        m_pending_resource_root = path;
        m_pending_model_available = model_available;
    }
    else {
        m_pending_resource_root.reset();
        m_pending_model_available = false;
    }
    return true;
}

std::shared_ptr<const blackflow::perception::BlackFlowMapAnalyzer>
    BlackFlowMapPerceptionResource::acquire(std::string& error)
{
    std::lock_guard lock(m_mutex);
    if (const auto current = m_analyzer.lock(); current != nullptr) {
        return current;
    }
    if (m_pending_resource_root.has_value()) {
        m_resource_root = std::move(*m_pending_resource_root);
        m_model_available = m_pending_model_available;
        m_pending_resource_root.reset();
        m_pending_model_available = false;
    }
    if (m_resource_root.empty()) {
        error = "BlackFlow map perception resource path is not registered";
        return nullptr;
    }
    if (!m_model_available) {
        error = "BlackFlow map model is not registered";
        return nullptr;
    }

    auto analyzer = std::make_shared<blackflow::perception::BlackFlowMapAnalyzer>();
    if (!analyzer->load(m_resource_root, error)) {
        return nullptr;
    }
    m_analyzer = analyzer;
    return analyzer;
}
} // namespace asst
