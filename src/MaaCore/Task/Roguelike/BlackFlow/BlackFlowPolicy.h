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

// 目标对规划的约束强度。它与 MilestoneTerminality 相互独立：前者决定“必不必须做”，
// 后者决定“做完了算不算收工”。同一个选择器换一组取值就是另一条策略，不需要另写代码。
enum class MilestoneEnforcement
{
    // 只进路线效用评分，达不成也不影响求解。
    Soft,
    // 可行则必达：先证明“加上这条约束仍有安全解”，证明通过才升为终局合取项；
    // 证不通过就降级成 Soft，因此不会像无条件强制那样把本层判成无解。
    FeasibleHard,
    // 无条件必达。留给物理上不可能缺席的目标，代价是不可达时本层直接无解。
    Hard,
};

// 达成之后本局是否就此结束。None 表示还得走到物理出口，IsTerminal 表示目标节点
// 本身就是合法终点（例如襁褓动物进了秘境行商就收工）。
enum class MilestoneTerminality
{
    None,
    IsTerminal,
};

// 目标被判定错过或不可能时的收尾方式。Terminate 直接结算本局，省得每条策略
// 都去重写一遍同样的终止规则。
enum class MilestoneMissAction
{
    Ignore,
    Terminate,
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
    std::vector<std::string> marker_types;
    std::optional<bool> badged;
    std::optional<NodeIdentityState> identity_state;
    std::optional<bool> identity_revealed;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool matches(const Node& node) const noexcept;
};

struct Milestone
{
    std::string id;
    std::string description;
    MilestoneEnforcement enforcement = MilestoneEnforcement::Soft;
    MilestoneTerminality terminality = MilestoneTerminality::None;
    MilestoneMissAction on_miss = MilestoneMissAction::Ignore;
    std::string miss_outcome;
    std::string miss_reason;
    bool miss_succeeded = false;
    // kind 只在 enforcement 为 Soft、或强制目标被降级之后决定软层分档。
    MilestoneKind kind = MilestoneKind::None;
    MilestoneCompletion completion = MilestoneCompletion::VisitCount;
    int floor_begin = 0;
    int floor_end = 0;
    int rank = 0;
    int required_count = 1;
    int weight = 1;
    int minimum_unknown_nodes_revealed = 0;
    Condition active_if;
    Condition complete_if { ConditionKind::Constant, false };
    NodeSelector selector;
    std::string page_intent;
    std::vector<std::string> reserve_ids;
    std::vector<std::string> prerequisites;

    // 参与绑定候选的目标。是否真的绑定还要看可行性阶梯的结论。
    [[nodiscard]] bool binding_candidate() const noexcept { return enforcement != MilestoneEnforcement::Soft; }
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
    // 走出本层与耗尽行动力结局相同的层。这些层没有锁定目标的那几轮不再为出口预留行动力，
    // 路线一直走到付不起下一步为止。
    //
    // 按层声明而非整体开关：策略可能只在部分层这样收工，例如只打第二、四层的追猎，
    // 区间表达不了这种层集合。必须抵达终点的层一律不要列进来。
    std::unordered_set<int> no_AP_is_terminal_floors;
};

struct ResolvedPolicy
{
    std::string profile_id;
    std::string description;
    std::vector<std::string> modules;
    std::vector<RoutePreference> route_preferences;
    std::vector<PolicyRule> rules;
    std::vector<ResourceReserve> reserves;
    std::vector<Milestone> milestones;
    std::vector<GrantedScrap> granted_scraps;
    std::vector<StrategyTerminalRule> terminal_rules;
    std::string failure_action = "stop_run";
    // 走出本层与耗尽行动力结局相同的层。这些层没有锁定目标的那几轮不再为出口预留行动力，
    // 路线一直走到付不起下一步为止。
    //
    // 按层声明而非整体开关：策略可能只在部分层这样收工，例如只打第二、四层的追猎，
    // 区间表达不了这种层集合。必须抵达终点的层一律不要列进来。
    std::unordered_set<int> no_AP_is_terminal_floors;
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
    // 只计跨层保留的加工品。过期的下楼即作废，省下来没有意义。
    int persistent_processing_move_count = 0;
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
    // 因为无法满足而被放弃的资源预留。放弃它们是为了不让本轮变成无路可走。
    std::vector<std::string> released_reserve_ids;
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
        const std::unordered_set<std::string>& binding_milestone_ids,
        const std::vector<PolicyCandidate>& candidates) const;
};

[[nodiscard]] bool rule_is_active(const PolicyRule& rule, const FactStore& facts);
[[nodiscard]] bool
    rule_matches_candidate(const PolicyRule& rule, const FactStore& facts, const FactStore& candidate_facts);
[[nodiscard]] bool milestone_matches_node(const Milestone& milestone, const Node& node) noexcept;
[[nodiscard]] bool milestone_is_active(
    const Milestone& milestone,
    int floor,
    const FactStore& facts,
    const MissionState& mission_state);
[[nodiscard]] bool milestone_is_complete(const Milestone& milestone, const FactStore& facts);

[[nodiscard]] std::string_view to_string(RuleKind kind) noexcept;
[[nodiscard]] std::string_view to_string(PolicyTier tier) noexcept;
[[nodiscard]] std::string_view to_string(MilestoneStatus status) noexcept;
[[nodiscard]] std::string_view to_string(DecisionReasonCategory category) noexcept;
} // namespace asst::blackflow

