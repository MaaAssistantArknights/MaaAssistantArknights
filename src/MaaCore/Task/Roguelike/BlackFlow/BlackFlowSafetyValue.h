#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "BlackFlowSafetyPlanner.h"

namespace asst::blackflow
{
class OnDemandStateGraph;
struct PlannerState;

struct SafetyValueOutcome
{
    SafetyStateId successor = 0;
    int action_point_gain = 0;
};

struct SafetyValueAction
{
    std::string id;
    int action_point_cost = 0;
    int minimum_action_points_to_start = 1;
    std::vector<SafetyValueOutcome> outcomes;
};

struct SafetyDominanceDescriptor
{
    std::uint64_t structure_hash = 0;
    std::array<std::uint64_t, 5> structure {};
    std::array<std::uint8_t, 13> movement_charges {};
};

struct LazySafetyValueProblem
{
    using GoalPredicate = std::function<bool(SafetyStateId)>;
    using ActionExpander = std::function<bool(SafetyStateId, std::vector<SafetyValueAction>&, std::string&)>;
    using DominanceDescriptor = std::function<std::optional<SafetyDominanceDescriptor>(SafetyStateId)>;

    std::string instance_name;
    GoalPredicate is_goal;
    ActionExpander expand_actions;
    DominanceDescriptor dominance_descriptor;
};

struct SafetyValueStatistics
{
    std::size_t discovered_states = 0;
    std::size_t expanded_states = 0;
    std::size_t fixed_point_updates = 0;
};

class SafetyValueSolver
{
public:
    explicit SafetyValueSolver(LazySafetyValueProblem problem);

    [[nodiscard]] int N(SafetyStateId state, std::string* error = nullptr);
    [[nodiscard]] int N_bounded(SafetyStateId state, int maximum_action_points, std::string* error = nullptr);
    [[nodiscard]] std::optional<std::string>
        bounded_witness(SafetyStateId state, int maximum_action_points, std::string* error = nullptr);
    [[nodiscard]] std::optional<std::size_t>
        bounded_proof_depth(SafetyStateId state, int maximum_action_points, std::string* error = nullptr);
    [[nodiscard]] int cached_requirement(SafetyStateId state) const noexcept;
    [[nodiscard]] int Q(SafetyStateId state, std::string_view action_id, std::string* error = nullptr);
    [[nodiscard]] std::optional<std::string> witness(SafetyStateId state, std::string* error = nullptr);
    [[nodiscard]] bool certifies(SafetyStateId state, int action_points, std::string* error = nullptr);

    [[nodiscard]] const SafetyValueStatistics& statistics() const noexcept { return m_statistics; }

    [[nodiscard]] const std::string& instance_name() const noexcept { return m_problem.instance_name; }

    [[nodiscard]] const std::string& last_error() const noexcept { return m_error; }

    void reset();

private:
    struct CachedState
    {
        bool goal = false;
        bool expanded = false;
        int value = UnreachableActionPointRequirement;
        std::vector<SafetyValueAction> actions;
        std::vector<std::uint8_t> action_enabled;
        std::vector<SafetyStateId> predecessors;
        std::optional<std::string> selected_action;
    };

    [[nodiscard]] bool ensure_solved(SafetyStateId initial, std::string* error);
    [[nodiscard]] bool discover_closure(SafetyStateId initial);
    [[nodiscard]] bool ensure_bounded_solved(SafetyStateId initial, int maximum_action_points, std::string* error);
    [[nodiscard]] bool discover_bounded_closure(SafetyStateId initial, int maximum_action_points);
    [[nodiscard]] bool solve_fixed_point();
    [[nodiscard]] int action_value(const SafetyValueAction& action) const noexcept;
    [[nodiscard]] int solve_bounded_recursive(SafetyStateId state, int maximum_action_points);
    [[nodiscard]] bool validate_action(SafetyStateId source, const SafetyValueAction& action, std::string& error) const;
    void report_error(std::string message, std::string* error);

    struct BoundedResult
    {
        bool visiting = false;
        int value = UnreachableActionPointRequirement;
        std::size_t proof_depth = 0;
        std::optional<std::string> selected_action;
    };

    struct DominanceEntry
    {
        SafetyDominanceDescriptor descriptor;
        int action_points = 0;
        bool safe = false;
        std::size_t proof_depth = 0;
        std::optional<std::string> selected_action;
    };

    [[nodiscard]] std::optional<BoundedResult> dominated_result(SafetyStateId state, int action_points) const;
    void remember_dominance(SafetyStateId state, int action_points, const BoundedResult& result);

    LazySafetyValueProblem m_problem;
    std::unordered_map<SafetyStateId, CachedState> m_states;
    std::unordered_map<std::uint64_t, BoundedResult> m_bounded_results;
    std::unordered_map<std::uint64_t, std::vector<DominanceEntry>> m_dominance_frontiers;
    SafetyValueStatistics m_statistics;
    std::string m_error;
    bool m_requires_full_fixed_point = false;
};

using OnDemandSafetyValueGoalPredicate = std::function<bool(SafetyStateId, const PlannerState&)>;

[[nodiscard]] LazySafetyValueProblem make_on_demand_safety_value_problem(
    OnDemandStateGraph& graph,
    std::string instance_name,
    OnDemandSafetyValueGoalPredicate goal_predicate = {});
} // namespace asst::blackflow
