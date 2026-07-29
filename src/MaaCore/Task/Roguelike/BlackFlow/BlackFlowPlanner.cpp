#include "BlackFlowPlanner.h"

#include "BlackFlowSafetyValue.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <future>
#include <iterator>
#include <limits>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace asst::blackflow
{
namespace
{
std::vector<std::string> sorted_types(const std::unordered_set<std::string>& types)
{
    std::vector<std::string> result(types.begin(), types.end());
    std::ranges::sort(result);
    return result;
}

struct ReachableFeatures
{
    std::unordered_set<std::string> node_types;
    bool has_badged = false;
    bool has_badged_incident = false;
};

// 羽瞰点无需前往就会揭示曼哈顿距离 2 以内的节点，前往之后揭示范围扩大到 3。
// 前两圈在观测里本来就是已揭示的，所以规划器只需要预测踩上去多开出来的第三圈。
constexpr int LightRevealRadius = 3;

bool revealed_by_consumed_light(
    const MapSnapshot& map,
    const OnDemandStateGraph& graph,
    SafetyStateId state,
    NodeId node)
{
    for (const auto& [light_id, light] : map.nodes()) {
        if (light.type != NodeType::Light || !graph.is_light_consumed(state, light_id)) {
            continue;
        }
        const auto revealed = map.nodes_within_manhattan(light_id, LightRevealRadius);
        if (std::ranges::find(revealed, node) != revealed.end()) {
            return true;
        }
    }
    return false;
}

int unknown_big_nodes_revealed(
    const MapSnapshot& map,
    const OnDemandStateGraph& graph,
    SafetyStateId state,
    NodeId light)
{
    const Node* light_node = map.find_node(light);
    if (light_node == nullptr || light_node->type != NodeType::Light) {
        return 0;
    }
    int count = 0;
    for (const NodeId id : map.nodes_within_manhattan(light, LightRevealRadius)) {
        const Node* candidate = map.find_node(id);
        if (candidate == nullptr || candidate->type == NodeType::Empty || candidate->identity_revealed ||
            graph.is_completed(state, id) || graph.source_run().revealed_nodes.contains(id) ||
            revealed_by_consumed_light(map, graph, state, id)) {
            continue;
        }
        ++count;
    }
    return count;
}

int intermediate_interaction_cost(NodeType type) noexcept
{
    return type == NodeType::Empty || type == NodeType::ScrapShop ? 0 : 1;
}

struct RouteMetric
{
    int battles = 0;
    int processing_moves = 0;
    int duration = 0;
};

RouteMetric add_metric(RouteMetric lhs, const RouteMetric& rhs) noexcept
{
    const auto add = [](int first, int second) {
        return static_cast<int>(
            std::min<std::int64_t>(static_cast<std::int64_t>(first) + second, std::numeric_limits<int>::max()));
    };
    lhs.battles = add(lhs.battles, rhs.battles);
    lhs.processing_moves = add(lhs.processing_moves, rhs.processing_moves);
    lhs.duration = add(lhs.duration, rhs.duration);
    return lhs;
}

std::int64_t route_penalty(const RouteMetric& metric) noexcept
{
    return static_cast<std::int64_t>(metric.battles) + metric.processing_moves + metric.duration;
}

bool route_metric_weakly_better(const RouteMetric& lhs, const RouteMetric& rhs) noexcept
{
    return route_penalty(lhs) <= route_penalty(rhs) && std::tie(lhs.battles, lhs.processing_moves, lhs.duration) <=
                                                           std::tie(rhs.battles, rhs.processing_moves, rhs.duration);
}

struct RouteMilestone
{
    const Milestone* definition = nullptr;
    const ResolvedPolicy* policy = nullptr;
    int initial_progress = 0;
    bool project_hidden_identity = false;
};

std::vector<RouteMilestone> route_milestones(
    const ResolvedPolicy& policy,
    const MissionState& mission,
    int floor,
    const FactStore& facts,
    const std::unordered_set<std::string>& unresolved_hidden_end_milestone_ids)
{
    std::vector<RouteMilestone> result;
    for (const Milestone& milestone : policy.milestones) {
        const MilestoneStatus status = mission.status(milestone.id);
        if (floor < milestone.floor_begin || floor > milestone.floor_end || status == MilestoneStatus::Satisfied ||
            status == MilestoneStatus::Missed || status == MilestoneStatus::Impossible ||
            !milestone.active_if.evaluate(facts) || milestone.selector.empty()) {
            continue;
        }
        result.emplace_back(
            RouteMilestone {
                &milestone,
                &policy,
                mission.progress(milestone.id),
                unresolved_hidden_end_milestone_ids.contains(milestone.id),
            });
    }
    std::ranges::sort(result, [](const RouteMilestone& lhs, const RouteMilestone& rhs) {
        if (lhs.definition->end != rhs.definition->end) {
            return lhs.definition->end;
        }
        return std::tie(lhs.definition->kind, lhs.definition->rank, lhs.definition->id) <
               std::tie(rhs.definition->kind, rhs.definition->rank, rhs.definition->id);
    });
    return result;
}

bool route_milestone_matches_node(const RouteMilestone& milestone, const Node& node) noexcept
{
    return milestone_matches_node(*milestone.definition, node) ||
           (milestone.project_hidden_identity && milestone.policy != nullptr &&
            hidden_node_may_reveal_milestone(*milestone.policy, *milestone.definition, node));
}

bool simulated_prerequisites_satisfied(
    const Milestone& milestone,
    const std::vector<RouteMilestone>& milestones,
    const std::vector<int>& progress,
    const MissionState& mission)
{
    return std::ranges::all_of(milestone.prerequisites, [&](const std::string& prerequisite) {
        if (mission.status(prerequisite) == MilestoneStatus::Satisfied) {
            return true;
        }
        for (std::size_t index = 0; index < milestones.size(); ++index) {
            if (milestones[index].definition->id == prerequisite) {
                return progress[index] >= milestones[index].definition->required_count;
            }
        }
        return false;
    });
}

using CountedNodes = std::vector<std::vector<NodeId>>;

std::vector<std::string> advance_milestones(
    const Node& node,
    const std::vector<RouteMilestone>& milestones,
    const MissionState& mission,
    std::vector<int>& progress,
    CountedNodes& counted,
    int unknown_nodes_revealed = 0)
{
    std::vector<std::string> advanced;
    const std::vector<int> before = progress;
    for (std::size_t index = 0; index < milestones.size(); ++index) {
        const RouteMilestone& route_milestone = milestones[index];
        const Milestone& milestone = *route_milestone.definition;
        const bool exact_match = milestone_matches_node(milestone, node);
        if (before[index] >= milestone.required_count ||
            (!exact_match && !route_milestone_matches_node(route_milestone, node)) ||
            milestone.minimum_unknown_nodes_revealed > unknown_nodes_revealed ||
            std::ranges::find(counted[index], node.id) != counted[index].end() ||
            !simulated_prerequisites_satisfied(milestone, milestones, before, mission)) {
            continue;
        }
        progress[index] = std::min(milestone.required_count, before[index] + 1);
        counted[index].emplace_back(node.id);
        std::ranges::sort(counted[index]);
        if (exact_match) {
            advanced.emplace_back(milestone.id);
        }
    }
    return advanced;
}

std::vector<int> end_progress_score(const std::vector<RouteMilestone>& milestones, const std::vector<int>& progress)
{
    std::vector<int> score;
    std::size_t begin = 0;
    while (begin < milestones.size() && milestones[begin].definition->end) {
        const int rank = milestones[begin].definition->rank;
        int completed = 0;
        int sum = 0;
        std::size_t end = begin;
        while (end < milestones.size() && milestones[end].definition->end && milestones[end].definition->rank == rank) {
            completed += progress[end] >= milestones[end].definition->required_count ? 1 : 0;
            sum += milestones[end].definition->weight *
                   std::min(progress[end], milestones[end].definition->required_count);
            ++end;
        }
        score.emplace_back(completed);
        score.emplace_back(sum);
        begin = end;
    }
    return score;
}

std::vector<std::int64_t>
    preferred_progress_score(const std::vector<RouteMilestone>& milestones, const std::vector<int>& progress)
{
    std::vector<std::int64_t> score;
    for (const MilestoneKind kind : { MilestoneKind::Preferred, MilestoneKind::Opportunistic }) {
        std::size_t begin = 0;
        while (begin < milestones.size()) {
            while (begin < milestones.size() && milestones[begin].definition->kind != kind) {
                ++begin;
            }
            if (begin == milestones.size()) {
                break;
            }
            const int rank = milestones[begin].definition->rank;
            std::int64_t reward = 0;
            std::size_t end = begin;
            while (end < milestones.size() && milestones[end].definition->kind == kind &&
                   milestones[end].definition->rank == rank) {
                const Milestone& milestone = *milestones[end].definition;
                const int gained = std::max(
                    0,
                    std::min(progress[end], milestone.required_count) -
                        std::min(milestones[end].initial_progress, milestone.required_count));
                reward += static_cast<std::int64_t>(milestone.weight) * gained;
                ++end;
            }
            score.emplace_back(reward);
            begin = end;
        }
    }
    return score;
}

bool score_greater(const std::vector<int>& lhs, const std::vector<int>& rhs)
{
    return std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

struct RouteLabel
{
    SafetyStateId state = 0;
    int action_points = 0;
    std::vector<int> progress;
    CountedNodes counted;
    RouteMetric metric;
    int intermediate_interactions = 0;
    std::vector<std::string> immediate_milestone_ids;
    std::vector<NodeId> route;
    std::vector<PlannedRouteStep> steps;
};

bool route_label_better(
    const RouteLabel& lhs,
    const RouteLabel& rhs,
    const std::vector<RouteMilestone>& milestones,
    bool minimize_intermediate_interactions = false)
{
    const auto lhs_end = end_progress_score(milestones, lhs.progress);
    const auto rhs_end = end_progress_score(milestones, rhs.progress);
    if (score_greater(lhs_end, rhs_end)) {
        return true;
    }
    if (score_greater(rhs_end, lhs_end)) {
        return false;
    }
    const auto lhs_preferred = preferred_progress_score(milestones, lhs.progress);
    const auto rhs_preferred = preferred_progress_score(milestones, rhs.progress);
    for (std::size_t index = 0; index < std::min(lhs_preferred.size(), rhs_preferred.size()); ++index) {
        if (lhs_preferred[index] == rhs_preferred[index]) {
            continue;
        }
        const std::int64_t lhs_utility = lhs_preferred[index] - route_penalty(lhs.metric) -
                                         (minimize_intermediate_interactions ? lhs.intermediate_interactions : 0);
        const std::int64_t rhs_utility = rhs_preferred[index] - route_penalty(rhs.metric) -
                                         (minimize_intermediate_interactions ? rhs.intermediate_interactions : 0);
        if (lhs_utility != rhs_utility) {
            return lhs_utility > rhs_utility;
        }
        return lhs_preferred[index] > rhs_preferred[index];
    }
    if (minimize_intermediate_interactions) {
        const auto lhs_metric = std::tie(
            lhs.metric.battles,
            lhs.intermediate_interactions,
            lhs.metric.processing_moves,
            lhs.metric.duration);
        const auto rhs_metric = std::tie(
            rhs.metric.battles,
            rhs.intermediate_interactions,
            rhs.metric.processing_moves,
            rhs.metric.duration);
        if (lhs_metric != rhs_metric) {
            return lhs_metric < rhs_metric;
        }
    }
    else if (
        std::tie(lhs.metric.battles, lhs.metric.processing_moves, lhs.metric.duration) !=
        std::tie(rhs.metric.battles, rhs.metric.processing_moves, rhs.metric.duration)) {
        return std::tie(lhs.metric.battles, lhs.metric.processing_moves, lhs.metric.duration) <
               std::tie(rhs.metric.battles, rhs.metric.processing_moves, rhs.metric.duration);
    }
    return lhs.action_points > rhs.action_points;
}

bool boolean_fact(const FactStore& facts, std::string_view name)
{
    const FactValue* value = facts.find(name);
    return value != nullptr && std::holds_alternative<bool>(*value) && std::get<bool>(*value);
}

struct BudgetStateKey
{
    SafetyStateId state = 0;
    int action_points = 0;

    bool operator==(const BudgetStateKey&) const noexcept = default;
};

struct BudgetStateKeyHash
{
    std::size_t operator()(const BudgetStateKey& key) const noexcept
    {
        const std::size_t first = std::hash<SafetyStateId> {}(key.state);
        const std::size_t second = std::hash<int> {}(key.action_points);
        return first ^ (second + 0x9e3779b97f4a7c15ULL + (first << 6U) + (first >> 2U));
    }
};

class OnDemandSafetyOracle
{
public:
    explicit OnDemandSafetyOracle(
        OnDemandStateGraph& graph,
        std::string instance_name,
        bool enable_resource_dominance = true) :
        m_graph(graph),
        m_solver([&graph, instance_name = std::move(instance_name), enable_resource_dominance]() mutable {
            auto problem = make_on_demand_safety_value_problem(graph, std::move(instance_name));
            if (!enable_resource_dominance) {
                problem.dominance_descriptor = {};
            }
            return problem;
        }())
    {
    }

    bool certifies(SafetyStateId state, int action_points)
    {
        const int required = exact_requirement(state, action_points);
        return required < UnreachableActionPointRequirement && action_points >= required;
    }

    bool action_certifies(const OnDemandSafetyAction& action, int action_points, std::size_t* proof_depth = nullptr)
    {
        const int required = exact_action_requirement(action, action_points);
        if (required >= UnreachableActionPointRequirement || action_points < required) {
            return false;
        }
        std::size_t depth = 0;
        for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
            const int remaining =
                action_points_after(action_points, action.action_point_cost, outcome.action_point_gain);
            const auto successor_proof = proof(outcome.successor, remaining);
            if (!successor_proof.has_value()) {
                return false;
            }
            depth = std::max(depth, successor_proof->depth + 1);
        }
        if (proof_depth != nullptr) {
            *proof_depth = depth;
        }
        return true;
    }

    int requirement(SafetyStateId state, int maximum_action_points)
    {
        return exact_requirement(state, maximum_action_points);
    }

    int action_requirement(const OnDemandSafetyAction& action, int maximum_action_points)
    {
        return exact_action_requirement(action, maximum_action_points);
    }

    std::optional<std::size_t> cached_depth(SafetyStateId state, int action_points)
    {
        std::string error;
        const auto depth = m_solver.bounded_proof_depth(state, action_points, &error);
        if (!error.empty()) {
            m_error = std::move(error);
        }
        return depth;
    }

    std::optional<std::string> lexicographic_first_action(SafetyStateId state, int action_points)
    {
        std::string error;
        const auto action = m_solver.bounded_witness(state, action_points, &error);
        if (!error.empty()) {
            m_error = std::move(error);
        }
        return action;
    }

    std::optional<std::string> first_action(SafetyStateId state, int action_points)
    {
        return lexicographic_first_action(state, action_points);
    }

    const std::string& error() const noexcept { return m_error; }

private:
    struct Proof
    {
        std::size_t depth = 0;
        std::string action_id;
    };

    int exact_requirement(SafetyStateId state, int maximum_action_points)
    {
        std::string error;
        const int required = m_solver.N_bounded(state, maximum_action_points, &error);
        if (!error.empty()) {
            m_error = std::move(error);
        }
        return required;
    }

    int exact_action_requirement(const OnDemandSafetyAction& action, int maximum_action_points)
    {
        const int first_budget = std::max(action.minimum_action_points_to_start, action.action_point_cost);
        for (int action_points = first_budget; action_points <= maximum_action_points; ++action_points) {
            bool safe = true;
            for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
                const int remaining =
                    action_points_after(action_points, action.action_point_cost, outcome.action_point_gain);
                const int successor = exact_requirement(outcome.successor, remaining);
                if (successor >= UnreachableActionPointRequirement || successor > remaining) {
                    safe = false;
                    break;
                }
            }
            if (safe) {
                return action_points;
            }
        }
        return UnreachableActionPointRequirement;
    }

    std::optional<Proof> proof(SafetyStateId state, int action_points)
    {
        if (m_graph.is_terminal(state)) {
            return Proof {};
        }
        const BudgetStateKey key { state, action_points };
        if (const auto found = m_proofs.find(key); found != m_proofs.end()) {
            return found->second;
        }
        std::unordered_set<BudgetStateKey, BudgetStateKeyHash> visiting;
        return build_proof(state, action_points, visiting);
    }

    std::optional<Proof> build_proof(
        SafetyStateId state,
        int action_points,
        std::unordered_set<BudgetStateKey, BudgetStateKeyHash>& visiting)
    {
        if (m_graph.is_terminal(state)) {
            return Proof {};
        }
        const BudgetStateKey key { state, action_points };
        if (const auto found = m_proofs.find(key); found != m_proofs.end()) {
            return found->second;
        }
        if (!visiting.emplace(key).second) {
            return std::nullopt;
        }
        const int state_requirement = exact_requirement(state, action_points);
        const auto* actions = m_graph.actions(state, &m_error);
        if (actions == nullptr) {
            visiting.erase(key);
            return std::nullopt;
        }
        const auto attempt = [&](const OnDemandSafetyAction& action) -> std::optional<Proof> {
            if (exact_action_requirement(action, action_points) != state_requirement) {
                return std::nullopt;
            }
            std::size_t depth = 0;
            for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
                const int remaining =
                    action_points_after(action_points, action.action_point_cost, outcome.action_point_gain);
                const auto successor = build_proof(outcome.successor, remaining, visiting);
                if (!successor.has_value()) {
                    return std::nullopt;
                }
                depth = std::max(depth, successor->depth + 1);
            }
            return Proof { depth, action.candidate.action_id };
        };
        std::optional<Proof> selected;
        for (const OnDemandSafetyAction& action : *actions) {
            const bool direct_terminal =
                std::ranges::all_of(action.outcomes, [&](const OnDemandSafetyOutcome& outcome) {
                    return m_graph.is_terminal(outcome.successor);
                });
            if (direct_terminal && (selected = attempt(action)).has_value()) {
                break;
            }
        }
        if (!selected.has_value()) {
            for (const OnDemandSafetyAction& action : *actions) {
                const bool direct_terminal =
                    std::ranges::all_of(action.outcomes, [&](const OnDemandSafetyOutcome& outcome) {
                        return m_graph.is_terminal(outcome.successor);
                    });
                if (!direct_terminal && (selected = attempt(action)).has_value()) {
                    break;
                }
            }
        }
        visiting.erase(key);
        if (selected.has_value()) {
            m_proofs.insert_or_assign(key, *selected);
        }
        return selected;
    }

    OnDemandStateGraph& m_graph;
    SafetyValueSolver m_solver;
    std::unordered_map<BudgetStateKey, Proof, BudgetStateKeyHash> m_proofs;
    std::string m_error;
};

