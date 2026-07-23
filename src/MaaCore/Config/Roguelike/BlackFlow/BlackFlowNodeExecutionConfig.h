#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/BlackFlow/BlackFlowNodeExecutionTypes.h"

namespace asst
{
class BlackFlowNodeExecutionConfig final :
    public MAA_NS::SingletonHolder<BlackFlowNodeExecutionConfig>,
    public AbstractConfig
{
public:
    virtual ~BlackFlowNodeExecutionConfig() override = default;

    [[nodiscard]] const blackflow::NodeExecutionRoute*
        resolve_route(const blackflow::NodeExecutionContext& context) const noexcept;
    [[nodiscard]] const blackflow::NodeTaskResult* get_task_result(const std::string& task) const noexcept;

    [[nodiscard]] const std::vector<blackflow::NodeExecutionRoute>& routes() const noexcept { return m_routes; }

    [[nodiscard]] int schema_version() const noexcept { return m_schema_version; }

    bool parse_for_test(const json::value& json, std::string* error = nullptr);

private:
    virtual bool parse(const json::value& json) override;

    int m_schema_version = 0;
    std::vector<blackflow::NodeExecutionRoute> m_routes;
    std::unordered_map<std::string, blackflow::NodeTaskResult> m_task_results;
};

inline static auto& BlackFlowNodeExecution = BlackFlowNodeExecutionConfig::get_instance();
} // namespace asst
