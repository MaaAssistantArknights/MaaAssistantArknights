#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "Config/AbstractResource.h"
#include "Vision/Roguelike/BlackFlow/BlackFlowMapAnalyzer.h"

namespace asst
{
class BlackFlowMapPerceptionResource final :
    public MAA_NS::SingletonHolder<BlackFlowMapPerceptionResource>,
    public AbstractResource
{
public:
    ~BlackFlowMapPerceptionResource() override = default;
    bool load(const std::filesystem::path& path) override;
    bool load(const std::filesystem::path& path, const std::optional<std::filesystem::path>& model_path);

    void set_dependency_status(std::string component, bool available, std::string error = {});

    [[nodiscard]] std::shared_ptr<const blackflow::perception::BlackFlowMapAnalyzer> acquire(std::string& error);

private:
    std::optional<std::filesystem::path> m_template_manifest_path;
    std::optional<std::filesystem::path> m_edge_config_path;
    std::optional<std::filesystem::path> m_runtime_manifest_path;
    std::optional<std::filesystem::path> m_model_path;
    std::optional<std::filesystem::path> m_pending_template_manifest_path;
    std::optional<std::filesystem::path> m_pending_edge_config_path;
    std::optional<std::filesystem::path> m_pending_runtime_manifest_path;
    std::optional<std::filesystem::path> m_pending_model_path;
    std::weak_ptr<const blackflow::perception::BlackFlowMapAnalyzer> m_analyzer;
    std::map<std::string, std::string> m_dependency_errors;
    std::mutex m_mutex;
};

inline static auto& BlackFlowMapPerception = BlackFlowMapPerceptionResource::get_instance();
} // namespace asst