NodeType route_node_type(const MapSnapshot& map, const OnDemandStateGraph& graph, SafetyStateId source, NodeId node)
{
    const Node* target = map.find_node(node);
    if (target == nullptr) {
        return NodeType::Unknown;
    }
    if (!target->traversal.repeatable && graph.is_completed(source, node)) {
        return NodeType::Empty;
    }
    return target->type;
}

RouteMetric move_metric(
    const MapSnapshot& map,
    const OnDemandStateGraph& graph,
    SafetyStateId source,
    const MoveCandidate& move)
{
    const bool battle = is_combat_node_type(route_node_type(map, graph, source, move.target));
    return {
        battle ? 1 : 0,
        move.movement == MovementKind::Walk ? 0 : 1,
        move.movement == MovementKind::Walk ? std::max(1, static_cast<int>(move.path.size())) : 1,
    };
}

ReachableFeatures reachable_features(
    const MapSnapshot& map,
    OnDemandStateGraph& graph,
    OnDemandSafetyOracle& oracle,
    SafetyStateId initial,
    int initial_action_points,
    std::string* error)
{
    std::unordered_set<std::string> possible_types;
    bool possible_badged = false;
    bool possible_badged_incident = false;
    for (const auto& [id, node] : map.nodes()) {
        (void)id;
        if (node.progress == NodeProgress::Removed || node.type == NodeType::Empty) {
            continue;
        }
        possible_types.emplace(to_string(node.type));
        if (node.type == NodeType::BattleBoss) {
            possible_types.emplace("final");
        }
        possible_badged = possible_badged || node.badged;
        possible_badged_incident = possible_badged_incident || (node.badged && node.type == NodeType::Incident);
    }

    std::unordered_map<SafetyStateId, int> best_action_points;
    std::deque<std::pair<SafetyStateId, int>> pending;
    pending.emplace_back(initial, initial_action_points);
    best_action_points.emplace(initial, initial_action_points);
    ReachableFeatures result;
    while (!pending.empty()) {
        const auto [state_id, action_points] = pending.front();
        pending.pop_front();
        const Node* node = map.find_node(graph.state(state_id).node);
        if (node != nullptr && node->type != NodeType::Empty) {
            result.node_types.emplace(to_string(node->type));
            result.has_badged = result.has_badged || node->badged;
            result.has_badged_incident =
                result.has_badged_incident || (node->badged && node->type == NodeType::Incident);
        }
        if (graph.is_terminal(state_id)) {
            result.node_types.emplace("final");
        }
        if (result.node_types.size() == possible_types.size() && (!possible_badged || result.has_badged) &&
            (!possible_badged_incident || result.has_badged_incident)) {
            break;
        }

        const auto* actions = graph.actions(state_id, error);
        if (actions == nullptr) {
            return result;
        }
        for (const OnDemandSafetyAction& action : *actions) {
            if (!oracle.action_certifies(action, action_points)) {
                continue;
            }
            for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
                const int remaining =
                    action_points_after(action_points, action.action_point_cost, outcome.action_point_gain);
                const auto found = best_action_points.find(outcome.successor);
                if (found == best_action_points.end() || remaining > found->second) {
                    best_action_points.insert_or_assign(outcome.successor, remaining);
                    pending.emplace_back(outcome.successor, remaining);
                }
            }
        }
    }
    return result;
}

