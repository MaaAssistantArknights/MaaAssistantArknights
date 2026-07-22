#include "BlackFlowSafetyPlanner.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>

namespace asst::blackflow
{
namespace
{
int saturated_requirement(int successor_requirement, int cost, int gain) noexcept
{
    if (successor_requirement >= UnreachableActionPointRequirement) {
        return UnreachableActionPointRequirement;
    }
    const std::int64_t value =
        static_cast<std::int64_t>(successor_requirement) + static_cast<std::int64_t>(cost) - gain;
    return static_cast<int>(std::clamp<std::int64_t>(value, 0, UnreachableActionPointRequirement));
}

std::size_t saturated_depth(std::size_t depth) noexcept
{
    return depth == std::numeric_limits<std::size_t>::max() ? depth : depth + 1;
}
} // namespace

int SafetySolution::requirement(SafetyStateId state) const noexcept
{
    const auto iter = required_action_points.find(state);
    return iter == required_action_points.end() ? UnreachableActionPointRequirement : iter->second;
}

const std::string* SafetySolution::action(SafetyStateId state) const noexcept
{
    const auto iter = selected_actions.find(state);
    return iter == selected_actions.end() ? nullptr : &iter->second;
}

std::optional<std::size_t> SafetySolution::depth(SafetyStateId state) const noexcept
{
    const auto iter = proof_depth.find(state);
    return iter == proof_depth.end() ? std::nullopt : std::optional<std::size_t>(iter->second);
}

bool SafetySolution::certifies(SafetyStateId state, int current_action_points) const noexcept
{
    const int required = requirement(state);
    return required < UnreachableActionPointRequirement && current_action_points >= required;
}

bool SafetyPlanner::validate(const SafetyProblem& problem, std::string* error) const
{
    std::unordered_set<SafetyStateId> state_ids;
    for (const auto& state : problem.states) {
        if (!state_ids.emplace(state.id).second) {
            if (error != nullptr) {
                *error = "duplicate safety state id";
            }
            return false;
        }
    }
    if (state_ids.empty()) {
        if (error != nullptr) {
            *error = "safety problem has no states";
        }
        return false;
    }

    std::unordered_set<std::string> action_ids;
    for (const auto& action : problem.actions) {
        if (action.id.empty() || !action_ids.emplace(action.id).second) {
            if (error != nullptr) {
                *error = "safety action id is empty or duplicated";
            }
            return false;
        }
        if (!state_ids.contains(action.source)) {
            if (error != nullptr) {
                *error = "safety action source does not exist";
            }
            return false;
        }
        if (action.action_point_cost < 0 || action.minimum_action_points_to_start < 0 || action.outcomes.empty()) {
            if (error != nullptr) {
                *error = "safety action has an invalid cost, start requirement, or outcome list";
            }
            return false;
        }
        for (const auto& outcome : action.outcomes) {
            if (!state_ids.contains(outcome.successor) || outcome.action_point_gain < 0) {
                if (error != nullptr) {
                    *error = "safety action outcome is invalid";
                }
                return false;
            }
        }
    }
    return true;
}

SafetySolveResult SafetyPlanner::solve(const SafetyProblem& problem) const
{
    std::string validation_error;
    if (!validate(problem, &validation_error)) {
        return { std::nullopt, std::move(validation_error) };
    }

    std::unordered_map<SafetyStateId, std::vector<const SafetyAction*>> actions_by_source;
    SafetySolution solution;
    for (const auto& state : problem.states) {
        solution.required_action_points.emplace(state.id, state.safe_exit ? 0 : UnreachableActionPointRequirement);
        if (state.safe_exit) {
            solution.proof_depth.emplace(state.id, 0);
        }
    }
    for (const auto& action : problem.actions) {
        actions_by_source[action.source].emplace_back(&action);
    }
    for (auto& [source, actions] : actions_by_source) {
        (void)source;
        std::ranges::sort(actions, {}, &SafetyAction::id);
    }

    const std::size_t state_count = problem.states.size();
    const std::size_t action_count = std::max<std::size_t>(problem.actions.size(), 1);
    const std::size_t maximum_passes = state_count * (state_count + action_count) + 1;
    bool changed = false;
    for (std::size_t pass = 0; pass < maximum_passes; ++pass) {
        changed = false;
        for (const auto& state : problem.states) {
            if (state.safe_exit) {
                continue;
            }
            const auto source_actions = actions_by_source.find(state.id);
            if (source_actions == actions_by_source.end()) {
                continue;
            }

            int best = solution.requirement(state.id);
            const SafetyAction* witness = nullptr;
            std::size_t witness_depth = std::numeric_limits<std::size_t>::max();
            for (const SafetyAction* action : source_actions->second) {
                int requirement = std::max(action->minimum_action_points_to_start, action->action_point_cost);
                std::size_t action_depth = 0;
                bool reachable = true;
                for (const auto& outcome : action->outcomes) {
                    const int successor_requirement = solution.requirement(outcome.successor);
                    const auto successor_depth = solution.depth(outcome.successor);
                    if (successor_requirement >= UnreachableActionPointRequirement || !successor_depth.has_value()) {
                        reachable = false;
                        break;
                    }
                    requirement = std::max(
                        requirement,
                        saturated_requirement(
                            successor_requirement,
                            action->action_point_cost,
                            outcome.action_point_gain));
                    action_depth = std::max(action_depth, saturated_depth(*successor_depth));
                }
                if (reachable && requirement < best) {
                    best = requirement;
                    witness = action;
                    witness_depth = action_depth;
                }
            }

            if (witness != nullptr) {
                solution.required_action_points[state.id] = best;
                solution.selected_actions.insert_or_assign(state.id, witness->id);
                solution.proof_depth.insert_or_assign(state.id, witness_depth);
                changed = true;
            }
        }
        if (!changed) {
            return { std::move(solution), {} };
        }
    }
    return { std::nullopt, "safety requirement iteration did not converge" };
}

std::optional<SafetyBounds> SafetyPlanner::solve_bounds(
    const SafetyProblem& relaxed_problem,
    const SafetyProblem& confirmed_problem,
    SafetyStateId relaxed_initial,
    SafetyStateId confirmed_initial,
    std::string* error) const
{
    auto relaxed = solve(relaxed_problem);
    if (!relaxed) {
        if (error != nullptr) {
            *error = "relaxed safety problem: " + relaxed.error;
        }
        return std::nullopt;
    }
    auto confirmed = solve(confirmed_problem);
    if (!confirmed) {
        if (error != nullptr) {
            *error = "confirmed safety problem: " + confirmed.error;
        }
        return std::nullopt;
    }

    SafetyBounds bounds;
    bounds.optimistic_lower_bound = relaxed.solution->requirement(relaxed_initial);
    bounds.confirmed_upper_bound = confirmed.solution->requirement(confirmed_initial);
    if (bounds.optimistic_lower_bound > bounds.confirmed_upper_bound &&
        bounds.confirmed_upper_bound < UnreachableActionPointRequirement) {
        if (error != nullptr) {
            *error = "optimistic action-point lower bound exceeds confirmed upper bound";
        }
        return std::nullopt;
    }
    if (const std::string* first = confirmed.solution->action(confirmed_initial); first != nullptr) {
        bounds.confirmed_first_action = *first;
    }
    bounds.confirmed_proof_depth = confirmed.solution->depth(confirmed_initial);
    bounds.relaxed_solution = std::move(*relaxed.solution);
    bounds.confirmed_solution = std::move(*confirmed.solution);
    return bounds;
}
} // namespace asst::blackflow

