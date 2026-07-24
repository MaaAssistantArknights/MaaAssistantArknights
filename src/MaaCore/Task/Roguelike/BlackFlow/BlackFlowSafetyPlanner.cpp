#include "BlackFlowSafetyPlanner.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <queue>
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

    const std::size_t state_count = problem.states.size();
    std::unordered_map<SafetyStateId, std::size_t> state_indices;
    state_indices.reserve(state_count);

    std::vector<std::vector<const SafetyAction*>> actions_by_source(state_count);
    std::vector<std::vector<std::size_t>> predecessors(state_count);
    SafetySolution solution;
    solution.required_action_points.reserve(state_count);
    solution.proof_depth.reserve(state_count);
    solution.selected_actions.reserve(state_count);

    for (std::size_t index = 0; index < state_count; ++index) {
        const auto& state = problem.states[index];
        state_indices.emplace(state.id, index);
        solution.required_action_points.emplace(state.id, state.safe_exit ? 0 : UnreachableActionPointRequirement);
        if (state.safe_exit) {
            solution.proof_depth.emplace(state.id, 0);
        }
    }

    for (const auto& action : problem.actions) {
        const std::size_t source_index = state_indices.at(action.source);
        actions_by_source[source_index].emplace_back(&action);
        for (const auto& outcome : action.outcomes) {
            predecessors[state_indices.at(outcome.successor)].emplace_back(source_index);
        }
    }
    for (auto& actions : actions_by_source) {
        std::ranges::sort(actions, {}, &SafetyAction::id);
    }
    for (auto& predecessor_list : predecessors) {
        std::ranges::sort(predecessor_list);
        predecessor_list.erase(std::unique(predecessor_list.begin(), predecessor_list.end()), predecessor_list.end());
    }

    using StateQueue = std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>>;
    StateQueue current_queue;
    StateQueue next_queue;
    std::vector<bool> current_queued(state_count, false);
    std::vector<bool> next_queued(state_count, false);

    for (std::size_t index = 0; index < state_count; ++index) {
        if (!problem.states[index].safe_exit && !actions_by_source[index].empty()) {
            current_queue.emplace(index);
            current_queued[index] = true;
        }
    }

    const std::size_t action_count = std::max<std::size_t>(problem.actions.size(), 1);
    const std::size_t maximum_passes = state_count * (state_count + action_count) + 1;
    for (std::size_t pass = 0; pass < maximum_passes; ++pass) {
        bool changed = false;
        while (!current_queue.empty()) {
            const std::size_t state_index = current_queue.top();
            current_queue.pop();
            current_queued[state_index] = false;

            const auto& state = problem.states[state_index];
            int best = solution.requirement(state.id);
            const SafetyAction* witness = nullptr;
            std::size_t witness_depth = std::numeric_limits<std::size_t>::max();
            for (const SafetyAction* action : actions_by_source[state_index]) {
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

            if (witness == nullptr) {
                continue;
            }

            solution.required_action_points[state.id] = best;
            solution.selected_actions.insert_or_assign(state.id, witness->id);
            solution.proof_depth.insert_or_assign(state.id, witness_depth);
            changed = true;

            for (const std::size_t predecessor_index : predecessors[state_index]) {
                if (problem.states[predecessor_index].safe_exit) {
                    continue;
                }
                if (predecessor_index > state_index) {
                    if (!current_queued[predecessor_index]) {
                        current_queue.emplace(predecessor_index);
                        current_queued[predecessor_index] = true;
                    }
                }
                else if (!next_queued[predecessor_index]) {
                    next_queue.emplace(predecessor_index);
                    next_queued[predecessor_index] = true;
                }
            }
        }

        if (!changed) {
            return { std::move(solution), {} };
        }
        if (pass + 1 == maximum_passes) {
            break;
        }
        if (next_queue.empty()) {
            return { std::move(solution), {} };
        }

        current_queue = std::move(next_queue);
        current_queued = std::move(next_queued);
        next_queue = StateQueue {};
        next_queued.assign(state_count, false);
    }
    return { std::nullopt, "safety requirement iteration did not converge" };
}

std::optional<SafetyAssessment>
    SafetyPlanner::assess(const SafetyProblem& problem, SafetyStateId initial, std::string* error) const
{
    auto solved = solve(problem);
    if (!solved) {
        if (error != nullptr) {
            *error = solved.error;
        }
        return std::nullopt;
    }

    SafetyAssessment assessment;
    assessment.required_action_points = solved.solution->requirement(initial);
    if (const std::string* first = solved.solution->action(initial); first != nullptr) {
        assessment.first_action = *first;
    }
    assessment.proof_depth = solved.solution->depth(initial);
    assessment.solution = std::move(*solved.solution);
    return assessment;
}
} // namespace asst::blackflow