int maximum_future_entries(
    const MapSnapshot& map,
    const OnDemandStateGraph& graph,
    SafetyStateId state_id,
    int action_points)
{
    const PlannerState& state = graph.state(state_id);
    int remaining_nodes = 0;
    int possible_light_gains = 0;
    for (const auto& [id, node] : map.nodes()) {
        if (node.progress == NodeProgress::Removed || route_node_type(map, graph, state_id, id) == NodeType::Empty) {
            continue;
        }
        ++remaining_nodes;
        if (node.type == NodeType::Light) {
            ++possible_light_gains;
        }
    }
    if (remaining_nodes == 0) {
        return 0;
    }

    const DynamicCostModel& costs = graph.source_run().costs;
    if (costs.walk_cost_per_edge == 0 ||
        std::ranges::any_of(costs.action_cost_overrides, [](const auto& value) { return value.second == 0; })) {
        return remaining_nodes;
    }

    std::int64_t entries = std::max(action_points, 0) + possible_light_gains;
    for (const MovementSpec& movement : movement_specs()) {
        if (movement.kind == MovementKind::Walk) {
            continue;
        }
        const int charges = state.movement_charges[static_cast<std::size_t>(movement.kind)];
        if (charges <= 0) {
            continue;
        }
        const int cost = costs.movement_cost(movement);
        if (cost == 0) {
            entries += charges;
        }
        entries += static_cast<std::int64_t>(charges) * std::max(movement.effect.action_point_gain, 0);
    }
    return static_cast<int>(std::min<std::int64_t>(remaining_nodes, entries));
}

bool route_may_beat(
    const MapSnapshot& map,
    const OnDemandStateGraph& graph,
    const std::vector<RouteMilestone>& milestones,
    const RouteLabel& current,
    const RouteLabel& incumbent,
    bool minimize_intermediate_interactions)
{
    std::vector<int> upper_progress = current.progress;
    const int entry_limit = maximum_future_entries(map, graph, current.state, current.action_points);
    const int nonterminal_entry_limit = std::max(entry_limit - 1, 0);
    for (std::size_t index = 0; index < milestones.size(); ++index) {
        const Milestone& milestone = *milestones[index].definition;
        int matching_nonterminals = 0;
        bool matching_exact_terminal = false;
        for (const auto& [id, stored] : map.nodes()) {
            if (stored.progress == NodeProgress::Removed ||
                std::ranges::find(current.counted[index], id) != current.counted[index].end()) {
                continue;
            }
            Node node = stored;
            node.type = route_node_type(map, graph, current.state, id);
            if (node.type == NodeType::Empty) {
                node.name.clear();
            }
            const bool exact_match = milestone_matches_node(milestone, node);
            if (!exact_match && !route_milestone_matches_node(milestones[index], node)) {
                continue;
            }
            if (graph.is_terminal_node(id)) {
                matching_exact_terminal = matching_exact_terminal || exact_match;
            }
            else {
                ++matching_nonterminals;
            }
        }
        const int possible_increment = std::min(matching_nonterminals, nonterminal_entry_limit) +
                                       (entry_limit > 0 && matching_exact_terminal ? 1 : 0);
        upper_progress[index] = std::min(milestone.required_count, current.progress[index] + possible_increment);
    }

    const auto upper_end = end_progress_score(milestones, upper_progress);
    const auto incumbent_end = end_progress_score(milestones, incumbent.progress);
    if (score_greater(upper_end, incumbent_end)) {
        return true;
    }
    if (score_greater(incumbent_end, upper_end)) {
        return false;
    }

    RouteMetric minimum_metric = current.metric;
    if (!graph.is_terminal(current.state)) {
        minimum_metric.duration = add_metric(minimum_metric, RouteMetric { 0, 0, 1 }).duration;
        bool has_terminal = false;
        bool every_terminal_is_combat = true;
        for (const auto& [id, node] : map.nodes()) {
            if (node.progress == NodeProgress::Removed || !graph.is_terminal_node(id)) {
                continue;
            }
            has_terminal = true;
            every_terminal_is_combat = every_terminal_is_combat && is_combat_node_type(node.type);
        }
        if (has_terminal && every_terminal_is_combat) {
            minimum_metric.battles = add_metric(minimum_metric, RouteMetric { 1, 0, 0 }).battles;
        }
    }

    const auto lower_preferred = preferred_progress_score(milestones, current.progress);
    std::vector<std::int64_t> bounded_upper_preferred = preferred_progress_score(milestones, upper_progress);
    const auto incumbent_preferred = preferred_progress_score(milestones, incumbent.progress);
    const std::int64_t current_penalty =
        route_penalty(current.metric) + (minimize_intermediate_interactions ? current.intermediate_interactions : 0);
    const std::int64_t minimum_penalty =
        route_penalty(minimum_metric) + (minimize_intermediate_interactions ? current.intermediate_interactions : 0);
    const std::int64_t incumbent_penalty =
        route_penalty(incumbent.metric) +
        (minimize_intermediate_interactions ? incumbent.intermediate_interactions : 0);
    std::vector<std::int64_t> upper_utilities;
    upper_utilities.reserve(bounded_upper_preferred.size());
    std::size_t score_index = 0;
    for (const MilestoneKind kind : { MilestoneKind::Preferred, MilestoneKind::Opportunistic }) {
        std::size_t begin = 0;
        while (begin < milestones.size()) {
            while (begin < milestones.size() && milestones[begin].definition->kind != kind) {
                ++begin;
            }
            if (begin == milestones.size()) {
                break;
            }
            const int rank = milestones[begin].definition->rank;
            std::size_t end = begin;
            while (end < milestones.size() && milestones[end].definition->kind == kind &&
                   milestones[end].definition->rank == rank) {
                ++end;
            }

            int best_terminal_reward = 0;
            int best_terminal_net = std::numeric_limits<int>::min();
            bool has_terminal = false;
            std::vector<int> nonterminal_rewards;
            std::vector<int> nonterminal_nets;
            for (const auto& [id, stored] : map.nodes()) {
                if (stored.progress == NodeProgress::Removed) {
                    continue;
                }
                Node node = stored;
                node.type = route_node_type(map, graph, current.state, id);
                if (node.type == NodeType::Empty) {
                    node.name.clear();
                }
                int reward = 0;
                for (std::size_t milestone_index = begin; milestone_index < end; ++milestone_index) {
                    const Milestone& milestone = *milestones[milestone_index].definition;
                    if (current.progress[milestone_index] >= milestone.required_count ||
                        std::ranges::find(current.counted[milestone_index], id) !=
                            current.counted[milestone_index].end() ||
                        !route_milestone_matches_node(milestones[milestone_index], node)) {
                        continue;
                    }
                    reward += milestone.weight;
                }
                const bool terminal = graph.is_terminal_node(id);
                const int interaction_penalty =
                    minimize_intermediate_interactions ? (terminal ? 0 : intermediate_interaction_cost(node.type)) : 1;
                const int net = reward - interaction_penalty - (is_combat_node_type(node.type) ? 1 : 0);
                if (terminal) {
                    has_terminal = true;
                    best_terminal_reward = std::max(best_terminal_reward, reward);
                    best_terminal_net = std::max(best_terminal_net, net);
                }
                else if (reward > 0) {
                    nonterminal_rewards.emplace_back(reward);
                    nonterminal_nets.emplace_back(net);
                }
            }
            if (!has_terminal || score_index >= bounded_upper_preferred.size()) {
                return true;
            }
            std::ranges::sort(nonterminal_rewards, std::greater {});
            std::ranges::sort(nonterminal_nets, std::greater {});
            const std::size_t optional_slots =
                std::min<std::size_t>(nonterminal_rewards.size(), static_cast<std::size_t>(nonterminal_entry_limit));
            std::int64_t additive_reward_upper = lower_preferred[score_index] + best_terminal_reward;
            for (std::size_t position = 0; position < optional_slots; ++position) {
                additive_reward_upper += nonterminal_rewards[position];
            }
            bounded_upper_preferred[score_index] =
                std::min(bounded_upper_preferred[score_index], additive_reward_upper);

            std::int64_t additive_utility_upper = lower_preferred[score_index] - current_penalty + best_terminal_net;
            const std::size_t net_slots =
                std::min<std::size_t>(nonterminal_nets.size(), static_cast<std::size_t>(nonterminal_entry_limit));
            for (std::size_t position = 0; position < net_slots && nonterminal_nets[position] > 0; ++position) {
                additive_utility_upper += nonterminal_nets[position];
            }
            upper_utilities.emplace_back(
                std::min(additive_utility_upper, bounded_upper_preferred[score_index] - minimum_penalty));
            ++score_index;
            begin = end;
        }
    }
    if (score_index != bounded_upper_preferred.size()) {
        return true;
    }

    for (std::size_t index = 0; index < bounded_upper_preferred.size(); ++index) {
        const std::int64_t lower_reward = lower_preferred[index];
        const std::int64_t upper_reward = bounded_upper_preferred[index];
        const std::int64_t incumbent_reward = incumbent_preferred[index];
        const std::int64_t upper_utility = upper_utilities[index];
        const std::int64_t incumbent_utility = incumbent_reward - incumbent_penalty;
        if (upper_utility > incumbent_utility ||
            (upper_utility == incumbent_utility && upper_reward > incumbent_reward)) {
            return true;
        }
        if (incumbent_reward < lower_reward || incumbent_reward > upper_reward) {
            return false;
        }
    }

    if (minimize_intermediate_interactions) {
        const auto minimum_tie = std::tie(
            minimum_metric.battles,
            current.intermediate_interactions,
            minimum_metric.processing_moves,
            minimum_metric.duration);
        const auto incumbent_tie = std::tie(
            incumbent.metric.battles,
            incumbent.intermediate_interactions,
            incumbent.metric.processing_moves,
            incumbent.metric.duration);
        if (minimum_tie != incumbent_tie) {
            return minimum_tie < incumbent_tie;
        }
    }
    else {
        const auto minimum_tie =
            std::tie(minimum_metric.battles, minimum_metric.processing_moves, minimum_metric.duration);
        const auto incumbent_tie =
            std::tie(incumbent.metric.battles, incumbent.metric.processing_moves, incumbent.metric.duration);
        if (minimum_tie != incumbent_tie) {
            return minimum_tie < incumbent_tie;
        }
    }
    return true;
}

