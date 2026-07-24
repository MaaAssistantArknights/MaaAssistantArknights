#include "BlackFlowStateSpace.h"

#include "BlackFlowCompactStateSpace.h"

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

std::size_t movement_index(MovementKind movement) noexcept
{
    return static_cast<std::size_t>(movement);
}

std::optional<PlannerNodeMask>
    node_bit(const std::unordered_map<NodeId, std::uint8_t>& node_indices, NodeId node) noexcept
{
    const auto found = node_indices.find(node);
    if (found == node_indices.end()) {
        return std::nullopt;
    }
    return PlannerNodeMask { 1 } << found->second;
}

RunState materialize_run_state(
    const RunState& source,
    const PlannerState& state,
    const std::vector<NodeId>& indexed_nodes,
    int action_points)
{
    RunState result = source;
    result.current_node = state.node;
    result.resources.action_points = action_points;
    result.resources.movement_charges.clear();
    for (const auto& spec : movement_specs()) {
        if (spec.kind != MovementKind::Walk) {
            result.resources.movement_charges.emplace(
                spec.kind,
                static_cast<int>(state.movement_charges[movement_index(spec.kind)]));
        }
    }

    result.node_progress.clear();
    result.visited_nodes.clear();
    result.consumed_one_time_nodes.clear();
    result.revealed_nodes = source.revealed_nodes;
    for (std::size_t index = 0; index < indexed_nodes.size(); ++index) {
        const PlannerNodeMask bit = PlannerNodeMask { 1 } << index;
        const NodeId node = indexed_nodes[index];
        if ((state.completed_nodes & bit) != 0) {
            result.node_progress.emplace(node, NodeProgress::Completed);
        }
        if ((state.opened_blockers & bit) != 0) {
            result.visited_nodes.emplace(node);
        }
        if ((state.consumed_lights & bit) != 0) {
            result.consumed_one_time_nodes.emplace(node);
        }
    }
    return result;
}

bool unavailable_target(
    const MapSnapshot& map,
    const PlannerState& state,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices,
    NodeId target)
{
    const Node* node = map.find_node(target);
    const auto bit = node_bit(node_indices, target);
    return node == nullptr || !bit.has_value() ||
           (((state.completed_nodes & *bit) != 0) && !node->traversal.repeatable);
}

PlannerState transition_state(
    const MapSnapshot& map,
    const PlannerState& source,
    const MoveAction& action,
    NodeId landing,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices)
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
    const Node* node = map.find_node(completed);
    const auto bit = node_bit(node_indices, completed);
    if (node == nullptr || !bit.has_value()) {
        return successor;
    }

    if (!node->traversal.repeatable && node->type != NodeType::Empty) {
        successor.completed_nodes |= *bit;
        successor.opened_blockers &= ~*bit;
    }
    else if (node->traversal.blocks_walk) {
        successor.opened_blockers |= *bit;
    }

    if (node->type == NodeType::Light) {
        successor.consumed_lights |= *bit;
    }
    return successor;
}

int action_gain(
    const MapSnapshot& map,
    const PlannerState& source,
    const MoveAction& action,
    NodeId landing,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices)
{
    const MovementSpec* movement = find_movement_spec(action.candidate.movement);
    int gain = movement == nullptr ? 0 : movement->effect.action_point_gain;
    NodeId effect_node = action.candidate.target;
    if (effect_node == InvalidNodeId) {
        effect_node = landing;
    }
    const Node* node = map.find_node(effect_node);
    const auto bit = node_bit(node_indices, effect_node);
    if (node != nullptr && bit.has_value() && node->type == NodeType::Light && (source.consumed_lights & *bit) == 0) {
        ++gain;
    }
    return gain;
}

