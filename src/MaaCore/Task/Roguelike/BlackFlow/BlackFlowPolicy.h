#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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
    StrategyConstraint,
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
};

enum class MilestoneKind
{
    None,
    Preferred,
    Opportunistic,
};

enum class MilestoneCompletion
{
    VisitCount,
    Condition,
};

struct PolicyRule
{
    std::string id;
    std::string description;
    RuleKind kind = RuleKind::Prefer;
    PolicyTier tier = PolicyTier::Development;
    int rank = 0;
    Condition when;
    Condition candidate_if { ConditionKind::Constant, false };
    std::string page_intent;
};

struct ResourceReserve
{
    std::string id;
    std::string description;
    std::string resource;
    int minimum = 0;
    Condition active_if;
    Condition release_if { ConditionKind::Constant, false };
};

struct NodeSelector
{
    std::vector<NodeType> node_types;
    std::vector<std::string> node_names;
    std::optional<bool> badged;
    std::optional<NodeIdentityState> identity_state;
    std::optional<bool> identity_revealed;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool matches(const Node& node) const noexcept;
};

struct HiddenNodeReveal
{
    NodeType hidden_node_type = NodeType::Unknown;
    std::vector<NodeType> revealed_node_types;
};

struct Milestone
{
    std::string id;
    std::string description;
    MilestoneKind kind = MilestoneKind::None;
    MilestoneCompletion completion = MilestoneCompletion::VisitCount;
    int floor_begin = 0;
    int floor_end = 0;
    int rank = 0;
    int required_count = 1;
    int weight = 1;
    bool end = false;
    int minimum_unknown_nodes_revealed = 0;
    Condition active_if;
    Condition complete_if { ConditionKind::Constant, false };
    NodeSelector selector;
    std::string page_intent;
    std::vector<std::string> reserve_ids;
    std::vector<std::string> prerequisites;
};

struct MissionState
{
    MissionViability viability = MissionViability::Possible;
    std::unordered_map<std::string, MilestoneStatus> milestones;
    std::unordered_map<std::string, int> milestone_progress;
    std::unordered_map<std::string, std::unordered_set<NodeId>> milestone_nodes;

    [[nodiscard]] MilestoneStatus status(std::string_view id) const noexcept;
    [[nodiscard]] int progress(std::string_view id) const noexcept;
    void set_status(std::string id, MilestoneStatus status);
    void set_progress(std::string id, int value);
    bool record_node(const std::vector<Milestone>& definitions, const FactStore& facts, const Node& node);
    void refresh(const std::vector<Milestone>& definitions, int floor, const FactStore& facts);
};

enum class RoutePreference
{
    MinimizeIntermediateInteractions,
};

struct GrantedScrap
{
    std::string id;
    std::string description;
    MovementKind movement = MovementKind::Walk;
    Condition when;
};

struct PolicyModule
{
    std::string id;
    std::string description;
    std::vector<RoutePreference> route_preferences;
    std::vector<PolicyRule> rules;
    std::vector<ResourceReserve> reserves;
    std::vector<Milestone> milestones;
    std::vector<GrantedScrap> granted_scraps;
};

struct StrategyTerminalRule
{
    std::string id;
    Condition when;
    std::string outcome;
    std::string reason;
    bool succeeded = false;
    std::string next_action;
};

struct PolicyProfile
{
    std::string id;
    std::string description;
    std::vector<std::string> modules;
    std::vector<StrategyTerminalRule> terminal_rules;
    std::string failure_action = "stop_run";
};

struct ResolvedPolicy
{
    std::string profile_id;
    std::string description;
    std::vector<std::string> modules;
    std::vector<RoutePreference> route_preferences;
    std::vector<HiddenNodeReveal> hidden_node_reveals;
    std::vector<PolicyRule> rules;
    std::vector<ResourceReserve> reserves;
    std::vector<Milestone> milestones;
    std::vector<GrantedScrap> granted_scraps;
    std::vector<StrategyTerminalRule> terminal_rules;
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

struct PlannedRouteStep
{
    MoveCandidate move;
    int action_points_before = 0;
    int action_point_cost = 0;
    int action_point_gain = 0;
    int action_points_after = 0;
};

struct PolicyCandidate
{
    MoveCandidate move;
    FactStore facts;
    bool legal = true;
    bool safe = false;
    int development_score = 0;
    int risk_score = 0;
    int battle_count = 0;
    int intermediate_interaction_count = 0;
    int processing_move_count = 0;
    int estimated_duration = 0;
    std::unordered_map<std::string, int> milestone_progress;
    std::vector<std::string> immediate_milestone_ids;
    std::vector<NodeId> planned_route;
    std::vector<PlannedRouteStep> planned_route_steps;
};

enum class DecisionReasonCategory
{
    StrategyEnd,
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
    std::vector<NodeId> planned_route;
    std::vector<PlannedRouteStep> planned_route_steps;
    std::unordered_map<std::string, int> planned_milestone_progress;
    std::vector<MoveCandidate> runners_up;
    std::vector<std::string> rejected;
    std::unordered_map<std::string, std::size_t> rejection_counts;
    std::size_t total_candidates = 0;
    std::size_t eligible_candidates = 0;
    DecisionReasonCategory reason_category = DecisionReasonCategory::TieBreak;
    std::string decisive_rule_id;
    std::string decisive_milestone_id;
    std::vector<std::string> decisive_milestone_ids;
    std::string selected_page_intent;
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
[[nodiscard]] bool milestone_matches_node(const Milestone& milestone, const Node& node) noexcept;
[[nodiscard]] bool hidden_node_may_reveal_milestone(
    const ResolvedPolicy& policy,
    const Milestone& milestone,
    const Node& node) noexcept;
[[nodiscard]] bool milestone_is_active(
    const Milestone& milestone,
    int floor,
    const FactStore& facts,
    const MissionState& mission_state);
[[nodiscard]] bool milestone_is_complete(const Milestone& milestone, const FactStore& facts);

[[nodiscard]] std::string_view to_string(RuleKind kind) noexcept;
[[nodiscard]] std::string_view to_string(PolicyTier tier) noexcept;
[[nodiscard]] std::string_view to_string(DecisionReasonCategory category) noexcept;
} // namespace asst::blackflow