struct RouteSearchBudget
{
    std::chrono::steady_clock::time_point deadline;
    std::size_t remaining_expansions = 0;
    std::size_t initial_expansions = 0;

    [[nodiscard]] bool exhausted() const noexcept
    {
        return remaining_expansions == 0 || std::chrono::steady_clock::now() >= deadline;
    }

    [[nodiscard]] std::size_t consumed_expansions() const noexcept { return initial_expansions - remaining_expansions; }

    [[nodiscard]] bool time_exhausted() const noexcept { return std::chrono::steady_clock::now() >= deadline; }

    bool consume() noexcept
    {
        if (exhausted()) {
            return false;
        }
        --remaining_expansions;
        return true;
    }
};

RouteLabel best_route_after_outcome(
    const MapSnapshot& map,
    OnDemandStateGraph& graph,
    OnDemandSafetyOracle& oracle,
    const std::vector<RouteMilestone>& milestones,
    const MissionState& mission,
    const MoveCandidate& root_move,
    const OnDemandSafetyAction& root_action,
    const OnDemandSafetyOutcome& root_outcome,
    int initial_action_points,
    int remaining_action_points,
    const RouteSearchOptions& search_options,
    RouteSearchBudget& search_budget,
    bool minimize_intermediate_interactions,
    std::string* error)
{
    RouteLabel initial;
    initial.state = root_outcome.successor;
    initial.action_points = remaining_action_points;
    initial.progress.reserve(milestones.size());
    initial.counted.resize(milestones.size());
    for (std::size_t index = 0; index < milestones.size(); ++index) {
        const RouteMilestone& milestone = milestones[index];
        initial.progress.emplace_back(milestone.initial_progress);
        const auto counted = mission.milestone_nodes.find(milestone.definition->id);
        if (counted != mission.milestone_nodes.end()) {
            initial.counted[index].assign(counted->second.begin(), counted->second.end());
            std::ranges::sort(initial.counted[index]);
        }
    }
    initial.metric = move_metric(map, graph, graph.initial_state(), root_move);
    initial.steps.emplace_back(
        PlannedRouteStep {
            root_move,
            initial_action_points,
            root_action.action_point_cost,
            root_outcome.action_point_gain,
            remaining_action_points,
        });
    NodeId entered_node = root_move.target;
    if (entered_node == InvalidNodeId) {
        entered_node = graph.state(root_outcome.successor).node;
    }
    if (minimize_intermediate_interactions && !graph.is_terminal(root_outcome.successor)) {
        initial.intermediate_interactions =
            intermediate_interaction_cost(route_node_type(map, graph, graph.initial_state(), entered_node));
    }
    if (const Node* target = map.find_node(entered_node); target != nullptr) {
        Node entered = *target;
        entered.type = route_node_type(map, graph, graph.initial_state(), target->id);
        if (entered.type == NodeType::Empty) {
            entered.name.clear();
        }
        initial.immediate_milestone_ids = advance_milestones(
            entered,
            milestones,
            mission,
            initial.progress,
            initial.counted,
            unknown_big_nodes_revealed(map, graph, graph.initial_state(), entered.id));
        initial.route.emplace_back(target->id);
    }

    std::unordered_map<SafetyStateId, std::vector<RouteLabel>> labels;
    labels[initial.state].emplace_back(initial);

    struct PendingRoute
    {
        RouteLabel route;
        std::size_t terminal_depth = std::numeric_limits<std::size_t>::max();
    };

    const auto make_pending = [&](RouteLabel route) {
        return PendingRoute {
            route,
            oracle.cached_depth(route.state, route.action_points).value_or(std::numeric_limits<std::size_t>::max()),
        };
    };
    const auto lower_priority = [&](const PendingRoute& lhs, const PendingRoute& rhs) {
        if (route_label_better(lhs.route, rhs.route, milestones, minimize_intermediate_interactions)) {
            return false;
        }
        if (route_label_better(rhs.route, lhs.route, milestones, minimize_intermediate_interactions)) {
            return true;
        }
        return lhs.terminal_depth > rhs.terminal_depth;
    };
    std::priority_queue<PendingRoute, std::vector<PendingRoute>, decltype(lower_priority)> pending(lower_priority);
    pending.emplace(make_pending(initial));
    std::optional<RouteLabel> best;
    const std::size_t route_expansion_budget_per_root = search_options.expansions_per_root;
    std::size_t expanded_routes = 0;

    const auto build_next = [&](const RouteLabel& current,
                                const OnDemandSafetyAction& action,
                                bool verify_safety = true) -> std::optional<RouteLabel> {
        if (action.outcomes.size() != 1) {
            return std::nullopt;
        }
        const int requirement = verify_safety
                                    ? oracle.action_requirement(action, current.action_points)
                                    : std::max(action.minimum_action_points_to_start, action.action_point_cost);
        if (requirement >= UnreachableActionPointRequirement || current.action_points < requirement) {
            return std::nullopt;
        }
        const OnDemandSafetyOutcome& outcome = action.outcomes.front();
        const int remaining =
            action_points_after(current.action_points, action.action_point_cost, outcome.action_point_gain);
        RouteLabel next = current;
        next.state = outcome.successor;
        next.action_points = remaining;
        next.metric = add_metric(next.metric, move_metric(map, graph, current.state, action.candidate));
        if (minimize_intermediate_interactions && !graph.is_terminal(outcome.successor)) {
            next.intermediate_interactions +=
                intermediate_interaction_cost(route_node_type(map, graph, current.state, action.candidate.target));
        }
        MoveCandidate planned_move = action.candidate;
        planned_move.action_point_requirement = requirement;
        next.steps.emplace_back(
            PlannedRouteStep {
                std::move(planned_move),
                current.action_points,
                action.action_point_cost,
                outcome.action_point_gain,
                remaining,
            });
        NodeId target_id = action.candidate.target;
        if (target_id == InvalidNodeId) {
            target_id = graph.state(outcome.successor).node;
        }
        if (const Node* target = map.find_node(target_id); target != nullptr) {
            Node entered = *target;
            entered.type = route_node_type(map, graph, current.state, target->id);
            if (entered.type == NodeType::Empty) {
                entered.name.clear();
            }
            (void)advance_milestones(
                entered,
                milestones,
                mission,
                next.progress,
                next.counted,
                unknown_big_nodes_revealed(map, graph, current.state, entered.id));
            next.route.emplace_back(target->id);
        }
        return next;
    };

    const auto flexibility_score = [&](const RouteLabel& route) {
        const PlannerState& state = graph.state(route.state);
        int score = 0;
        for (const MovementSpec& movement : movement_specs()) {
            if (movement.kind == MovementKind::Walk) {
                continue;
            }
            int range_weight = 1;
            switch (movement.range) {
            case MovementRange::WalkEdges:
                range_weight = 1;
                break;
            case MovementRange::SurroundingEight:
                range_weight = 4;
                break;
            case MovementRange::OrthogonalTwo:
            case MovementRange::ManhattanTwo:
                range_weight = 3;
                break;
            case MovementRange::OrthogonalThree:
                range_weight = 4;
                break;
            case MovementRange::FullMap:
                range_weight = 8;
                break;
            }
            const int charges = state.movement_charges[static_cast<std::size_t>(movement.kind)];
            score += range_weight * charges;
            if (charges > 0) {
                score += 4;
            }
        }
        return score;
    };
    const auto heuristic_better = [&](const RouteLabel& lhs, const RouteLabel& rhs) {
        const auto lhs_end = end_progress_score(milestones, lhs.progress);
        const auto rhs_end = end_progress_score(milestones, rhs.progress);
        if (score_greater(lhs_end, rhs_end)) {
            return true;
        }
        if (score_greater(rhs_end, lhs_end)) {
            return false;
        }
        const auto lhs_preferred = preferred_progress_score(milestones, lhs.progress);
        const auto rhs_preferred = preferred_progress_score(milestones, rhs.progress);
        for (std::size_t index = 0; index < std::min(lhs_preferred.size(), rhs_preferred.size()); ++index) {
            const std::int64_t lhs_utility = lhs_preferred[index] - route_penalty(lhs.metric) -
                                             (minimize_intermediate_interactions ? lhs.intermediate_interactions : 0);
            const std::int64_t rhs_utility = rhs_preferred[index] - route_penalty(rhs.metric) -
                                             (minimize_intermediate_interactions ? rhs.intermediate_interactions : 0);
            if (lhs_utility != rhs_utility) {
                return lhs_utility > rhs_utility;
            }
            if (lhs_preferred[index] != rhs_preferred[index]) {
                return lhs_preferred[index] > rhs_preferred[index];
            }
        }
        const int lhs_flexibility = flexibility_score(lhs);
        const int rhs_flexibility = flexibility_score(rhs);
        if (lhs_flexibility != rhs_flexibility) {
            return lhs_flexibility > rhs_flexibility;
        }
        return route_label_better(lhs, rhs, milestones, minimize_intermediate_interactions);
    };
    const auto complete_greedy = [&](RouteLabel greedy) -> std::optional<RouteLabel> {
        struct PreviewedAction
        {
            const OnDemandSafetyAction* action = nullptr;
            RouteLabel preview;
        };
        struct PreviewedPair
        {
            const OnDemandSafetyAction* first = nullptr;
            const OnDemandSafetyAction* second = nullptr;
            RouteLabel preview;
        };
        struct PreviewedSequence
        {
            std::vector<const OnDemandSafetyAction*> actions;
            RouteLabel preview;
        };

        std::unordered_set<BudgetStateKey, BudgetStateKeyHash> greedy_seen;
        while (!graph.is_terminal(greedy.state) &&
               greedy_seen.emplace(BudgetStateKey { greedy.state, greedy.action_points }).second) {
            const auto* actions = graph.actions(greedy.state, error);
            if (actions == nullptr) {
                break;
            }
            const auto witness = oracle.first_action(greedy.state, greedy.action_points);
            std::vector<PreviewedAction> previews;
            previews.reserve(actions->size());
            for (const OnDemandSafetyAction& action : *actions) {
                if (auto preview = build_next(greedy, action, false); preview.has_value()) {
                    previews.emplace_back(PreviewedAction { &action, std::move(*preview) });
                }
            }

            std::unordered_map<const OnDemandSafetyAction*, std::optional<RouteLabel>> verified;
            const auto verify_first = [&](const OnDemandSafetyAction* action) -> std::optional<RouteLabel> {
                if (const auto found = verified.find(action); found != verified.end()) {
                    return found->second;
                }
                if (!search_budget.consume()) {
                    return std::nullopt;
                }
                auto next = build_next(greedy, *action);
                verified.emplace(action, next);
                return next;
            };

            std::optional<RouteLabel> selected;
            if (root_move.movement == MovementKind::Walk && search_options.greedy_preview_depth > 2) {
                std::vector<PreviewedSequence> sequences;
                sequences.reserve(previews.size());
                for (const PreviewedAction& first : previews) {
                    PreviewedSequence sequence { { first.action }, first.preview };
                    std::unordered_set<BudgetStateKey, BudgetStateKeyHash> preview_seen;
                    preview_seen.emplace(BudgetStateKey { sequence.preview.state, sequence.preview.action_points });
                    while (static_cast<int>(sequence.actions.size()) < search_options.greedy_preview_depth &&
                           !graph.is_terminal(sequence.preview.state) && !search_budget.time_exhausted()) {
                        const auto* deeper_actions = graph.actions(sequence.preview.state, error);
                        if (deeper_actions == nullptr) {
                            break;
                        }
                        std::optional<PreviewedAction> best_next;
                        for (const OnDemandSafetyAction& deeper_action : *deeper_actions) {
                            auto next = build_next(sequence.preview, deeper_action, false);
                            if (!next.has_value()) {
                                continue;
                            }
                            if (!graph.is_terminal(next->state) && !heuristic_better(*next, sequence.preview)) {
                                continue;
                            }
                            if (!best_next.has_value() || heuristic_better(*next, best_next->preview)) {
                                best_next = PreviewedAction { &deeper_action, std::move(*next) };
                            }
                        }
                        if (!best_next.has_value() || !preview_seen
                                                           .emplace(
                                                               BudgetStateKey {
                                                                   best_next->preview.state,
                                                                   best_next->preview.action_points,
                                                               })
                                                           .second) {
                            break;
                        }
                        sequence.actions.emplace_back(best_next->action);
                        sequence.preview = std::move(best_next->preview);
                    }
                    sequences.emplace_back(std::move(sequence));
                }
                std::ranges::stable_sort(sequences, [&](const PreviewedSequence& lhs, const PreviewedSequence& rhs) {
                    return heuristic_better(lhs.preview, rhs.preview);
                });
                for (const PreviewedSequence& sequence : sequences) {
                    auto first = verify_first(sequence.actions.front());
                    if (!first.has_value()) {
                        if (search_budget.exhausted()) {
                            break;
                        }
                        continue;
                    }
                    RouteLabel verified_route = *first;
                    bool valid = true;
                    for (std::size_t index = 1; index < sequence.actions.size(); ++index) {
                        if (!search_budget.consume()) {
                            valid = false;
                            break;
                        }
                        auto next = build_next(verified_route, *sequence.actions[index]);
                        if (!next.has_value()) {
                            valid = false;
                            break;
                        }
                        verified_route = std::move(*next);
                    }
                    if (valid) {
                        selected = std::move(*first);
                        break;
                    }
                }
            }
            else if (root_move.movement == MovementKind::Walk && search_options.greedy_preview_depth <= 1) {
                std::ranges::stable_sort(previews, [&](const PreviewedAction& lhs, const PreviewedAction& rhs) {
                    return heuristic_better(lhs.preview, rhs.preview);
                });
                for (const PreviewedAction& preview : previews) {
                    if (auto first = verify_first(preview.action); first.has_value()) {
                        selected = std::move(*first);
                        break;
                    }
                    if (search_budget.exhausted()) {
                        break;
                    }
                }
            }
            else if (root_move.movement == MovementKind::Walk) {
                std::vector<PreviewedPair> pairs;
                for (const PreviewedAction& first : previews) {
                    pairs.emplace_back(PreviewedPair { first.action, nullptr, first.preview });
                    const auto* second_actions = graph.actions(first.preview.state, error);
                    if (second_actions == nullptr) {
                        continue;
                    }
                    for (const OnDemandSafetyAction& second_action : *second_actions) {
                        auto second = build_next(first.preview, second_action, false);
                        if (second.has_value() && heuristic_better(*second, first.preview)) {
                            pairs.emplace_back(
                                PreviewedPair {
                                    first.action,
                                    &second_action,
                                    std::move(*second),
                                });
                        }
                    }
                }
                std::ranges::stable_sort(pairs, [&](const PreviewedPair& lhs, const PreviewedPair& rhs) {
                    return heuristic_better(lhs.preview, rhs.preview);
                });
                for (const PreviewedPair& pair : pairs) {
                    auto first = verify_first(pair.first);
                    if (!first.has_value()) {
                        if (search_budget.exhausted()) {
                            break;
                        }
                        continue;
                    }
                    if (pair.second != nullptr) {
                        if (!search_budget.consume()) {
                            break;
                        }
                        if (!build_next(*first, *pair.second).has_value()) {
                            continue;
                        }
                    }
                    selected = std::move(*first);
                    break;
                }
            }
            else {
                std::ranges::stable_sort(previews, [&](const PreviewedAction& lhs, const PreviewedAction& rhs) {
                    return route_label_better(lhs.preview, rhs.preview, milestones, minimize_intermediate_interactions);
                });
                const auto find_first_safe = [&](const auto& predicate) -> std::optional<RouteLabel> {
                    for (const PreviewedAction& preview : previews) {
                        if (!predicate(preview)) {
                            continue;
                        }
                        if (auto next = verify_first(preview.action); next.has_value()) {
                            return next;
                        }
                        if (search_budget.exhausted()) {
                            break;
                        }
                    }
                    return std::nullopt;
                };
                selected = find_first_safe(
                    [&](const PreviewedAction& preview) { return preview.preview.progress != greedy.progress; });
                if (!selected.has_value()) {
                    selected = find_first_safe(
                        [&](const PreviewedAction& preview) { return graph.is_terminal(preview.preview.state); });
                }
                if (!selected.has_value() && witness.has_value()) {
                    selected = find_first_safe([&](const PreviewedAction& preview) {
                        return preview.action->candidate.action_id == *witness;
                    });
                }
                if (!selected.has_value()) {
                    selected = find_first_safe([](const PreviewedAction&) { return true; });
                }
            }

            if (!selected.has_value() && witness.has_value()) {
                const auto witness_action = std::ranges::find_if(*actions, [&](const OnDemandSafetyAction& action) {
                    return action.candidate.action_id == *witness;
                });
                if (witness_action != actions->end()) {
                    selected = build_next(greedy, *witness_action);
                }
            }
            if (!selected.has_value()) {
                break;
            }
            greedy = std::move(*selected);
        }
        return graph.is_terminal(greedy.state) ? std::optional<RouteLabel>(std::move(greedy)) : std::nullopt;
    };
    if (auto greedy = complete_greedy(initial); greedy.has_value()) {
        best = std::move(*greedy);
    }
    while (!pending.empty() && expanded_routes < route_expansion_budget_per_root && search_budget.consume()) {
        RouteLabel current = pending.top().route;
        pending.pop();
        ++expanded_routes;
        if (graph.is_terminal(current.state)) {
            if (!best.has_value() ||
                route_label_better(current, *best, milestones, minimize_intermediate_interactions)) {
                best = current;
            }
            continue;
        }
        if (best.has_value() &&
            !route_may_beat(map, graph, milestones, current, *best, minimize_intermediate_interactions)) {
            continue;
        }

        const auto* actions = graph.actions(current.state, error);
        if (actions == nullptr) {
            return best.value_or(initial);
        }
        for (const OnDemandSafetyAction& action : *actions) {
            if (action.outcomes.size() != 1) {
                continue;
            }
            const int requirement = oracle.action_requirement(action, current.action_points);
            if (requirement >= UnreachableActionPointRequirement) {
                continue;
            }
            const OnDemandSafetyOutcome& outcome = action.outcomes.front();
            const int remaining =
                action_points_after(current.action_points, action.action_point_cost, outcome.action_point_gain);
            RouteLabel next = current;
            next.state = outcome.successor;
            next.action_points = remaining;
            next.metric = add_metric(next.metric, move_metric(map, graph, current.state, action.candidate));
            if (minimize_intermediate_interactions && !graph.is_terminal(outcome.successor)) {
                next.intermediate_interactions +=
                    intermediate_interaction_cost(route_node_type(map, graph, current.state, action.candidate.target));
            }
            MoveCandidate planned_move = action.candidate;
            planned_move.action_point_requirement = requirement;
            next.steps.emplace_back(
                PlannedRouteStep {
                    std::move(planned_move),
                    current.action_points,
                    action.action_point_cost,
                    outcome.action_point_gain,
                    remaining,
                });
            if (const Node* target = map.find_node(action.candidate.target); target != nullptr) {
                Node entered = *target;
                entered.type = route_node_type(map, graph, current.state, target->id);
                if (entered.type == NodeType::Empty) {
                    entered.name.clear();
                }
                (void)advance_milestones(
                    entered,
                    milestones,
                    mission,
                    next.progress,
                    next.counted,
                    unknown_big_nodes_revealed(map, graph, current.state, entered.id));
                next.route.emplace_back(target->id);
            }
            auto& existing = labels[next.state];
            const bool dominated = std::ranges::any_of(existing, [&](const RouteLabel& value) {
                return value.progress == next.progress && value.counted == next.counted &&
                       value.action_points >= next.action_points &&
                       (!minimize_intermediate_interactions ||
                        value.intermediate_interactions <= next.intermediate_interactions) &&
                       route_metric_weakly_better(value.metric, next.metric);
            });
            if (dominated) {
                continue;
            }
            std::erase_if(existing, [&](const RouteLabel& value) {
                return value.progress == next.progress && value.counted == next.counted &&
                       next.action_points >= value.action_points &&
                       (!minimize_intermediate_interactions ||
                        next.intermediate_interactions <= value.intermediate_interactions) &&
                       route_metric_weakly_better(next.metric, value.metric);
            });
            existing.emplace_back(next);
            pending.emplace(make_pending(std::move(next)));
        }
    }
    return best.value_or(initial);
}