bool revealed_by_consumed_light(
    const MapSnapshot& map,
    const PlannerState& state,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices,
    NodeId node)
{
    for (const auto& [light_id, light] : map.nodes()) {
        const auto light_mask = node_bit(node_indices, light_id);
        if (light.type != NodeType::Light || !light_mask.has_value() || (state.consumed_lights & *light_mask) == 0) {
            continue;
        }
        const auto revealed = map.nodes_within_manhattan(light_id, 2);
        if (std::ranges::find(revealed, node) != revealed.end()) {
            return true;
        }
    }
    return false;
}

int unknown_big_nodes_revealed(
    const MapSnapshot& map,
    const RunState& run,
    const PlannerState& state,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices,
    NodeId light)
{
    const Node* light_node = map.find_node(light);
    if (light_node == nullptr || light_node->type != NodeType::Light) {
        return 0;
    }
    int count = 0;
    for (const NodeId id : map.nodes_within_manhattan(light, 2)) {
        const Node* candidate = map.find_node(id);
        const auto candidate_mask = node_bit(node_indices, id);
        if (candidate == nullptr || candidate->type == NodeType::Empty || candidate->identity_revealed ||
            (candidate_mask.has_value() && (state.completed_nodes & *candidate_mask) != 0) ||
            run.revealed_nodes.contains(id) || revealed_by_consumed_light(map, state, node_indices, id)) {
            continue;
        }
        ++count;
    }
    return count;
}

bool advance_goal_progress(
    const MapSnapshot& map,
    const RunState& run,
    const PlannerState& source,
    NodeId entered_node,
    const StateExpansionOptions& options,
    const std::unordered_map<NodeId, std::uint8_t>& node_indices,
    PlannerState& successor,
    std::string* error)
{
    if (options.safety_goal == nullptr) {
        return true;
    }
    if (options.safety_goal_facts == nullptr) {
        if (error != nullptr) {
            *error = "safety goal facts are missing";
        }
        return false;
    }
    const Node* entered = map.find_node(entered_node);
    if (entered == nullptr) {
        if (error != nullptr) {
            *error = "safety goal transition references a missing entered node";
        }
        return false;
    }
    const int revealed = unknown_big_nodes_revealed(map, run, source, node_indices, entered_node);
    const auto next_progress =
        options.safety_goal
            ->advance_node(source.goal_progress_id, *entered, revealed, *options.safety_goal_facts, error);
    if (!next_progress.has_value()) {
        return false;
    }
    successor.goal_progress_id = *next_progress;
    return true;
}

} // namespace

OnDemandStateGraph::OnDemandStateGraph() = default;
OnDemandStateGraph::~OnDemandStateGraph() = default;

std::optional<ProjectedMoveOutcome> project_move_outcome(
    const MapSnapshot& map,
    const RunState& run,
    const MoveCandidate& move,
    int exact_action_point_cost,
    std::string* error)
{
    const Node* target = map.find_node(move.target);
    const Node* landing = map.find_node(move.landing);
    if (!move.controllable || move.source != run.current_node || target == nullptr || landing == nullptr ||
        target->floor != run.floor || landing->floor != run.floor || run.resources.action_points < 1 ||
        exact_action_point_cost < 0 || exact_action_point_cost > run.resources.action_points) {
        if (error != nullptr) {
            *error = "previewed move cannot be projected from the current run state";
        }
        return std::nullopt;
    }

    const MovementSpec* movement = find_movement_spec(move.movement);
    if (movement == nullptr) {
        if (error != nullptr) {
            *error = "previewed move references an unknown movement";
        }
        return std::nullopt;
    }

    ProjectedMoveOutcome outcome;
    outcome.run = run;
    outcome.action_point_gain = move.predicted_action_point_gain;
    outcome.run.current_node = move.landing;
    outcome.run.resources.action_points =
        action_points_after(run.resources.action_points, exact_action_point_cost, outcome.action_point_gain);
    outcome.run.resources.hope += movement->effect.hope_gain;
    outcome.run.resources.ingots += movement->effect.ingot_gain;

    if (move.movement != MovementKind::Walk) {
        auto charge = outcome.run.resources.movement_charges.find(move.movement);
        if (charge == outcome.run.resources.movement_charges.end() || charge->second <= 0) {
            if (error != nullptr) {
                *error = "previewed move has no remaining movement charge";
            }
            return std::nullopt;
        }
        --charge->second;
    }

    outcome.run.visited_nodes.emplace(move.target);
    if (!target->traversal.repeatable && target->type != NodeType::Empty) {
        outcome.run.node_progress.insert_or_assign(move.target, NodeProgress::Completed);
    }
    if (target->type == NodeType::Light && !outcome.run.consumed_one_time_nodes.contains(move.target)) {
        outcome.run.consumed_one_time_nodes.emplace(move.target);
        const auto revealed = map.nodes_within_manhattan(move.target, 2);
        outcome.run.revealed_nodes.insert(revealed.begin(), revealed.end());
    }
    if (move.movement != MovementKind::Walk && is_combat_node_type(target->type) &&
        outcome.run.resources.white_model_birds > 0) {
        --outcome.run.resources.white_model_birds;
    }
    if (outcome.run.resources != run.resources) {
        ++outcome.run.resources_revision;
    }
    return outcome;
}

