#include "BlackFlowStateSpace.h"

#include <algorithm>
#include <deque>
#include <limits>

namespace asst::blackflow
{
namespace
{
std::size_t combine_hash(std::size_t seed, std::size_t value) noexcept
{
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

bool contains_sorted(const std::vector<NodeId>& values, NodeId value)
{
    return std::ranges::binary_search(values, value);
}

void insert_sorted(std::vector<NodeId>& values, NodeId value)
{
    const auto position = std::ranges::lower_bound(values, value);
    if (position == values.end() || *position != value) {
        values.insert(position, value);
    }
}

std::size_t movement_index(MovementKind movement) noexcept
{
    return static_cast<std::size_t>(movement);
}

RunState materialize_run_state(const RunState& source, const PlannerState& state)
{
    RunState result = source;
    result.current_node = state.node;
    result.resources.action_points = std::numeric_limits<int>::max() / 8;
    result.resources.movement_charges.clear();
    result.cross_floor_expired.clear();
    for (const auto& spec : movement_specs()) {
        if (spec.kind != MovementKind::Walk) {
            result.resources.movement_charges.emplace(
                spec.kind,
                static_cast<int>(state.movement_charges[movement_index(spec.kind)]));
            if (state.cross_floor_expired[movement_index(spec.kind)]) {
                result.cross_floor_expired.emplace(spec.kind);
            }
        }
    }
    result.node_progress.clear();
    result.visited_nodes.clear();
    result.visited_nodes.insert(state.visited_nodes.begin(), state.visited_nodes.end());
    for (const NodeId completed : state.completed_nodes) {
        result.node_progress.emplace(completed, NodeProgress::Completed);
    }
    result.consumed_one_time_nodes.clear();
    result.consumed_one_time_nodes.insert(state.consumed_one_time_nodes.begin(), state.consumed_one_time_nodes.end());
    result.revealed_nodes.clear();
    result.revealed_nodes.insert(state.revealed_nodes.begin(), state.revealed_nodes.end());
    return result;
}

bool unavailable_target(const MapSnapshot& map, const PlannerState& state, NodeId target)
{
    const Node* node = map.find_node(target);
    return node == nullptr || (contains_sorted(state.completed_nodes, target) && !node->traversal.repeatable);
}

PlannerState
    transition_state(const MapSnapshot& map, const PlannerState& source, const MoveAction& action, NodeId landing)
{
    PlannerState successor = source;
    successor.node = landing;
    if (action.candidate.movement != MovementKind::Walk) {
        auto& charge = successor.movement_charges[movement_index(action.candidate.movement)];
        if (charge > 0) {
            --charge;
        }
    }

    NodeId completed = action.candidate.target;
    if (completed == InvalidNodeId) {
        completed = landing;
    }
    insert_sorted(successor.visited_nodes, completed);
    const Node* node = map.find_node(completed);
    if (node != nullptr && !node->traversal.repeatable && node->type != NodeType::Empty) {
        insert_sorted(successor.completed_nodes, completed);
    }
    if (node != nullptr && node->type == NodeType::FeatherPoint &&
        !contains_sorted(successor.consumed_one_time_nodes, completed)) {
        insert_sorted(successor.consumed_one_time_nodes, completed);
        for (const NodeId revealed : map.nodes_within_manhattan(completed, 2)) {
            insert_sorted(successor.revealed_nodes, revealed);
        }
    }
    return successor;
}

int action_gain(const MapSnapshot& map, const PlannerState& source, const MoveAction& action, NodeId landing)
{
    const MovementSpec* movement = find_movement_spec(action.candidate.movement);
    int gain = movement == nullptr ? 0 : movement->effect.action_point_gain;
    NodeId effect_node = action.candidate.target;
    if (effect_node == InvalidNodeId) {
        effect_node = landing;
    }
    const Node* node = map.find_node(effect_node);
    if (node != nullptr && node->type == NodeType::FeatherPoint &&
        !contains_sorted(source.consumed_one_time_nodes, effect_node)) {
        ++gain;
    }
    return gain;
}
} // namespace

std::size_t PlannerStateHash::operator()(const PlannerState& state) const noexcept
{
    std::size_t seed = std::hash<NodeId> {}(state.node);
    for (const auto charge : state.movement_charges) {
        seed = combine_hash(seed, std::hash<std::uint8_t> {}(charge));
    }
    for (const bool expired : state.cross_floor_expired) {
        seed = combine_hash(seed, std::hash<bool> {}(expired));
    }
    for (const NodeId node : state.completed_nodes) {
        seed = combine_hash(seed, std::hash<NodeId> {}(node));
    }
    for (const NodeId node : state.visited_nodes) {
        seed = combine_hash(seed, std::hash<NodeId> {}(node));
    }
    for (const NodeId node : state.consumed_one_time_nodes) {
        seed = combine_hash(seed, std::hash<NodeId> {}(node));
    }
    for (const NodeId node : state.revealed_nodes) {
        seed = combine_hash(seed, std::hash<NodeId> {}(node));
    }
    seed = combine_hash(seed, std::hash<std::uint64_t> {}(state.dynamic_cost_revision));
    return combine_hash(seed, std::hash<bool> {}(state.terminal));
}

bool BlackFlowStateExpander::is_terminal(
    const MapSnapshot& map,
    const PlannerState& state,
    const StateExpansionOptions& options) const noexcept
{
    if (state.terminal || options.strategy_terminal_nodes.contains(state.node)) {
        return true;
    }
    const Node* node = map.find_node(state.node);
    if (node == nullptr) {
        return false;
    }
    return (options.final_is_terminal && node->type == NodeType::Final) ||
           (options.fate_is_terminal && node->type == NodeType::Fate);
}

std::optional<ExpandedSafetyProblem> BlackFlowStateExpander::build(
    const MapSnapshot& map,
    const RunState& run,
    const StateExpansionOptions& options,
    std::string* error) const
{
    if (map.find_node(run.current_node) == nullptr || options.maximum_states == 0) {
        if (error != nullptr) {
            *error = "state expansion requires a valid current node and a positive state limit";
        }
        return std::nullopt;
    }
    std::string validation_error;
    if (!run.costs.validate(&validation_error)) {
        if (error != nullptr) {
            *error = validation_error;
        }
        return std::nullopt;
    }

    ExpandedSafetyProblem expanded;
    PlannerState initial;
    initial.node = run.current_node;
    initial.dynamic_cost_revision = run.costs.revision;
    for (const auto& spec : movement_specs()) {
        if (spec.kind == MovementKind::Walk) {
            continue;
        }
        int charges = 0;
        if (const auto found = run.resources.movement_charges.find(spec.kind);
            found != run.resources.movement_charges.end() && !run.cross_floor_expired.contains(spec.kind)) {
            charges = std::clamp(found->second, 0, 255);
        }
        initial.movement_charges[movement_index(spec.kind)] = static_cast<std::uint8_t>(charges);
        initial.cross_floor_expired[movement_index(spec.kind)] = run.cross_floor_expired.contains(spec.kind);
    }
    for (const auto& [node, progress] : run.node_progress) {
        if (progress == NodeProgress::Completed) {
            initial.completed_nodes.emplace_back(node);
        }
    }
    std::ranges::sort(initial.completed_nodes);
    initial.completed_nodes.erase(
        std::unique(initial.completed_nodes.begin(), initial.completed_nodes.end()),
        initial.completed_nodes.end());
    initial.visited_nodes.assign(run.visited_nodes.begin(), run.visited_nodes.end());
    std::ranges::sort(initial.visited_nodes);
    initial.consumed_one_time_nodes.assign(run.consumed_one_time_nodes.begin(), run.consumed_one_time_nodes.end());
    std::ranges::sort(initial.consumed_one_time_nodes);
    initial.revealed_nodes.assign(run.revealed_nodes.begin(), run.revealed_nodes.end());
    for (const auto& [node_id, node] : map.nodes()) {
        if (node.identity_revealed) {
            insert_sorted(initial.revealed_nodes, node_id);
        }
        if (node.type == NodeType::FeatherPoint) {
            for (const NodeId revealed : map.nodes_within_manhattan(node_id, 1)) {
                insert_sorted(initial.revealed_nodes, revealed);
            }
        }
    }

    std::unordered_map<PlannerState, SafetyStateId, PlannerStateHash> ids;
    std::deque<SafetyStateId> pending;
    auto add_state = [&](PlannerState state) -> std::optional<SafetyStateId> {
        const auto found = ids.find(state);
        if (found != ids.end()) {
            return found->second;
        }
        if (expanded.planner_states.size() >= options.maximum_states) {
            return std::nullopt;
        }
        const SafetyStateId id = static_cast<SafetyStateId>(expanded.planner_states.size());
        const bool terminal = is_terminal(map, state, options);
        state.terminal = terminal;
        expanded.planner_states.emplace_back(std::move(state));
        ids.emplace(expanded.planner_states.back(), id);
        expanded.problem.states.emplace_back(
            SafetyState { id, terminal, std::to_string(expanded.planner_states.back().node) });
        pending.emplace_back(id);
        return id;
    };

    const auto initial_id = add_state(std::move(initial));
    if (!initial_id.has_value()) {
        if (error != nullptr) {
            *error = "failed to create initial planner state";
        }
        return std::nullopt;
    }
    expanded.initial_state = *initial_id;

    while (!pending.empty()) {
        const SafetyStateId source_id = pending.front();
        pending.pop_front();
        const PlannerState source = expanded.planner_states[source_id];
        if (is_terminal(map, source, options)) {
            continue;
        }

        const RunState materialized = materialize_run_state(run, source);
        const auto actions = enumerate_move_actions(map, materialized, options.knowledge);
        for (const auto& move : actions) {
            if (options.forbidden_action_ids.contains(move.candidate.action_id)) {
                continue;
            }
            std::vector<SafetyOutcome> outcomes;
            for (const NodeId landing : move.possible_landings) {
                NodeId target = move.candidate.target == InvalidNodeId ? landing : move.candidate.target;
                if (unavailable_target(map, source, target)) {
                    continue;
                }
                PlannerState successor = transition_state(map, source, move, landing);
                successor.terminal = is_terminal(map, successor, options);
                const auto successor_id = add_state(std::move(successor));
                if (!successor_id.has_value()) {
                    if (error != nullptr) {
                        *error = "state expansion exceeded the configured finite-state limit";
                    }
                    return std::nullopt;
                }
                outcomes.emplace_back(SafetyOutcome { *successor_id, action_gain(map, source, move, landing) });
            }
            std::ranges::sort(outcomes, {}, &SafetyOutcome::successor);
            outcomes.erase(
                std::unique(
                    outcomes.begin(),
                    outcomes.end(),
                    [](const SafetyOutcome& lhs, const SafetyOutcome& rhs) {
                        return lhs.successor == rhs.successor && lhs.action_point_gain == rhs.action_point_gain;
                    }),
                outcomes.end());
            if (outcomes.empty()) {
                continue;
            }

            const std::string action_id = std::to_string(source_id) + ":" + move.candidate.action_id;
            expanded.problem.actions.emplace_back(
                SafetyAction {
                    action_id,
                    source_id,
                    move.candidate.predicted_action_point_cost,
                    1,
                    std::move(outcomes),
                });
            expanded.action_candidates.insert_or_assign(action_id, move.candidate);
        }
    }
    return expanded;
}
} // namespace asst::blackflow