ReachableFeatures planned_route_features(const MapSnapshot& map, const std::vector<NodeId>& route)
{
    ReachableFeatures result;
    for (const NodeId id : route) {
        const Node* node = map.find_node(id);
        if (node == nullptr || node->type == NodeType::Empty || node->progress == NodeProgress::Removed) {
            continue;
        }
        result.node_types.emplace(to_string(node->type));
        if (node->type == NodeType::BattleBoss || node->type == NodeType::Final) {
            result.node_types.emplace("final");
        }
        result.has_badged = result.has_badged || node->badged;
        result.has_badged_incident = result.has_badged_incident || (node->badged && node->type == NodeType::Incident);
    }
    return result;
}

void merge_route_union(ReachableFeatures& destination, const ReachableFeatures& source)
{
    destination.node_types.insert(source.node_types.begin(), source.node_types.end());
    destination.has_badged = destination.has_badged || source.has_badged;
    destination.has_badged_incident = destination.has_badged_incident || source.has_badged_incident;
}

void intersect_route_features(ReachableFeatures& destination, const ReachableFeatures& source)
{
    std::erase_if(destination.node_types, [&](const std::string& type) { return !source.node_types.contains(type); });
    destination.has_badged = destination.has_badged && source.has_badged;
    destination.has_badged_incident = destination.has_badged_incident && source.has_badged_incident;
}