std::size_t PlannerStateHash::operator()(const PlannerState& state) const noexcept
{
    std::size_t seed = std::hash<NodeId> {}(state.node);
    for (const auto charge : state.movement_charges) {
        seed = combine_hash(seed, std::hash<std::uint8_t> {}(charge));
    }
    seed = combine_hash(seed, std::hash<PlannerNodeMask> {}(state.completed_nodes));
    seed = combine_hash(seed, std::hash<PlannerNodeMask> {}(state.opened_blockers));
    seed = combine_hash(seed, std::hash<PlannerNodeMask> {}(state.consumed_lights));
    seed = combine_hash(seed, std::hash<SafetyGoalProgressId> {}(state.goal_progress_id));
    return combine_hash(seed, std::hash<bool> {}(state.terminal));
}

bool ExpandedSafetyProblem::mask_contains(PlannerNodeMask mask, NodeId node) const noexcept
{
    const auto bit = node_bit(node_indices, node);
    return bit.has_value() && (mask & *bit) != 0;
}

bool ExpandedSafetyProblem::is_completed(SafetyStateId state, NodeId node) const noexcept
{
    return state < planner_states.size() && mask_contains(planner_states[state].completed_nodes, node);
}

bool OnDemandStateGraph::initialize(
    const MapSnapshot& map,
    const RunState& run,
    StateExpansionOptions options,
    std::string* error)
{
    m_map = &map;
    m_run = &run;
    m_options = std::move(options);
    m_initial_state = 0;
    m_indexed_nodes.clear();
    m_node_indices.clear();
    m_states.clear();
    m_ids.clear();
    m_actions.clear();
    m_compact.reset();

    if ((m_options.safety_goal == nullptr) != (m_options.safety_goal_facts == nullptr)) {
        if (error != nullptr) {
            *error = "on-demand state graph requires safety goal and facts together";
        }
        return false;
    }
    if (map.find_node(run.current_node) == nullptr || m_options.maximum_states == 0) {
        if (error != nullptr) {
            *error = "on-demand state graph requires a valid current node and a positive state limit";
        }
        return false;
    }
    std::string validation_error;
    if (!run.costs.validate(&validation_error)) {
        if (error != nullptr) {
            *error = validation_error;
        }
        return false;
    }
    if (m_options.use_compact_actions) {
        m_compact = std::make_unique<BlackFlowCompactStateSpace>();
        if (!m_compact->initialize(map, run, m_options, error)) {
            m_compact.reset();
            return false;
        }
    }

    for (const auto& [node_id, node] : map.nodes()) {
        if (node.floor == run.floor) {
            m_indexed_nodes.emplace_back(node_id);
        }
    }
    std::ranges::sort(m_indexed_nodes);
    if (m_indexed_nodes.size() > 64) {
        if (error != nullptr) {
            *error = "on-demand compact graph supports at most 64 nodes on one floor";
        }
        return false;
    }
    m_node_indices.reserve(m_indexed_nodes.size());
    for (std::size_t index = 0; index < m_indexed_nodes.size(); ++index) {
        m_node_indices.emplace(m_indexed_nodes[index], static_cast<std::uint8_t>(index));
    }
    m_ids.reserve(std::min<std::size_t>(m_options.maximum_states, 262144));

    PlannerState initial;
    initial.node = run.current_node;
    if (m_options.safety_goal != nullptr) {
        initial.goal_progress_id = m_options.initial_goal_progress_id != InvalidSafetyGoalProgressId
                                       ? m_options.initial_goal_progress_id
                                       : m_options.safety_goal->initial_progress_id();
    }
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
    }
    for (const auto& [node, progress] : run.node_progress) {
        const auto node_mask = bit(node);
        if (progress == NodeProgress::Completed && node_mask.has_value()) {
            initial.completed_nodes |= *node_mask;
        }
    }
    for (const NodeId node_id : run.visited_nodes) {
        const Node* node = map.find_node(node_id);
        const auto node_mask = bit(node_id);
        if (node != nullptr && node_mask.has_value() && node->traversal.blocks_walk &&
            (initial.completed_nodes & *node_mask) == 0) {
            initial.opened_blockers |= *node_mask;
        }
    }
    for (const NodeId node_id : run.consumed_one_time_nodes) {
        const Node* node = map.find_node(node_id);
        const auto node_mask = bit(node_id);
        if (node != nullptr && node_mask.has_value() && node->type == NodeType::Light) {
            initial.consumed_lights |= *node_mask;
        }
    }

    const auto initial_id = intern(std::move(initial), error);
    if (!initial_id.has_value()) {
        return false;
    }
    m_initial_state = *initial_id;
    return true;
}

