#pragma once

#include <memory>
#include <mutex>
#include <optional>

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

    [[nodiscard]] std::shared_ptr<const blackflow::perception::BlackFlowMapAnalyzer> acquire(std::string& error);

private:
    std::filesystem::path m_resource_root;
    std::optional<std::filesystem::path> m_pending_resource_root;
    std::weak_ptr<const blackflow::perception::BlackFlowMapAnalyzer> m_analyzer;
    bool m_model_available = false;
    bool m_pending_model_available = false;
    std::mutex m_mutex;
};

inline static auto& BlackFlowMapPerception = BlackFlowMapPerceptionResource::get_instance();
} // namespace asst