void set_route_feature_facts(FactStore& facts, const ReachableFeatures& possible, const ReachableFeatures& guaranteed)
{
    facts.set("candidate.route_node_types", sorted_types(possible.node_types));
    facts.set("candidate.guaranteed_route_node_types", sorted_types(guaranteed.node_types));
    facts.set("candidate.route_has_badged", possible.has_badged);
    facts.set("candidate.guaranteed_route_has_badged", guaranteed.has_badged);
    facts.set("candidate.route_has_badged_incident", possible.has_badged_incident);
    facts.set("candidate.guaranteed_route_has_badged_incident", guaranteed.has_badged_incident);
}

void set_first_move_facts(FactStore& facts, const RunState& run, const MoveCandidate& move)
{
    facts.set("candidate.movement", std::string(to_string(move.movement)));
    facts.set(
        "candidate.move_edges",
        static_cast<std::int64_t>(move.movement == MovementKind::Walk ? move.path.size() : 0));
    facts.set(
        "candidate.requires_movement_switch",
        !run.active_movement.has_value() || *run.active_movement != move.movement);
}

FactStore on_demand_candidate_facts(
    const MapSnapshot& map,
    OnDemandStateGraph& graph,
    const OnDemandSafetyAction& root_action,
    const RunState& run,
    bool strategy_end)
{
    FactStore facts;
    const MoveCandidate& move = root_action.candidate;
    const Node* target = map.find_node(move.target);
    facts.set("candidate.node_type", std::string(target == nullptr ? "unclassified" : to_string(target->type)));
    facts.set("candidate.node_name", target == nullptr ? std::string() : target->name);
    facts.set("candidate.badged", target != nullptr && target->badged);
    const bool combat = target != nullptr && is_combat_node_type(target->type);
    facts.set("candidate.combat", combat);
    facts.set("candidate.boss", combat && target->type == NodeType::BattleBoss);
    facts.set(
        "candidate.exit",
        target != nullptr && (target->type == NodeType::Final || target->type == NodeType::BattleBoss));
    facts.set("candidate.strategy_end", strategy_end);
    facts.set("candidate.uses_processing_item", move.movement != MovementKind::Walk);
    set_first_move_facts(facts, run, move);
    facts.set(
        "candidate.light_reveal_count",
        static_cast<std::int64_t>(
            target != nullptr && target->type == NodeType::Light
                ? unknown_big_nodes_revealed(map, graph, graph.initial_state(), target->id)
                : 0));

    set_route_feature_facts(facts, ReachableFeatures {}, ReachableFeatures {});
    return facts;
}
} // namespace