std::optional<SafetyStateId> OnDemandStateGraph::intern(PlannerState state, std::string* error)
{
    state.terminal = state_is_terminal(state);
    const auto found = m_ids.find(state);
    if (found != m_ids.end()) {
        return found->second;
    }
    if (m_states.size() >= m_options.maximum_states) {
        if (error != nullptr) {
            *error = "on-demand state graph exceeded the configured finite-state limit";
        }
        return std::nullopt;
    }
    const SafetyStateId id = static_cast<SafetyStateId>(m_states.size());
    m_states.emplace_back(std::move(state));
    m_ids.emplace(m_states.back(), id);
    m_actions.emplace_back(std::nullopt);
    return id;
}

RunState OnDemandStateGraph::materialize(const PlannerState& state) const
{
    return materialize_run_state(*m_run, state, m_indexed_nodes, 1);
}

bool OnDemandStateGraph::state_is_endpoint(const PlannerState& state) const noexcept
{
    if (m_options.strategy_terminal_nodes.contains(state.node)) {
        return true;
    }
    const Node* node = m_map->find_node(state.node);
    return node != nullptr && m_options.final_is_terminal &&
           (node->type == NodeType::Final || node->type == NodeType::BattleBoss);
}

bool OnDemandStateGraph::state_is_terminal(const PlannerState& state) const noexcept
{
    const bool endpoint_legal = state_is_endpoint(state);
    if (!endpoint_legal) {
        return false;
    }
    return m_options.safety_goal == nullptr ||
           m_options.safety_goal->is_floor_terminal_legal(state.goal_progress_id, m_run->floor, endpoint_legal);
}

std::optional<PlannerNodeMask> OnDemandStateGraph::bit(NodeId node) const noexcept
{
    return node_bit(m_node_indices, node);
}

bool OnDemandStateGraph::is_terminal(SafetyStateId id) const noexcept
{
    return id < m_states.size() && m_states[id].terminal;
}

