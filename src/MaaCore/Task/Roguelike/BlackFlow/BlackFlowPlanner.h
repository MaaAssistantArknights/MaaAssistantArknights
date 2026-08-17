#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "BlackFlowPolicy.h"
#include "BlackFlowStateSpace.h"

namespace asst::blackflow
{
struct RouteSearchOptions
{
    int time_budget_ms = 1000;
    std::size_t total_expansions = 4096;
    std::size_t expansions_per_root = 128;
    int greedy_preview_depth = 2;
    bool safety_resource_dominance = true;
};

struct BlackFlowPlanRequest
{
    const MapSnapshot* map = nullptr;
    const RunState* run = nullptr;
    const ResolvedPolicy* policy = nullptr;
    const FactStore* facts = nullptr;
    const MissionState* mission = nullptr;
    // 策略声明「达成即收工」的目标节点，与物理出口一起构成端点集合。
    std::unordered_set<NodeId> strategy_terminal_nodes;
    // 待锁定的强制目标，按优先级从高到低排好。plan() 会沿这个序做可行性阶梯：
    // 证得出安全解就锁定，证不出就从末尾降级一个再试，因此不会出现整层无解。
    std::vector<std::string> binding_milestone_candidates;
    // 候选里最前面这几条是无条件必达的，阶梯不会降级它们；不可达时本层照声明判成无解。
    std::size_t undemotable_binding_count = 0;
    // 走出本层与耗尽行动力结局相同的策略打开它。打开后锁定目标为空的那几轮不再为出口
    // 预留行动力，路线可以一直走到付不起下一步为止。
    bool no_AP_is_terminal = false;
    const std::unordered_set<std::string>* forbidden_actions = nullptr;
    std::optional<NodeId> probe_target;
    std::size_t maximum_states = 2'000'000;
    RouteSearchOptions route_search;
};

struct PreviewSafetyVerification
{
    bool safe = false;
    int action_points_after = 0;
    int required_action_points_after = UnreachableActionPointRequirement;
    std::optional<std::size_t> proof_depth;
    std::string error;
};

// 安全值的评估结论：当前状态距安全出口还需多少行动力，以及首个动作与证明深度。
struct SafetyAssessment
{
    int required_action_points = UnreachableActionPointRequirement;
    std::optional<std::string> first_action;
    std::optional<std::size_t> proof_depth;
};

struct BlackFlowPlan
{
    SafetyAssessment safety;
    SafetyAssessment relaxed_safety;
    PolicyDecision decision;
    // 本轮实际锁定的强制目标，以及因为证不出可行而降级成倾向的那些。
    std::unordered_set<std::string> binding_milestone_ids;
    std::vector<std::string> demoted_milestone_ids;
    std::uint64_t map_revision = 0;
    std::uint64_t cost_revision = 0;
    std::size_t confirmed_state_count = 0;
    std::size_t relaxed_state_count = 0;
    std::size_t route_search_expansions = 0;
    bool route_search_time_exhausted = false;
    bool route_search_expansions_exhausted = false;
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return error.empty() && decision.selected.has_value(); }
};

class BlackFlowPlanner
{
public:
    [[nodiscard]] BlackFlowPlan plan(const BlackFlowPlanRequest& request) const;
    [[nodiscard]] PreviewSafetyVerification verify_previewed_move(
        const BlackFlowPlanRequest& request,
        const MoveCandidate& move,
        int exact_action_point_cost) const;
};
} // namespace asst::blackflow