PreviewSafetyVerification BlackFlowPlanner::verify_previewed_move(
    const BlackFlowPlanRequest& request,
    const MoveCandidate& move,
    int exact_action_point_cost) const
{
    PreviewSafetyVerification result;
    if (request.map == nullptr || request.run == nullptr || request.policy == nullptr || request.facts == nullptr ||
        request.mission == nullptr) {
        result.error = "preview safety request is incomplete";
        return result;
    }

    auto projected = project_move_outcome(*request.map, *request.run, move, exact_action_point_cost, &result.error);
    if (!projected.has_value()) {
        return result;
    }
    result.action_points_after = projected->run.resources.action_points;

    std::string goal_error;
    auto safety_goal = SafetyGoalProgram::compile(*request.policy, *request.mission, *request.facts, &goal_error);
    if (!safety_goal.has_value()) {
        result.error = "strategy preview safety goal compilation failed: " + goal_error;
        return result;
    }
    SafetyGoalProgressId preview_goal_progress = safety_goal->initial_progress_id();
    if (const Node* entered = request.map->find_node(move.target); entered != nullptr) {
        int newly_revealed_unknown_big_nodes = 0;
        if (entered->type == NodeType::Light) {
            StateExpansionOptions source_options;
            source_options.strategy_goal_nodes = request.strategy_goal_nodes;
            source_options.graph_layer = GraphLayer::Confirmed;
            source_options.maximum_states = request.maximum_states;
            if (request.forbidden_actions != nullptr) {
                source_options.forbidden_action_ids = *request.forbidden_actions;
            }

            OnDemandStateGraph source_graph;
            if (!source_graph.initialize(*request.map, *request.run, std::move(source_options), &result.error)) {
                result.error = "confirmed source graph initialization failed: " + result.error;
                return result;
            }
            newly_revealed_unknown_big_nodes =
                unknown_big_nodes_revealed(*request.map, source_graph, source_graph.initial_state(), entered->id);
        }

        const auto advanced = safety_goal->advance_node(
            preview_goal_progress,
            *entered,
            newly_revealed_unknown_big_nodes,
            *request.facts,
            &goal_error);
        if (!advanced.has_value()) {
            result.error = "strategy preview safety goal transition failed: " + goal_error;
            return result;
        }
        preview_goal_progress = *advanced;
    }

    StateExpansionOptions options;
    options.strategy_goal_nodes = request.strategy_goal_nodes;
    options.graph_layer = GraphLayer::Confirmed;
    options.safety_goal = &*safety_goal;
    options.safety_goal_facts = request.facts;
    options.initial_goal_progress_id = preview_goal_progress;
    options.maximum_states = request.maximum_states;
    if (request.forbidden_actions != nullptr) {
        options.forbidden_action_ids = *request.forbidden_actions;
    }

    OnDemandStateGraph graph;
    if (!graph.initialize(*request.map, projected->run, std::move(options), &result.error)) {
        result.error = "confirmed successor graph initialization failed: " + result.error;
        return result;
    }
    OnDemandSafetyOracle oracle(graph, "Confirmed preview", request.route_search.safety_resource_dominance);
    result.required_action_points_after =
        oracle.requirement(graph.initial_state(), projected->run.resources.action_points);
    if (!oracle.error().empty()) {
        result.error = "confirmed successor safety calculation failed: " + oracle.error();
        return result;
    }
    result.safe = result.required_action_points_after < UnreachableActionPointRequirement;
    if (result.safe) {
        result.proof_depth = oracle.cached_depth(graph.initial_state(), result.required_action_points_after);
    }
    return result;
}