bool OnDemandStateGraph::is_terminal_node(NodeId node_id) const noexcept
{
    if (m_options.strategy_terminal_nodes.contains(node_id)) {
        return true;
    }
    const Node* node = m_map == nullptr ? nullptr : m_map->find_node(node_id);
    return node != nullptr && m_options.final_is_terminal &&
           (node->type == NodeType::Final || node->type == NodeType::BattleBoss);
}

bool OnDemandStateGraph::is_completed(SafetyStateId id, NodeId node) const noexcept
{
    if (id >= m_states.size()) {
        return false;
    }
    const auto node_mask = bit(node);
    return node_mask.has_value() && (m_states[id].completed_nodes & *node_mask) != 0;
}

bool OnDemandStateGraph::is_light_consumed(SafetyStateId id, NodeId node) const noexcept
{
    if (id >= m_states.size()) {
        return false;
    }
    const auto node_mask = bit(node);
    return node_mask.has_value() && (m_states[id].consumed_lights & *node_mask) != 0;
}

const std::vector<OnDemandSafetyAction>* OnDemandStateGraph::actions(SafetyStateId id, std::string* error)
{
    if (id >= m_states.size()) {
        if (error != nullptr) {
            *error = "on-demand action request references an unknown state";
        }
        return nullptr;
    }
    if (m_actions[id].has_value()) {
        return &*m_actions[id];
    }

    std::vector<OnDemandSafetyAction> generated;
    const PlannerState source = m_states[id];
    if (!state_is_endpoint(source)) {
        if (m_compact != nullptr) {
            const auto compact_actions = m_compact->actions(source, error);
            if (!compact_actions.has_value()) {
                return nullptr;
            }
            for (const CompactMoveAction& compact_action : *compact_actions) {
                OnDemandSafetyAction action;
                action.candidate = compact_action.candidate;
                action.action_point_cost = compact_action.action_point_cost;
                action.minimum_action_points_to_start = compact_action.minimum_action_points_to_start;
                for (const CompactActionOutcome& compact_outcome : compact_action.outcomes) {
                    PlannerState successor = compact_outcome.successor;
                    const NodeId entered =
                        action.candidate.target == InvalidNodeId ? successor.node : action.candidate.target;
                    if (!advance_goal_progress(
                            *m_map,
                            *m_run,
                            source,
                            entered,
                            m_options,
                            m_node_indices,
                            successor,
                            error)) {
                        return nullptr;
                    }
                    const auto successor_id = intern(std::move(successor), error);
                    if (!successor_id.has_value()) {
                        return nullptr;
                    }
                    action.outcomes.emplace_back(
                        OnDemandSafetyOutcome {
                            *successor_id,
                            compact_outcome.action_point_gain,
                        });
                }
                std::ranges::sort(action.outcomes, {}, &OnDemandSafetyOutcome::successor);
                action.outcomes.erase(
                    std::unique(
                        action.outcomes.begin(),
                        action.outcomes.end(),
                        [](const OnDemandSafetyOutcome& lhs, const OnDemandSafetyOutcome& rhs) {
                            return lhs.successor == rhs.successor && lhs.action_point_gain == rhs.action_point_gain;
                        }),
                    action.outcomes.end());
                if (!action.outcomes.empty()) {
                    generated.emplace_back(std::move(action));
                }
            }
        }
        else {
            const RunState run = materialize(source);
            const auto moves = enumerate_move_actions(*m_map, run, m_options.graph_layer);
            for (const MoveAction& move : moves) {
                if (m_options.forbidden_action_ids.contains(move.candidate.action_id)) {
                    continue;
                }
                OnDemandSafetyAction action;
                action.candidate = move.candidate;
                action.action_point_cost = move.candidate.predicted_action_point_cost;
                for (const NodeId landing : move.possible_landings) {
                    const NodeId target = move.candidate.target == InvalidNodeId ? landing : move.candidate.target;
                    if (unavailable_target(*m_map, source, m_node_indices, target)) {
                        continue;
                    }
                    const int gain = action_gain(*m_map, source, move, landing, m_node_indices);
                    PlannerState successor = transition_state(*m_map, source, move, landing, m_node_indices);
                    if (!advance_goal_progress(
                            *m_map,
                            *m_run,
                            source,
                            target,
                            m_options,
                            m_node_indices,
                            successor,
                            error)) {
                        return nullptr;
                    }
                    const auto successor_id = intern(std::move(successor), error);
                    if (!successor_id.has_value()) {
                        return nullptr;
                    }
                    action.outcomes.emplace_back(OnDemandSafetyOutcome { *successor_id, gain });
                }
                std::ranges::sort(action.outcomes, {}, &OnDemandSafetyOutcome::successor);
                action.outcomes.erase(
                    std::unique(
                        action.outcomes.begin(),
                        action.outcomes.end(),
                        [](const OnDemandSafetyOutcome& lhs, const OnDemandSafetyOutcome& rhs) {
                            return lhs.successor == rhs.successor && lhs.action_point_gain == rhs.action_point_gain;
                        }),
                    action.outcomes.end());
                if (!action.outcomes.empty()) {
                    generated.emplace_back(std::move(action));
                }
            }
        }
        std::ranges::sort(generated, [](const OnDemandSafetyAction& lhs, const OnDemandSafetyAction& rhs) {
            return lhs.candidate.action_id < rhs.candidate.action_id;
        });
    }
    m_actions[id] = std::move(generated);
    return &*m_actions[id];
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
    return options.final_is_terminal && (node->type == NodeType::Final || node->type == NodeType::BattleBoss);
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
    for (const auto& [node_id, node] : map.nodes()) {
        if (node.floor == run.floor) {
            expanded.indexed_nodes.emplace_back(node_id);
        }
    }
    std::ranges::sort(expanded.indexed_nodes);
    if (expanded.indexed_nodes.size() > 64) {
        if (error != nullptr) {
            *error = "compact state expansion supports at most 64 nodes on one floor";
        }
        return std::nullopt;
    }
    expanded.node_indices.reserve(expanded.indexed_nodes.size());
    for (std::size_t index = 0; index < expanded.indexed_nodes.size(); ++index) {
        expanded.node_indices.emplace(expanded.indexed_nodes[index], static_cast<std::uint8_t>(index));
    }

    PlannerState initial;
    initial.node = run.current_node;
    if (options.safety_goal != nullptr) {
        initial.goal_progress_id = options.initial_goal_progress_id != InvalidSafetyGoalProgressId
                                       ? options.initial_goal_progress_id
                                       : options.safety_goal->initial_progress_id();
    }
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
    }
    for (const auto& [node, progress] : run.node_progress) {
        const auto bit = node_bit(expanded.node_indices, node);
        if (progress == NodeProgress::Completed && bit.has_value()) {
            initial.completed_nodes |= *bit;
        }
    }
    for (const NodeId node_id : run.visited_nodes) {
        const Node* node = map.find_node(node_id);
        const auto bit = node_bit(expanded.node_indices, node_id);
        if (node != nullptr && bit.has_value() && node->traversal.blocks_walk &&
            (initial.completed_nodes & *bit) == 0) {
            initial.opened_blockers |= *bit;
        }
    }
    for (const NodeId node_id : run.consumed_one_time_nodes) {
        const Node* node = map.find_node(node_id);
        const auto bit = node_bit(expanded.node_indices, node_id);
        if (node != nullptr && bit.has_value() && node->type == NodeType::Light) {
            initial.consumed_lights |= *bit;
        }
    }

    std::unordered_map<PlannerState, SafetyStateId, PlannerStateHash> ids;
    ids.reserve(std::min<std::size_t>(options.maximum_states, 262144));
    std::deque<SafetyStateId> pending;
    std::vector<bool> pending_queued;

    auto add_or_update_state = [&](PlannerState state, int reachable_action_points) -> std::optional<SafetyStateId> {
        state.terminal = is_terminal(map, state, options);
        const auto found = ids.find(state);
        if (found != ids.end()) {
            const SafetyStateId id = found->second;
            if (reachable_action_points > expanded.maximum_reachable_action_points[id]) {
                expanded.maximum_reachable_action_points[id] = reachable_action_points;
                if (!state.terminal && reachable_action_points >= 0 && !pending_queued[id]) {
                    pending.emplace_back(id);
                    pending_queued[id] = true;
                }
            }
            return id;
        }
        if (expanded.planner_states.size() >= options.maximum_states) {
            return std::nullopt;
        }
        const SafetyStateId id = static_cast<SafetyStateId>(expanded.planner_states.size());
        expanded.planner_states.emplace_back(std::move(state));
        ids.emplace(expanded.planner_states.back(), id);
        expanded.problem.states.emplace_back(
            SafetyState {
                id,
                expanded.planner_states.back().terminal,
                std::to_string(expanded.planner_states.back().node),
            });
        expanded.maximum_reachable_action_points.emplace_back(reachable_action_points);
        pending_queued.emplace_back(false);
        if (!expanded.planner_states.back().terminal && reachable_action_points >= 0) {
            pending.emplace_back(id);
            pending_queued[id] = true;
        }
        return id;
    };

    const auto initial_id = add_or_update_state(std::move(initial), run.resources.action_points);
    if (!initial_id.has_value()) {
        if (error != nullptr) {
            *error = "failed to create initial planner state";
        }
        return std::nullopt;
    }
    expanded.initial_state = *initial_id;

    std::unordered_set<std::string> emitted_actions;
    while (!pending.empty()) {
        const SafetyStateId source_id = pending.front();
        pending.pop_front();
        pending_queued[source_id] = false;
        const PlannerState source = expanded.planner_states[source_id];
        const int source_action_points = expanded.maximum_reachable_action_points[source_id];
        if (source.terminal || source_action_points < 0) {
            continue;
        }

        const RunState materialized = materialize_run_state(run, source, expanded.indexed_nodes, source_action_points);
        const auto actions = enumerate_move_actions(map, materialized, options.graph_layer);
        for (const auto& move : actions) {
            if (options.forbidden_action_ids.contains(move.candidate.action_id)) {
                continue;
            }
            const int action_cost = move.candidate.predicted_action_point_cost;
            const bool feasible = source_action_points >= 1 && source_action_points >= action_cost;
            if (!feasible && source_id != expanded.initial_state) {
                continue;
            }

            std::vector<SafetyOutcome> outcomes;
            for (const NodeId landing : move.possible_landings) {
                const NodeId target = move.candidate.target == InvalidNodeId ? landing : move.candidate.target;
                if (unavailable_target(map, source, expanded.node_indices, target)) {
                    continue;
                }
                const int gain = action_gain(map, source, move, landing, expanded.node_indices);
                const int successor_action_points =
                    feasible ? action_points_after(source_action_points, action_cost, gain) : -1;
                PlannerState successor = transition_state(map, source, move, landing, expanded.node_indices);
                if (!advance_goal_progress(
                        map,
                        run,
                        source,
                        target,
                        options,
                        expanded.node_indices,
                        successor,
                        error)) {
                    return std::nullopt;
                }
                const auto successor_id = add_or_update_state(std::move(successor), successor_action_points);
                if (!successor_id.has_value()) {
                    if (error != nullptr) {
                        *error = "state expansion exceeded the configured finite-state limit";
                    }
                    return std::nullopt;
                }
                outcomes.emplace_back(SafetyOutcome { *successor_id, gain });
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
            if (emitted_actions.emplace(action_id).second) {
                expanded.problem.actions.emplace_back(
                    SafetyAction {
                        action_id,
                        source_id,
                        action_cost,
                        1,
                        std::move(outcomes),
                    });
                expanded.action_candidates.insert_or_assign(action_id, move.candidate);
            }
        }
    }
    return expanded;
}
} // namespace asst::blackflow
