#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "BlackFlowModel.h"

namespace asst::blackflow
{
using FactValue = std::variant<bool, std::int64_t, std::string, std::vector<std::string>>;

enum class FactType
{
    Boolean,
    Integer,
    String,
    StringList,
};

enum class FactScope
{
    Run,
    Floor,
    Page,
    Candidate,
};

struct FactDefinition
{
    std::string name;
    FactType type = FactType::Boolean;
    FactScope scope = FactScope::Run;
    std::string description;
};

class FactStore
{
public:
    void set(std::string key, FactValue value);
    bool erase(std::string_view key);
    void clear() noexcept;
    [[nodiscard]] const FactValue* find(std::string_view key) const noexcept;
    [[nodiscard]] FactStore overlay(const FactStore& higher_priority) const;

    [[nodiscard]] const auto& values() const noexcept { return m_values; }

private:
    std::unordered_map<std::string, FactValue> m_values;
};

class FactContext
{
public:
    bool define(FactDefinition definition, std::string* error = nullptr);
    bool set(FactScope scope, std::string key, FactValue value, std::string* error = nullptr);
    [[nodiscard]] const FactValue* find(std::string_view key) const noexcept;
    [[nodiscard]] const FactValue* find(FactScope scope, std::string_view key) const noexcept;
    [[nodiscard]] FactStore merged() const;
    void clear_scope(FactScope scope) noexcept;
    void begin_run() noexcept;
    void begin_floor() noexcept;
    void begin_page() noexcept;
    void begin_candidate() noexcept;

private:
    [[nodiscard]] FactStore& store(FactScope scope) noexcept;
    [[nodiscard]] const FactStore& store(FactScope scope) const noexcept;

    std::unordered_map<std::string, FactDefinition> m_definitions;
    FactStore m_run;
    FactStore m_floor;
    FactStore m_page;
    FactStore m_candidate;
};

enum class ConditionKind
{
    Constant,
    All,
    Any,
    Not,
    Predicate,
};

enum class CompareOperator
{
    Exists,
    NotExists,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Contains,
};

struct Condition
{
    ConditionKind kind = ConditionKind::Constant;
    bool constant = true;
    std::vector<Condition> children;
    std::string fact;
    CompareOperator compare = CompareOperator::Exists;
    std::optional<FactValue> value;

    [[nodiscard]] bool evaluate(const FactStore& facts) const;
};

enum class RuleKind
{
    Forbid,
    Require,
    Prefer,
    TieBreak,
};

enum class PolicyTier
{
    Legality,
    Safety,
    MandatoryMilestone,
    ResourceReserve,
    PreferredMilestone,
    Development,
    Risk,
    TieBreak,
};

enum class MilestoneStatus
{
    Inactive,
    Available,
    Satisfied,
    Missed,
    Impossible,
};

enum class MissionViability
{
    Confirmed,
    Possible,
    Impossible,
};

enum class MilestoneKind
{
    Mandatory,
    Preferred,
    Opportunistic,
};

struct PolicyRule
{
    std::string id;
    std::string description;
    RuleKind kind = RuleKind::Prefer;
    PolicyTier tier = PolicyTier::Development;
    int rank = 0;
    Condition when;
    Condition target;
};

struct ResourceReserve
{
    std::string id;
    std::string description;
    std::string resource;
    int minimum = 0;
    Condition active_if;
    Condition release_if;
};

struct Milestone
{
    std::string id;
    std::string description;
    MilestoneKind kind = MilestoneKind::Preferred;
    int floor_begin = 1;
    int floor_end = 99;
    int rank = 0;
    Condition active_if;
    Condition complete_if;
    Condition target;
    std::vector<std::string> reserve_ids;
    std::vector<std::string> prerequisites;
    std::vector<std::string> successors;
};

struct MissionState
{
    MissionViability viability = MissionViability::Possible;
    std::unordered_map<std::string, MilestoneStatus> milestones;

    [[nodiscard]] MilestoneStatus status(std::string_view id) const noexcept;
    void set_status(std::string id, MilestoneStatus status);
    void refresh(const std::vector<Milestone>& definitions, int floor, const FactStore& facts);
};

struct PolicyModule
{
    std::string id;
    std::string description;
    std::vector<PolicyRule> rules;
    std::vector<ResourceReserve> reserves;
    std::vector<Milestone> milestones;
};

struct PolicyProfile
{
    std::string id;
    std::string description;
    std::vector<std::string> modules;
    std::string failure_action = "stop_run";
};

struct ResolvedPolicy
{
    std::string profile_id;
    std::string description;
    std::vector<std::string> modules;
    std::vector<PolicyRule> rules;
    std::vector<ResourceReserve> reserves;
    std::vector<Milestone> milestones;
    std::string failure_action = "stop_run";
};

class ResourceRegistry
{
public:
    using Reader = std::function<std::int64_t(const RunState&)>;

    ResourceRegistry();
    bool register_resource(std::string id, Reader reader);
    [[nodiscard]] bool contains(std::string_view id) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> read(std::string_view id, const RunState& state) const;
    [[nodiscard]] std::optional<std::int64_t>
        read_after(std::string_view id, const RunState& state, const MoveCandidate& candidate) const;

private:
    std::unordered_map<std::string, Reader> m_readers;
};

struct PolicyCandidate
{
    MoveCandidate move;
    FactStore facts;
    bool legal = true;
    bool confirmed_safe = false;
    int development_score = 0;
    int risk_score = 0;
    int battle_count = 0;
    int estimated_duration = 0;
};

enum class DecisionReasonCategory
{
    MandatoryGoal,
    ResourceReserve,
    PreferredGoal,
    Development,
    RiskAvoidance,
    SafetyFallback,
    TieBreak,
};

struct PolicyDecision
{
    std::optional<MoveCandidate> selected;
    std::vector<MoveCandidate> runners_up;
    std::vector<std::string> rejected;
    std::unordered_map<std::string, std::size_t> rejection_counts;
    std::size_t total_candidates = 0;
    std::size_t eligible_candidates = 0;
    DecisionReasonCategory reason_category = DecisionReasonCategory::TieBreak;
    std::string decisive_rule_id;
    std::string decisive_milestone_id;
    std::string reason;
};

class PolicyExecutor
{
public:
    [[nodiscard]] PolicyDecision choose(
        const ResolvedPolicy& policy,
        const FactStore& facts,
        const MissionState& mission,
        const RunState& run,
        const ResourceRegistry& resources,
        const std::vector<PolicyCandidate>& candidates) const;
};

[[nodiscard]] bool rule_is_active(const PolicyRule& rule, const FactStore& facts);
[[nodiscard]] bool
    rule_matches_candidate(const PolicyRule& rule, const FactStore& facts, const FactStore& candidate_facts);
[[nodiscard]] bool milestone_is_active(
    const Milestone& milestone,
    int floor,
    const FactStore& facts,
    const MissionState& mission_state);
[[nodiscard]] bool milestone_is_complete(const Milestone& milestone, const FactStore& facts);
[[nodiscard]] bool
    milestone_matches_candidate(const Milestone& milestone, const FactStore& facts, const FactStore& candidate_facts);

[[nodiscard]] std::string_view to_string(RuleKind kind) noexcept;
[[nodiscard]] std::string_view to_string(PolicyTier tier) noexcept;
[[nodiscard]] std::string_view to_string(DecisionReasonCategory category) noexcept;
} // namespace asst::blackflow

