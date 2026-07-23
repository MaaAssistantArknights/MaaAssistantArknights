#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

    [[nodiscard]] std::unordered_set<std::string> page_intents() const;

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