BlackFlowPlan BlackFlowPlanner::plan(const BlackFlowPlanRequest& request) const
{
    BlackFlowPlan result;
    if (request.map == nullptr || request.run == nullptr || request.policy == nullptr || request.facts == nullptr ||
        request.mission == nullptr) {
        result.error = "BlackFlow planner request is incomplete";
        return result;
    }
    if (request.route_search.time_budget_ms <= 0 || request.route_search.total_expansions == 0 ||
        request.route_search.expansions_per_root == 0 || request.route_search.greedy_preview_depth <= 0) {
        result.error = "route search options must be positive";
        return result;
    }
    result.map_revision = request.map->revision;
    result.cost_revision = request.run->costs.revision;
    const bool minimize_intermediate_interactions =
        std::ranges::find(request.policy->route_preferences, RoutePreference::MinimizeIntermediateInteractions) !=
        request.policy->route_preferences.end();
    std::string error;
    auto safety_goal = SafetyGoalProgram::compile(*request.policy, *request.mission, *request.facts, &error);
    if (!safety_goal.has_value()) {
        result.error = "strategy safety goal compilation failed: " + error;
        return result;
    }
    SafetyGoalProgram relaxed_safety_goal = *safety_goal;

    StateExpansionOptions confirmed_options;
    confirmed_options.strategy_goal_nodes = request.strategy_goal_nodes;
    confirmed_options.graph_layer = GraphLayer::Confirmed;
    confirmed_options.safety_goal = &*safety_goal;
    confirmed_options.safety_goal_facts = request.facts;
    confirmed_options.maximum_states = request.maximum_states;
    if (request.forbidden_actions != nullptr) {
        confirmed_options.forbidden_action_ids = *request.forbidden_actions;
    }

    OnDemandStateGraph confirmed_graph;
    if (!confirmed_graph.initialize(*request.map, *request.run, confirmed_options, &error)) {
        result.error = "confirmed on-demand graph initialization failed: " + error;
        return result;
    }
    StateExpansionOptions relaxed_options = confirmed_options;
    relaxed_options.graph_layer = GraphLayer::Relaxed;
    relaxed_options.safety_goal = &relaxed_safety_goal;
    OnDemandStateGraph relaxed_graph;
    if (!relaxed_graph.initialize(*request.map, *request.run, relaxed_options, &error)) {
        result.error = "relaxed on-demand graph initialization failed: " + error;
        return result;
    }

    const int current_action_points = request.run->resources.action_points;
    OnDemandSafetyOracle confirmed_oracle(confirmed_graph, "Confirmed", request.route_search.safety_resource_dominance);
    OnDemandSafetyOracle relaxed_oracle(relaxed_graph, "Relaxed", request.route_search.safety_resource_dominance);
    auto confirmed_root_requirement_task = std::async(std::launch::async, [&] {
        return confirmed_oracle.requirement(confirmed_graph.initial_state(), current_action_points);
    });
    result.relaxed_safety.required_action_points =
        relaxed_oracle.requirement(relaxed_graph.initial_state(), current_action_points);
    result.safety.required_action_points = confirmed_root_requirement_task.get();
    if (!confirmed_oracle.error().empty() || !relaxed_oracle.error().empty()) {
        result.error = "root safety calculation failed: " +
                       (!confirmed_oracle.error().empty() ? confirmed_oracle.error() : relaxed_oracle.error());
        return result;
    }
    result.safety.solution.required_action_points.emplace(
        confirmed_graph.initial_state(),
        result.safety.required_action_points);
    if (result.safety.required_action_points < UnreachableActionPointRequirement) {
        result.safety.proof_depth =
            confirmed_oracle.cached_depth(confirmed_graph.initial_state(), result.safety.required_action_points);
        result.safety.first_action = confirmed_oracle.lexicographic_first_action(
            confirmed_graph.initial_state(),
            result.safety.required_action_points);
        if (result.safety.proof_depth.has_value()) {
            result.safety.solution.proof_depth.emplace(confirmed_graph.initial_state(), *result.safety.proof_depth);
        }
        if (result.safety.first_action.has_value()) {
            result.safety.solution.selected_actions.emplace(
                confirmed_graph.initial_state(),
                *result.safety.first_action);
        }
    }

    const auto* confirmed_root_action_list = confirmed_graph.actions(confirmed_graph.initial_state(), &error);
    if (confirmed_root_action_list == nullptr) {
        result.error = "confirmed root action generation failed: " + error;
        return result;
    }
    if (result.safety.first_action.has_value()) {
        const auto found = std::ranges::find_if(*confirmed_root_action_list, [&](const OnDemandSafetyAction& action) {
            return action.candidate.action_id == *result.safety.first_action;
        });
        if (found != confirmed_root_action_list->end()) {
            result.escape_first_action = found->candidate;
        }
    }

    result.relaxed_safety.solution.required_action_points.emplace(
        relaxed_graph.initial_state(),
        result.relaxed_safety.required_action_points);
    if (result.relaxed_safety.required_action_points < UnreachableActionPointRequirement) {
        result.relaxed_safety.proof_depth =
            relaxed_oracle.cached_depth(relaxed_graph.initial_state(), result.relaxed_safety.required_action_points);
        result.relaxed_safety.first_action = relaxed_oracle.lexicographic_first_action(
            relaxed_graph.initial_state(),
            result.relaxed_safety.required_action_points);
        if (result.relaxed_safety.proof_depth.has_value()) {
            result.relaxed_safety.solution.proof_depth.emplace(
                relaxed_graph.initial_state(),
                *result.relaxed_safety.proof_depth);
        }
        if (result.relaxed_safety.first_action.has_value()) {
            result.relaxed_safety.solution.selected_actions.emplace(
                relaxed_graph.initial_state(),
                *result.relaxed_safety.first_action);
        }
    }

    const auto* relaxed_root_action_list = relaxed_graph.actions(relaxed_graph.initial_state(), &error);
    if (relaxed_root_action_list == nullptr) {
        result.error = "relaxed root action generation failed: " + error;
        return result;
    }
    result.confirmed_state_count = confirmed_graph.state_count();
    result.relaxed_state_count = relaxed_graph.state_count();
    std::unordered_map<std::string, const OnDemandSafetyAction*> confirmed_root_actions;
    for (const OnDemandSafetyAction& action : *confirmed_root_action_list) {
        confirmed_root_actions.insert_or_assign(action.candidate.action_id, &action);
    }

    std::unordered_map<std::string, int> confirmed_root_requirements;
    std::unordered_map<std::string, int> relaxed_root_requirements;
    confirmed_root_requirements.reserve(confirmed_root_action_list->size());
    relaxed_root_requirements.reserve(relaxed_root_action_list->size());
    for (const OnDemandSafetyAction& action : *relaxed_root_action_list) {
        relaxed_root_requirements.emplace(
            action.candidate.action_id,
            relaxed_oracle.action_requirement(action, current_action_points));
    }
    for (const OnDemandSafetyAction& action : *confirmed_root_action_list) {
        if (!action.candidate.controllable) {
            confirmed_root_requirements.emplace(
                action.candidate.action_id,
                confirmed_oracle.action_requirement(action, current_action_points));
        }
    }
    if (!confirmed_oracle.error().empty() || !relaxed_oracle.error().empty()) {
        result.error = "batched root safety calculation failed: " +
                       (!confirmed_oracle.error().empty() ? confirmed_oracle.error() : relaxed_oracle.error());
        return result;
    }

    const auto milestones = route_milestones(
        *request.policy,
        *request.mission,
        request.run->floor,
        *request.facts,
        request.unresolved_hidden_end_milestone_ids);
    RouteSearchBudget route_search_budget {
        std::chrono::steady_clock::now() + std::chrono::milliseconds(request.route_search.time_budget_ms),
        request.route_search.total_expansions,
        request.route_search.total_expansions,
    };
    std::vector<PolicyCandidate> policy_candidates;
    std::vector<const OnDemandSafetyAction*> ordered_root_actions;
    ordered_root_actions.reserve(relaxed_root_action_list->size());
    for (const OnDemandSafetyAction& action : *relaxed_root_action_list) {
        ordered_root_actions.emplace_back(&action);
    }
    std::ranges::stable_sort(ordered_root_actions, [](const auto* lhs, const auto* rhs) {
        const int lhs_processing = lhs->candidate.movement == MovementKind::Walk ? 0 : 1;
        const int rhs_processing = rhs->candidate.movement == MovementKind::Walk ? 0 : 1;
        return lhs_processing < rhs_processing;
    });
    for (const OnDemandSafetyAction* action_pointer : ordered_root_actions) {
        const OnDemandSafetyAction& action = *action_pointer;
        PolicyCandidate candidate;
        candidate.move = action.candidate;
        const int relaxed_requirement = relaxed_root_requirements.at(action.candidate.action_id);
        candidate.move.action_point_requirement = relaxed_requirement;
        const bool relaxed_safe = relaxed_requirement < UnreachableActionPointRequirement;
        bool confirmed_safe = false;
        if (const auto confirmed = confirmed_root_actions.find(candidate.move.action_id);
            confirmed != confirmed_root_actions.end()) {
            const auto requirement = confirmed_root_requirements.find(candidate.move.action_id);
            if (requirement != confirmed_root_requirements.end() &&
                requirement->second < UnreachableActionPointRequirement) {
                candidate.move = confirmed->second->candidate;
                candidate.move.action_point_requirement = requirement->second;
                confirmed_safe = true;
            }
        }
        candidate.safe = confirmed_safe || (relaxed_safe && candidate.move.controllable);
        const bool probing_target = request.probe_target.has_value() && candidate.move.target == *request.probe_target;
        candidate.move.requires_preview_verification = candidate.safe && (!confirmed_safe || probing_target);
        candidate.facts = on_demand_candidate_facts(
            *request.map,
            relaxed_graph,
            action,
            *request.run,
            request.strategy_goal_nodes.contains(action.candidate.target));
        if (!error.empty() || !relaxed_oracle.error().empty()) {
            result.error =
                "candidate reachability calculation failed: " + (!error.empty() ? error : relaxed_oracle.error());
            return result;
        }
        candidate.facts.set("candidate.preview_required", candidate.move.requires_preview_verification);
        const Node* target = request.map->find_node(candidate.move.target);
        candidate.battle_count = target != nullptr && is_combat_node_type(target->type) ? 1 : 0;
        candidate.estimated_duration = candidate.move.movement == MovementKind::Walk
                                           ? std::max(1, static_cast<int>(candidate.move.path.size()))
                                           : 1;
        candidate.risk_score = target == nullptr || !target->identity_revealed
                                   ? 5
                                   : (target->type == NodeType::BattleElite || target->type == NodeType::BattleBoss
                                          ? 10
                                          : candidate.battle_count * 4);

        if (!candidate.safe) {
            policy_candidates.emplace_back(std::move(candidate));
            continue;
        }

        bool first_outcome = true;
        ReachableFeatures possible_route_features;
        std::optional<ReachableFeatures> guaranteed_route_features;
        std::vector<int> guaranteed_progress;
        RouteMetric worst_metric;
        int worst_intermediate_interactions = 0;
        for (const OnDemandSafetyOutcome& outcome : action.outcomes) {
            const int remaining =
                action_points_after(current_action_points, action.action_point_cost, outcome.action_point_gain);
            RouteLabel route = best_route_after_outcome(
                *request.map,
                relaxed_graph,
                relaxed_oracle,
                milestones,
                *request.mission,
                candidate.move,
                action,
                outcome,
                current_action_points,
                remaining,
                request.route_search,
                route_search_budget,
                minimize_intermediate_interactions,
                &error);
            if (!error.empty() || !relaxed_oracle.error().empty()) {
                result.error =
                    "candidate route calculation failed: " + (!error.empty() ? error : relaxed_oracle.error());
                return result;
            }
            worst_intermediate_interactions =
                std::max(worst_intermediate_interactions, route.intermediate_interactions);
            const ReachableFeatures outcome_features = planned_route_features(*request.map, route.route);
            merge_route_union(possible_route_features, outcome_features);
            if (!guaranteed_route_features.has_value()) {
                guaranteed_route_features = outcome_features;
            }
            else {
                intersect_route_features(*guaranteed_route_features, outcome_features);
            }
            if (first_outcome) {
                guaranteed_progress = route.progress;
                candidate.immediate_milestone_ids = route.immediate_milestone_ids;
                candidate.planned_route = route.route;
                candidate.planned_route_steps = route.steps;
                worst_metric = route.metric;
                first_outcome = false;
            }
            else {
                for (std::size_t index = 0; index < guaranteed_progress.size(); ++index) {
                    guaranteed_progress[index] = std::min(guaranteed_progress[index], route.progress[index]);
                }
                std::erase_if(candidate.immediate_milestone_ids, [&](const std::string& id) {
                    return std::ranges::find(route.immediate_milestone_ids, id) == route.immediate_milestone_ids.end();
                });
                worst_metric.battles = std::max(worst_metric.battles, route.metric.battles);
                worst_metric.processing_moves = std::max(worst_metric.processing_moves, route.metric.processing_moves);
                worst_metric.duration = std::max(worst_metric.duration, route.metric.duration);
                candidate.planned_route.clear();
                candidate.planned_route_steps.clear();
            }
        }
        set_route_feature_facts(
            candidate.facts,
            possible_route_features,
            guaranteed_route_features.value_or(ReachableFeatures {}));
        for (std::size_t index = 0; index < milestones.size(); ++index) {
            candidate.milestone_progress.emplace(milestones[index].definition->id, guaranteed_progress[index]);
        }
        candidate.battle_count = worst_metric.battles;
        candidate.intermediate_interaction_count = worst_intermediate_interactions;
        candidate.processing_move_count = worst_metric.processing_moves;
        candidate.estimated_duration = worst_metric.duration;
        candidate.development_score = 0;
        policy_candidates.emplace_back(std::move(candidate));
    }

    const bool has_safe_noncombat_alternative =
        std::ranges::any_of(policy_candidates, [](const PolicyCandidate& candidate) {
            return candidate.safe && !boolean_fact(candidate.facts, "candidate.combat");
        });
    for (auto& candidate : policy_candidates) {
        candidate.facts.set(
            "candidate.combat_is_optional",
            candidate.safe && boolean_fact(candidate.facts, "candidate.combat") && has_safe_noncombat_alternative);
    }

    bool has_safe_direct_exit = false;
    bool has_safe_non_exit = false;
    for (const auto& candidate : policy_candidates) {
        if (!candidate.safe) {
            continue;
        }
        if (boolean_fact(candidate.facts, "candidate.exit")) {
            has_safe_direct_exit = true;
        }
        else {
            has_safe_non_exit = true;
        }
    }
    FactStore policy_facts = *request.facts;
    policy_facts.set(
        "safety_margin_low",
        boolean_fact(*request.facts, "safety_margin_low") || (has_safe_direct_exit && !has_safe_non_exit));
    policy_facts.set(
        "floor_development_exhausted",
        boolean_fact(*request.facts, "floor_development_exhausted") || !has_safe_non_exit);

    ResourceRegistry resources;
    PolicyExecutor executor;
    if (request.probe_target.has_value()) {
        std::vector<PolicyCandidate> probe_candidates;
        std::ranges::copy_if(
            policy_candidates,
            std::back_inserter(probe_candidates),
            [&](const PolicyCandidate& candidate) {
                return candidate.safe && candidate.move.target == *request.probe_target;
            });
        if (!probe_candidates.empty()) {
            result.decision =
                executor
                    .choose(*request.policy, policy_facts, *request.mission, *request.run, resources, probe_candidates);
        }
    }
    if (!result.decision.selected.has_value()) {
        result.decision =
            executor
                .choose(*request.policy, policy_facts, *request.mission, *request.run, resources, policy_candidates);
    }
    if (!result.decision.selected.has_value()) {
        result.error = result.decision.reason;
    }
    else if (result.decision.selected->controllable) {
        const auto confirmed = confirmed_root_actions.find(result.decision.selected->action_id);
        if (confirmed != confirmed_root_actions.end()) {
            const int confirmed_requirement =
                confirmed_oracle.action_requirement(*confirmed->second, current_action_points);
            if (!confirmed_oracle.error().empty()) {
                result.error = "selected confirmed safety calculation failed: " + confirmed_oracle.error();
                return result;
            }
            if (confirmed_requirement < UnreachableActionPointRequirement) {
                MoveCandidate selected = confirmed->second->candidate;
                selected.action_point_requirement = confirmed_requirement;
                selected.requires_preview_verification =
                    request.probe_target.has_value() && selected.target == *request.probe_target;
                result.decision.selected = selected;
                if (!result.decision.planned_route_steps.empty() &&
                    result.decision.planned_route_steps.front().move.action_id == selected.action_id) {
                    result.decision.planned_route_steps.front().move = selected;
                }
            }
        }
    }
    result.confirmed_state_count = confirmed_graph.state_count();
    result.relaxed_state_count = relaxed_graph.state_count();
    result.route_search_expansions = route_search_budget.consumed_expansions();
    result.route_search_time_exhausted = route_search_budget.time_exhausted();
    result.route_search_expansions_exhausted = route_search_budget.remaining_expansions == 0;
    return result;
}
} // namespace asst::blackflow
