#include "BlackFlowStateSpace.h"

#include "BlackFlowCompactStateSpace.h"

#include <algorithm>

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
    state.terminal = state_is_goal(state);
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

// 端点只回答「这里还有没有后继」，因此只认物理出口。策略终点虽然可以就地收工，但仍然走得开，
// 把它算成端点会让站在上面的状态一个动作都展开不出来。
bool OnDemandStateGraph::state_is_endpoint(const PlannerState& state) const noexcept
{
    const Node* node = m_map->find_node(state.node);
    return node != nullptr && m_options.final_is_terminal && is_exit_node_type(node->type);
}

// 成功状态是「站在合法收工点」与「锁定目标已满足」的合取。
//
// 少了合取，走到出口就算赢，为策略目标预留的行动力会被最近的出口顶掉；少了收工点这一项，
// 目标一旦不在图上就没有任何成功状态，整层被判成无解——投影当初正是为了填这个洞而加的。
bool OnDemandStateGraph::state_is_goal(const PlannerState& state) const noexcept
{
    const bool stop_point = state_is_endpoint(state) || m_options.strategy_terminal_nodes.contains(state.node);
    return stop_point &&
           (m_options.safety_goal == nullptr || m_options.safety_goal->binding_goals_satisfied(state.goal_progress_id));
}

std::optional<PlannerNodeMask> OnDemandStateGraph::bit(NodeId node) const noexcept
{
    return node_bit(m_node_indices, node);
}

bool OnDemandStateGraph::is_terminal(SafetyStateId id) const noexcept
{
    return id < m_states.size() && m_states[id].terminal;
}

// 行动力耗尽是否构成合法收工。它无法写进 state_is_goal：PlannerState 不带行动力，
// 而目标谓词只看状态，判不出「还剩几点」。
//
// 因此这里回答的是更粗的一问——本轮的安全层是否还有约束对象。锁定目标非空时安全层照常
// 保证走到目标；锁定目标为空时，走到哪里停都算收工，安全层没有可证的命题，N 恒为零。
// 调用方据此短路求解，并把「再也付不起任何一步」当作路线终点。
bool OnDemandStateGraph::exhaustion_terminates() const noexcept
{
    return m_options.no_AP_is_terminal &&
           (m_options.safety_goal == nullptr || !m_options.safety_goal->has_binding_goals());
}

// 路线搜索用它判断「走到这里路线是否就结束了」，因此只看端点，不看目标进度。
bool OnDemandStateGraph::is_terminal_node(NodeId node_id) const noexcept
{
    if (m_options.strategy_terminal_nodes.contains(node_id)) {
        return true;
    }
    const Node* node = m_map == nullptr ? nullptr : m_map->find_node(node_id);
    return node != nullptr && m_options.final_is_terminal && is_exit_node_type(node->type);
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

} // namespace asst::blackflow
