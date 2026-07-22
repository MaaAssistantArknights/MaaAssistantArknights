#pragma once

#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
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

struct PageRoute
{
    std::string id;
    std::string alias;
    std::string task;
    int rank = 0;
    Condition when;
};

enum class TaskEventEffectKind
{
    Set,
    Add,
    CaptureInteger,
};

struct TaskEventEffect
{
    TaskEventEffectKind kind = TaskEventEffectKind::Set;
    std::string fact;
    std::optional<FactValue> value;
    std::string source;
    int minimum = std::numeric_limits<int>::min();
    int maximum = std::numeric_limits<int>::max();
};

struct TaskEvent
{
    std::string task;
    std::vector<TaskEventEffect> effects;
    std::string outcome_code;
    std::string termination_reason;
    bool terminate = false;
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
    [[nodiscard]] const std::vector<blackflow::PageRoute>* get_page_routes(const std::string& profile) const noexcept;
    [[nodiscard]] const blackflow::TaskEvent* get_task_event(const std::string& task) const noexcept;

    [[nodiscard]] const std::unordered_map<std::string, blackflow::FactDefinition>& facts() const noexcept
    {
        return m_facts;
    }

    [[nodiscard]] std::optional<blackflow::ResolvedPolicy>
        resolve_profile(const std::string& id, std::string* error = nullptr) const;

    [[nodiscard]] int schema_version() const noexcept { return m_schema_version; }

    bool parse_for_test(const json::value& json, std::string* error = nullptr);

private:
    virtual bool parse(const json::value& json) override;

    int m_schema_version = 0;
    std::unordered_map<std::string, blackflow::ResourceDefinition> m_resources;
    std::unordered_map<std::string, blackflow::FactDefinition> m_facts;
    std::unordered_map<std::string, blackflow::PolicyModule> m_modules;
    std::unordered_map<std::string, blackflow::PolicyProfile> m_profiles;
    std::unordered_map<std::string, std::vector<blackflow::PageRoute>> m_page_routes;
    std::unordered_map<std::string, blackflow::TaskEvent> m_task_events;
};

inline static auto& BlackFlowStrategy = BlackFlowStrategyConfig::get_instance();
} // namespace asst
