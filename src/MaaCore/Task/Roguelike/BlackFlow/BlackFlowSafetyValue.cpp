#include "BlackFlowSafetyValue.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <unordered_set>
#include <utility>

#include "BlackFlowStateSpace.h"

namespace asst::blackflow
{
namespace
{
int saturated_action_requirement(int successor_requirement, int cost, int gain) noexcept
{
    if (successor_requirement >= UnreachableActionPointRequirement) {
        return UnreachableActionPointRequirement;
    }
    const std::int64_t required =
        static_cast<std::int64_t>(successor_requirement) + static_cast<std::int64_t>(cost) - gain;
    return static_cast<int>(std::clamp<std::int64_t>(required, 0, UnreachableActionPointRequirement));
}
} // namespace

SafetyValueSolver::SafetyValueSolver(LazySafetyValueProblem problem) :
    m_problem(std::move(problem))
{
}

void SafetyValueSolver::report_error(std::string message, std::string* error)
{
    if (!m_problem.instance_name.empty()) {
        message = m_problem.instance_name + ": " + message;
    }
    m_error = std::move(message);
    if (error != nullptr) {
        *error = m_error;
    }
}

void SafetyValueSolver::reset()
{
    m_states.clear();
    m_bounded_results.clear();
    m_dominance_frontiers.clear();
    m_statistics = {};
    m_error.clear();
    m_requires_full_fixed_point = false;
}

std::optional<SafetyValueSolver::BoundedResult>
    SafetyValueSolver::dominated_result(SafetyStateId state, int action_points) const
{
    if (!m_problem.dominance_descriptor) {
        return std::nullopt;
    }
    const auto descriptor = m_problem.dominance_descriptor(state);
    if (!descriptor.has_value()) {
        return std::nullopt;
    }
    const auto frontier = m_dominance_frontiers.find(descriptor->structure_hash);
    if (frontier == m_dominance_frontiers.end()) {
        return std::nullopt;
    }
    const auto resources_at_least = [](const auto& lhs, const auto& rhs) {
        for (std::size_t index = 0; index < lhs.size(); ++index) {
            if (lhs[index] < rhs[index]) {
                return false;
            }
        }
        return true;
    };
    for (const DominanceEntry& entry : frontier->second) {
        if (entry.descriptor.structure != descriptor->structure) {
            continue;
        }
        if (entry.safe && action_points >= entry.action_points &&
            resources_at_least(descriptor->movement_charges, entry.descriptor.movement_charges)) {
            return BoundedResult {
                false,
                0,
                entry.proof_depth,
                entry.selected_action,
            };
        }
        if (!entry.safe && entry.action_points >= action_points &&
            resources_at_least(entry.descriptor.movement_charges, descriptor->movement_charges)) {
            return BoundedResult {
                false,
                UnreachableActionPointRequirement,
                0,
                std::nullopt,
            };
        }
    }
    return std::nullopt;
}

void SafetyValueSolver::remember_dominance(SafetyStateId state, int action_points, const BoundedResult& result)
{
    if (!m_problem.dominance_descriptor || result.visiting || m_requires_full_fixed_point) {
        return;
    }
    const auto descriptor = m_problem.dominance_descriptor(state);
    if (!descriptor.has_value()) {
        return;
    }
    const bool safe = result.value < UnreachableActionPointRequirement;
    const auto resources_at_least = [](const auto& lhs, const auto& rhs) {
        for (std::size_t index = 0; index < lhs.size(); ++index) {
            if (lhs[index] < rhs[index]) {
                return false;
            }
        }
        return true;
    };
    auto& frontier = m_dominance_frontiers[descriptor->structure_hash];
    for (const DominanceEntry& entry : frontier) {
        if (entry.safe != safe || entry.descriptor.structure != descriptor->structure) {
            continue;
        }
        const bool existing_dominates =
            safe ? entry.action_points <= action_points &&
                       resources_at_least(descriptor->movement_charges, entry.descriptor.movement_charges)
                 : entry.action_points >= action_points &&
                       resources_at_least(entry.descriptor.movement_charges, descriptor->movement_charges);
        if (existing_dominates) {
            return;
        }
    }
    std::erase_if(frontier, [&](const DominanceEntry& entry) {
        if (entry.safe != safe || entry.descriptor.structure != descriptor->structure) {
            return false;
        }
        return safe ? action_points <= entry.action_points &&
                          resources_at_least(entry.descriptor.movement_charges, descriptor->movement_charges)
                    : action_points >= entry.action_points &&
                          resources_at_least(descriptor->movement_charges, entry.descriptor.movement_charges);
    });
    frontier.emplace_back(
        DominanceEntry {
            *descriptor,
            action_points,
            safe,
            result.proof_depth,
            result.selected_action,
        });
}

bool SafetyValueSolver::validate_action(SafetyStateId source, const SafetyValueAction& action, std::string& error) const
{
    if (action.id.empty()) {
        error = "state " + std::to_string(source) + " has an action with an empty id";
        return false;
    }
    if (action.action_point_cost < 0 || action.minimum_action_points_to_start < 0 || action.outcomes.empty()) {
        error = "action " + action.id + " has an invalid cost, start requirement, or outcome list";
        return false;
    }
    if (std::ranges::any_of(action.outcomes, [](const SafetyValueOutcome& outcome) {
            return outcome.action_point_gain < 0;
        })) {
        error = "action " + action.id + " has a negative action-point gain";
        return false;
    }
    return true;
}

bool SafetyValueSolver::discover_closure(SafetyStateId initial)
{
    if (!m_problem.is_goal || !m_problem.expand_actions) {
        m_error = "safety value problem requires a goal predicate and an action expander";
        return false;
    }

    std::deque<SafetyStateId> pending;
    std::unordered_set<SafetyStateId> queued;
    pending.emplace_back(initial);
    queued.emplace(initial);
    while (!pending.empty()) {
        const SafetyStateId state = pending.front();
        pending.pop_front();
        queued.erase(state);

        auto [cached_iter, inserted] = m_states.try_emplace(state);
        if (inserted) {
            ++m_statistics.discovered_states;
        }
        if (cached_iter->second.expanded) {
            continue;
        }

        const bool goal = m_problem.is_goal(state);
        std::vector<SafetyValueAction> actions;
        if (!goal) {
            std::string expansion_error;
            if (!m_problem.expand_actions(state, actions, expansion_error)) {
                m_error = expansion_error.empty() ? "lazy action expansion failed for state " + std::to_string(state)
                                                  : std::move(expansion_error);
                return false;
            }
            std::unordered_set<std::string> action_ids;
            for (const SafetyValueAction& action : actions) {
                std::string validation_error;
                if (!validate_action(state, action, validation_error)) {
                    m_error = std::move(validation_error);
                    return false;
                }
                if (!action_ids.emplace(action.id).second) {
                    m_error = "state " + std::to_string(state) + " has duplicate action id " + action.id;
                    return false;
                }
            }
            std::ranges::sort(actions, {}, &SafetyValueAction::id);
        }

        CachedState& cached = m_states.at(state);
        cached.goal = goal;
        cached.expanded = true;
        cached.value = goal ? 0 : UnreachableActionPointRequirement;
        cached.actions = actions;
        cached.action_enabled.assign(actions.size(), 1);
        cached.selected_action.reset();
        ++m_statistics.expanded_states;

        for (const SafetyValueAction& action : actions) {
            for (const SafetyValueOutcome& outcome : action.outcomes) {
                auto [successor_iter, successor_inserted] = m_states.try_emplace(outcome.successor);
                if (successor_inserted) {
                    ++m_statistics.discovered_states;
                }
                successor_iter->second.predecessors.emplace_back(state);
                if (!successor_iter->second.expanded && queued.emplace(outcome.successor).second) {
                    pending.emplace_back(outcome.successor);
                }
            }
        }
    }
    return true;
}

bool SafetyValueSolver::discover_bounded_closure(SafetyStateId initial, int maximum_action_points)
{
    if (!m_problem.is_goal || !m_problem.expand_actions || maximum_action_points < 0) {
        m_error = "bounded safety value problem requires valid callbacks and a nonnegative action-point limit";
        return false;
    }

    std::deque<SafetyStateId> pending;
    std::unordered_map<SafetyStateId, int> best_action_points;
    pending.emplace_back(initial);
    best_action_points.emplace(initial, maximum_action_points);
    while (!pending.empty()) {
        const SafetyStateId state = pending.front();
        pending.pop_front();
        const int action_points = best_action_points.at(state);

        auto [cached_iter, inserted] = m_states.try_emplace(state);
        if (inserted) {
            ++m_statistics.discovered_states;
        }
        CachedState& cached = cached_iter->second;
        if (!cached.expanded) {
            cached.goal = m_problem.is_goal(state);
            cached.value = cached.goal ? 0 : UnreachableActionPointRequirement;
            cached.selected_action.reset();
            if (!cached.goal) {
                std::string expansion_error;
                if (!m_problem.expand_actions(state, cached.actions, expansion_error)) {
                    m_error = expansion_error.empty()
                                  ? "lazy action expansion failed for state " + std::to_string(state)
                                  : std::move(expansion_error);
                    return false;
                }
                std::unordered_set<std::string> action_ids;
                for (const SafetyValueAction& action : cached.actions) {
                    std::string validation_error;
                    if (!validate_action(state, action, validation_error)) {
                        m_error = std::move(validation_error);
                        return false;
                    }
                    if (!action_ids.emplace(action.id).second) {
                        m_error = "state " + std::to_string(state) + " has duplicate action id " + action.id;
                        return false;
                    }
                }
                std::ranges::sort(cached.actions, {}, &SafetyValueAction::id);
                cached.action_enabled.assign(cached.actions.size(), 0);
            }
            cached.expanded = true;
            ++m_statistics.expanded_states;
        }
        if (cached.goal) {
            continue;
        }

        for (std::size_t action_index = 0; action_index < cached.actions.size(); ++action_index) {
            const SafetyValueAction& action = cached.actions[action_index];
            if (action_points < action.minimum_action_points_to_start || action_points < action.action_point_cost) {
                continue;
            }
            if (cached.action_enabled[action_index] == 0) {
                cached.action_enabled[action_index] = 1;
                for (const SafetyValueOutcome& outcome : action.outcomes) {
                    auto [successor_iter, successor_inserted] = m_states.try_emplace(outcome.successor);
                    if (successor_inserted) {
                        ++m_statistics.discovered_states;
                    }
                    auto& predecessors = successor_iter->second.predecessors;
                    if (std::ranges::find(predecessors, state) == predecessors.end()) {
                        predecessors.emplace_back(state);
                    }
                }
            }
            for (const SafetyValueOutcome& outcome : action.outcomes) {
                const int remaining = std::max(0, action_points - action.action_point_cost + outcome.action_point_gain);
                const auto found = best_action_points.find(outcome.successor);
                if (found == best_action_points.end() || remaining > found->second) {
                    best_action_points.insert_or_assign(outcome.successor, remaining);
                    pending.emplace_back(outcome.successor);
                }
            }
        }
    }
    return true;
}

bool SafetyValueSolver::ensure_bounded_solved(SafetyStateId initial, int maximum_action_points, std::string* error)
{
    reset();
    if (!discover_bounded_closure(initial, maximum_action_points) || !solve_fixed_point()) {
        report_error(m_error.empty() ? "bounded safety value fixed point failed" : std::move(m_error), error);
        return false;
    }
    return true;
}

int SafetyValueSolver::action_value(const SafetyValueAction& action) const noexcept
{
    int requirement = std::max(action.minimum_action_points_to_start, action.action_point_cost);
    for (const SafetyValueOutcome& outcome : action.outcomes) {
        const auto successor = m_states.find(outcome.successor);
        if (successor == m_states.end() || successor->second.value >= UnreachableActionPointRequirement) {
            return UnreachableActionPointRequirement;
        }
        requirement = std::max(
            requirement,
            saturated_action_requirement(successor->second.value, action.action_point_cost, outcome.action_point_gain));
    }
    return requirement;
}

bool SafetyValueSolver::solve_fixed_point()
{
    std::deque<SafetyStateId> pending;
    std::unordered_set<SafetyStateId> queued;
    for (const auto& [state, cached] : m_states) {
        if (!cached.goal) {
            pending.emplace_back(state);
            queued.emplace(state);
        }
    }

    while (!pending.empty()) {
        const SafetyStateId state = pending.front();
        pending.pop_front();
        queued.erase(state);
        CachedState& cached = m_states.at(state);

        int best = UnreachableActionPointRequirement;
        std::optional<std::string> selected;
        for (std::size_t action_index = 0; action_index < cached.actions.size(); ++action_index) {
            if (action_index >= cached.action_enabled.size() || cached.action_enabled[action_index] == 0) {
                continue;
            }
            const SafetyValueAction& action = cached.actions[action_index];
            const int candidate = action_value(action);
            if (candidate < best || (candidate == best && candidate < UnreachableActionPointRequirement &&
                                     (!selected.has_value() || action.id < *selected))) {
                best = candidate;
                selected = action.id;
            }
        }
        if (best >= cached.value) {
            if (best == cached.value && selected.has_value() &&
                (!cached.selected_action.has_value() || *selected < *cached.selected_action)) {
                cached.selected_action = std::move(selected);
            }
            continue;
        }

        cached.value = best;
        cached.selected_action = std::move(selected);
        ++m_statistics.fixed_point_updates;
        for (const SafetyStateId predecessor : cached.predecessors) {
            if (queued.emplace(predecessor).second) {
                pending.emplace_back(predecessor);
            }
        }
    }
    return true;
}

bool SafetyValueSolver::ensure_solved(SafetyStateId initial, std::string* error)
{
    m_error.clear();
    if (!discover_closure(initial) || !solve_fixed_point()) {
        report_error(m_error.empty() ? "safety value fixed point failed" : std::move(m_error), error);
        return false;
    }
    return true;
}

int SafetyValueSolver::N(SafetyStateId state, std::string* error)
{
    if (!ensure_solved(state, error)) {
        return UnreachableActionPointRequirement;
    }
    return m_states.at(state).value;
}

int SafetyValueSolver::solve_bounded_recursive(SafetyStateId state, int maximum_action_points)
{
    if (maximum_action_points < 0 || maximum_action_points >= UnreachableActionPointRequirement) {
        return UnreachableActionPointRequirement;
    }
    const auto make_key = [](SafetyStateId state_id, int action_points) {
        return (static_cast<std::uint64_t>(state_id) << 32U) | static_cast<std::uint32_t>(action_points);
    };
    const std::uint64_t key = make_key(state, maximum_action_points);
    if (const auto dominated = dominated_result(state, maximum_action_points); dominated.has_value()) {
        m_bounded_results.emplace(key, *dominated);
        return dominated->value;
    }
    auto [result_iter, inserted_result] = m_bounded_results.try_emplace(key);
    if (!inserted_result) {
        if (result_iter->second.visiting) {
            m_requires_full_fixed_point = true;
            return UnreachableActionPointRequirement;
        }
        return result_iter->second.value;
    }
    result_iter->second.visiting = true;

    auto [cached_iter, inserted_state] = m_states.try_emplace(state);
    if (inserted_state) {
        ++m_statistics.discovered_states;
    }
    CachedState& cached = cached_iter->second;
    if (!cached.expanded) {
        cached.goal = m_problem.is_goal(state);
        cached.value = cached.goal ? 0 : UnreachableActionPointRequirement;
        if (!cached.goal) {
            std::string expansion_error;
            if (!m_problem.expand_actions(state, cached.actions, expansion_error)) {
                m_error = expansion_error.empty() ? "lazy action expansion failed for state " + std::to_string(state)
                                                  : std::move(expansion_error);
                m_bounded_results.at(key).visiting = false;
                return UnreachableActionPointRequirement;
            }
            std::unordered_set<std::string> action_ids;
            for (const SafetyValueAction& action : cached.actions) {
                std::string validation_error;
                if (!validate_action(state, action, validation_error)) {
                    m_error = std::move(validation_error);
                    m_bounded_results.at(key).visiting = false;
                    return UnreachableActionPointRequirement;
                }
                if (!action_ids.emplace(action.id).second) {
                    m_error = "state " + std::to_string(state) + " has duplicate action id " + action.id;
                    m_bounded_results.at(key).visiting = false;
                    return UnreachableActionPointRequirement;
                }
            }
            std::ranges::sort(cached.actions, {}, &SafetyValueAction::id);
            cached.action_enabled.assign(cached.actions.size(), 1);
        }
        cached.expanded = true;
        ++m_statistics.expanded_states;
    }
    if (cached.goal) {
        BoundedResult& result = m_bounded_results.at(key);
        result.visiting = false;
        result.value = 0;
        result.proof_depth = 0;
        result.selected_action.reset();
        remember_dominance(state, maximum_action_points, result);
        return 0;
    }
    const std::vector<SafetyValueAction> actions = cached.actions;

    const auto attempt = [&](const SafetyValueAction& action) {
        const int base = std::max(action.minimum_action_points_to_start, action.action_point_cost);
        if (base > maximum_action_points) {
            return false;
        }
        std::size_t depth = 0;
        for (const SafetyValueOutcome& outcome : action.outcomes) {
            const std::int64_t remaining64 =
                static_cast<std::int64_t>(maximum_action_points) - action.action_point_cost + outcome.action_point_gain;
            if (remaining64 < 0 || remaining64 >= UnreachableActionPointRequirement) {
                return false;
            }
            const int remaining = static_cast<int>(remaining64);
            if (solve_bounded_recursive(outcome.successor, remaining) >= UnreachableActionPointRequirement) {
                return false;
            }
            const auto successor_result = m_bounded_results.find(make_key(outcome.successor, remaining));
            if (successor_result == m_bounded_results.end() || successor_result->second.visiting) {
                return false;
            }
            depth = std::max(depth, successor_result->second.proof_depth + 1);
        }
        BoundedResult& result = m_bounded_results.at(key);
        result.visiting = false;
        result.value = 0;
        result.proof_depth = depth;
        result.selected_action = action.id;
        CachedState& state_cache = m_states.at(state);
        if (maximum_action_points < state_cache.value) {
            state_cache.value = maximum_action_points;
            state_cache.selected_action = action.id;
        }
        remember_dominance(state, maximum_action_points, result);
        return true;
    };
    for (const SafetyValueAction& action : actions) {
        const bool direct_goal = std::ranges::all_of(action.outcomes, [&](const SafetyValueOutcome& outcome) {
            return m_problem.is_goal(outcome.successor);
        });
        if (direct_goal && attempt(action)) {
            return 0;
        }
    }
    for (const SafetyValueAction& action : actions) {
        const bool direct_goal = std::ranges::all_of(action.outcomes, [&](const SafetyValueOutcome& outcome) {
            return m_problem.is_goal(outcome.successor);
        });
        if (!direct_goal && attempt(action)) {
            return 0;
        }
    }

    BoundedResult& result = m_bounded_results.at(key);
    result.visiting = false;
    result.value = UnreachableActionPointRequirement;
    result.proof_depth = 0;
    result.selected_action.reset();
    remember_dominance(state, maximum_action_points, result);
    return UnreachableActionPointRequirement;
}

int SafetyValueSolver::N_bounded(SafetyStateId state, int maximum_action_points, std::string* error)
{
    m_error.clear();
    if (!m_problem.is_goal || !m_problem.expand_actions || maximum_action_points < 0) {
        report_error("bounded safety value query is invalid", error);
        return UnreachableActionPointRequirement;
    }
    if (m_requires_full_fixed_point) {
        SafetyValueSolver fallback(m_problem);
        const int exact = fallback.N(state, error);
        return exact <= maximum_action_points ? exact : UnreachableActionPointRequirement;
    }

    for (int action_points = 0; action_points <= maximum_action_points; ++action_points) {
        const int value = solve_bounded_recursive(state, action_points);
        if (!m_error.empty()) {
            report_error(std::move(m_error), error);
            return UnreachableActionPointRequirement;
        }
        if (m_requires_full_fixed_point) {
            SafetyValueSolver fallback(m_problem);
            const int exact = fallback.N(state, error);
            return exact <= maximum_action_points ? exact : UnreachableActionPointRequirement;
        }
        if (value < UnreachableActionPointRequirement) {
            return action_points;
        }
    }
    return UnreachableActionPointRequirement;
}

std::optional<std::string>
    SafetyValueSolver::bounded_witness(SafetyStateId state, int maximum_action_points, std::string* error)
{
    const int requirement = N_bounded(state, maximum_action_points, error);
    if (requirement >= UnreachableActionPointRequirement) {
        return std::nullopt;
    }
    if (m_requires_full_fixed_point) {
        SafetyValueSolver fallback(m_problem);
        return fallback.witness(state, error);
    }
    const std::uint64_t key = (static_cast<std::uint64_t>(state) << 32U) | static_cast<std::uint32_t>(requirement);
    const auto found = m_bounded_results.find(key);
    return found == m_bounded_results.end() ? std::nullopt : found->second.selected_action;
}

std::optional<std::size_t>
    SafetyValueSolver::bounded_proof_depth(SafetyStateId state, int maximum_action_points, std::string* error)
{
    const int requirement = N_bounded(state, maximum_action_points, error);
    if (requirement >= UnreachableActionPointRequirement || m_requires_full_fixed_point) {
        return std::nullopt;
    }
    const std::uint64_t key = (static_cast<std::uint64_t>(state) << 32U) | static_cast<std::uint32_t>(requirement);
    const auto found = m_bounded_results.find(key);
    return found == m_bounded_results.end() ? std::nullopt : std::optional<std::size_t>(found->second.proof_depth);
}

int SafetyValueSolver::cached_requirement(SafetyStateId state) const noexcept
{
    const auto found = m_states.find(state);
    return found == m_states.end() ? UnreachableActionPointRequirement : found->second.value;
}

int SafetyValueSolver::Q(SafetyStateId state, std::string_view action_id, std::string* error)
{
    if (!ensure_solved(state, error)) {
        return UnreachableActionPointRequirement;
    }
    const CachedState& cached = m_states.at(state);
    const auto action = std::ranges::find(cached.actions, action_id, &SafetyValueAction::id);
    const std::size_t action_index = static_cast<std::size_t>(std::distance(cached.actions.begin(), action));
    if (action == cached.actions.end() || action_index >= cached.action_enabled.size() ||
        cached.action_enabled[action_index] == 0) {
        report_error("state " + std::to_string(state) + " has no enabled action " + std::string(action_id), error);
        return UnreachableActionPointRequirement;
    }
    return action_value(*action);
}

std::optional<std::string> SafetyValueSolver::witness(SafetyStateId state, std::string* error)
{
    if (!ensure_solved(state, error)) {
        return std::nullopt;
    }
    return m_states.at(state).selected_action;
}

bool SafetyValueSolver::certifies(SafetyStateId state, int action_points, std::string* error)
{
    const int requirement = N(state, error);
    return requirement < UnreachableActionPointRequirement && action_points >= requirement;
}

LazySafetyValueProblem make_on_demand_safety_value_problem(
    OnDemandStateGraph& graph,
    std::string instance_name,
    OnDemandSafetyValueGoalPredicate goal_predicate)
{
    LazySafetyValueProblem problem;
    problem.instance_name = std::move(instance_name);
    problem.is_goal = [&graph, goal_predicate = std::move(goal_predicate)](SafetyStateId state) {
        if (goal_predicate) {
            return goal_predicate(state, graph.state(state));
        }
        return graph.is_terminal(state);
    };
    problem.expand_actions =
        [&graph](SafetyStateId state, std::vector<SafetyValueAction>& converted, std::string& error) {
            const auto* actions = graph.actions(state, &error);
            if (actions == nullptr) {
                return false;
            }
            converted.clear();
            converted.reserve(actions->size());
            for (const OnDemandSafetyAction& action : *actions) {
                SafetyValueAction value_action;
                value_action.id = action.candidate.action_id;
                value_action.action_point_cost = action.action_point_cost;
                value_action.minimum_action_points_to_start = action.minimum_action_points_to_start;
                value_action.outcomes.reserve(action.outcomes.size());
                for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
                    value_action.outcomes.emplace_back(
                        SafetyValueOutcome {
                            outcome.successor,
                            outcome.action_point_gain,
                        });
                }
                converted.emplace_back(std::move(value_action));
            }
            return true;
        };
    problem.dominance_descriptor = [&graph](SafetyStateId state) -> std::optional<SafetyDominanceDescriptor> {
        const PlannerState& planner = graph.state(state);
        SafetyDominanceDescriptor descriptor;
        descriptor.structure = {
            static_cast<std::uint64_t>(planner.node),
            planner.completed_nodes,
            planner.opened_blockers,
            planner.consumed_lights,
            (static_cast<std::uint64_t>(planner.goal_progress_id) << 1U) | static_cast<std::uint64_t>(planner.terminal),
        };
        descriptor.movement_charges = planner.movement_charges;
        std::uint64_t hash = 1'469'598'103'934'665'603ULL;
        for (const std::uint64_t value : descriptor.structure) {
            hash ^= value;
            hash *= 1'099'511'628'211ULL;
        }
        descriptor.structure_hash = hash;
        return descriptor;
    };
    return problem;
}
} // namespace asst::blackflow
