#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/BlackFlow/BlackFlowPolicy.h"

namespace asst::blackflow
{
struct ResourceDefinition
{
    std::string id;
    std::string description;
};
} // namespace asst::blackflow

namespace asst
{
class BlackFlowStrategyConfig final : public MAA_NS::SingletonHolder<BlackFlowStrategyConfig>, public AbstractConfig
{
public:
    virtual ~BlackFlowStrategyConfig() override = default;

    [[nodiscard]] const blackflow::FactDefinition* get_fact_definition(const std::string& name) const noexcept;
    [[nodiscard]] const blackflow::PolicyModule* get_module(const std::string& id) const noexcept;
    [[nodiscard]] const blackflow::PolicyProfile* get_profile(const std::string& id) const noexcept;

    [[nodiscard]] const std::unordered_map<std::string, blackflow::FactDefinition>& facts() const noexcept
    {
        return m_facts;
    }

    [[nodiscard]] std::optional<blackflow::ResolvedPolicy>
        resolve_profile(const std::string& id, std::string* error = nullptr) const;

    [[nodiscard]] int schema_version() const noexcept { return m_schema_version; }

    // 一条里程碑声明的页面意图及其生效层段。层段同时写在 node_execution.json 的路由里，
    // 两份必须对齐，交叉校验取用此处。
    struct PageIntentWindow
    {
        std::string intent;
        int floor_begin = 1;
        int floor_end = 1;
    };

    [[nodiscard]] std::vector<PageIntentWindow> page_intent_windows() const;

    bool parse_for_test(const json::value& json, std::string* error = nullptr);

private:
    virtual bool parse(const json::value& json) override;

    int m_schema_version = 0;
    std::unordered_map<std::string, blackflow::ResourceDefinition> m_resources;
    std::unordered_map<std::string, blackflow::FactDefinition> m_facts;
    std::unordered_map<std::string, blackflow::PolicyModule> m_modules;
    std::unordered_map<std::string, blackflow::PolicyProfile> m_profiles;
};

inline static auto& BlackFlowStrategy = BlackFlowStrategyConfig::get_instance();
} // namespace asst
