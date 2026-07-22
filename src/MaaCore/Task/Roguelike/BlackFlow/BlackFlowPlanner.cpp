#include "BlackFlowPlanner.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace asst::blackflow
{
namespace
{
int action_requirement(const SafetyAction& action, const SafetySolution& solution)
{
    std::int64_t requirement = std::max(action.minimum_action_points_to_start, action.action_point_cost);
    for (const auto& outcome : action.outcomes) {
        const int successor = solution.requirement(outcome.successor);
        if (successor >= UnreachableActionPointRequirement) {
            return UnreachableActionPointRequirement;
        }
        requirement = std::max(
            requirement,
            static_cast<std::int64_t>(successor) + action.action_point_cost - outcome.action_point_gain);
    }
    return static_cast<int>(std::clamp<std::int64_t>(requirement, 0, UnreachableActionPointRequirement));
}

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

ReachableFeatures reachable_features(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    const SafetySolution& solution,
    SafetyStateId initial,
    int initial_action_points)
{
    std::unordered_map<SafetyStateId, std::vector<const SafetyAction*>> actions;
    for (const auto& action : expanded.problem.actions) {
        actions[action.source].emplace_back(&action);
    }
    std::unordered_map<SafetyStateId, int> best_action_points;
    std::deque<std::pair<SafetyStateId, int>> pending;
    pending.emplace_back(initial, initial_action_points);
    best_action_points.emplace(initial, initial_action_points);
    ReachableFeatures result;
    while (!pending.empty()) {
        const auto [state_id, action_points] = pending.front();
        pending.pop_front();
        if (state_id >= expanded.planner_states.size()) {
            continue;
        }
        const Node* node = map.find_node(expanded.planner_states[state_id].node);
        if (node != nullptr && node->type != NodeType::Empty) {
            result.node_types.emplace(to_string(node->type));
            result.has_badged = result.has_badged || node->badged;
            result.has_badged_incident =
                result.has_badged_incident || (node->badged && node->type == NodeType::Incident);
        }
        const auto outgoing = actions.find(state_id);
        if (outgoing == actions.end()) {
            continue;
        }
        for (const SafetyAction* action : outgoing->second) {
            if (action_points < action_requirement(*action, solution)) {
                continue;
            }
            for (const auto& outcome : action->outcomes) {
                const int remaining =
                    action_points_after(action_points, action->action_point_cost, outcome.action_point_gain);
                if (remaining < solution.requirement(outcome.successor)) {
                    continue;
                }
                const auto previous = best_action_points.find(outcome.successor);
                if (previous == best_action_points.end() || remaining > previous->second) {
                    best_action_points.insert_or_assign(outcome.successor, remaining);
                    pending.emplace_back(outcome.successor, remaining);
                }
            }
        }
    }
    return result;
}

int unknown_big_nodes_revealed(const MapSnapshot& map, NodeId node)
{
    int count = 0;
    for (const NodeId id : map.nodes_within_manhattan(node, 2)) {
        const Node* candidate = map.find_node(id);
        if (candidate != nullptr && candidate->type != NodeType::Empty && !candidate->identity_revealed) {
            ++count;
        }
    }
    return count;
}

NodeType
    route_node_type(const MapSnapshot& map, const ExpandedSafetyProblem& expanded, SafetyStateId source, NodeId node)
{
    const Node* target = map.find_node(node);
    if (target == nullptr) {
        return NodeType::Unknown;
    }
    if (source < expanded.planner_states.size() && !target->traversal.repeatable &&
        std::ranges::binary_search(expanded.planner_states[source].emptied_nodes, node)) {
        return NodeType::Empty;
    }
    return target->type;
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

RouteMetric move_metric(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    SafetyStateId source,
    const MoveCandidate& move)
{
    const bool battle = is_combat_node_type(route_node_type(map, expanded, source, move.target));
    return {
        battle ? 1 : 0,
        move.movement == MovementKind::Walk ? 0 : 1,
        move.movement == MovementKind::Walk ? std::max(1, static_cast<int>(move.path.size())) : 1,
    };
}

struct RouteMilestone
{
    const Milestone* definition = nullptr;
    int initial_progress = 0;
};

std::vector<RouteMilestone>
    route_milestones(const ResolvedPolicy& policy, const MissionState& mission, int floor, const FactStore& facts)
{
    std::vector<RouteMilestone> result;
    for (const Milestone& milestone : policy.milestones) {
        const MilestoneStatus status = mission.status(milestone.id);
        if (floor < milestone.floor_begin || floor > milestone.floor_end || status == MilestoneStatus::Satisfied ||
            status == MilestoneStatus::Missed || status == MilestoneStatus::Impossible ||
            !milestone.active_if.evaluate(facts) ||
            (milestone.target_node_types.empty() && milestone.target_node_names.empty())) {
            continue;
        }
        result.emplace_back(RouteMilestone { &milestone, mission.progress(milestone.id) });
    }
    std::ranges::sort(result, [](const RouteMilestone& lhs, const RouteMilestone& rhs) {
        return std::tie(lhs.definition->kind, lhs.definition->rank, lhs.definition->id) <
               std::tie(rhs.definition->kind, rhs.definition->rank, rhs.definition->id);
    });
    return result;
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

void advance_milestones(
    NodeId node,
    NodeType type,
    std::string_view name,
    const std::vector<RouteMilestone>& milestones,
    const MissionState& mission,
    std::vector<int>& progress,
    CountedNodes& counted)
{
    const std::vector<int> before = progress;
    for (std::size_t index = 0; index < milestones.size(); ++index) {
        const Milestone& milestone = *milestones[index].definition;
        const bool type_matches =
            std::ranges::find(milestone.target_node_types, type) != milestone.target_node_types.end();
        const bool name_matches =
            std::ranges::find(milestone.target_node_names, name) != milestone.target_node_names.end();
        if (before[index] >= milestone.required_count || (!type_matches && !name_matches) ||
            std::ranges::find(counted[index], node) != counted[index].end() ||
            !simulated_prerequisites_satisfied(milestone, milestones, before, mission)) {
            continue;
        }
        counted[index].emplace_back(node);
        std::ranges::sort(counted[index]);
        progress[index] = std::min(milestone.required_count, before[index] + 1);
    }
}

std::vector<int>
    mandatory_progress_score(const std::vector<RouteMilestone>& milestones, const std::vector<int>& progress)
{
    std::vector<int> score;
    std::size_t begin = 0;
    while (begin < milestones.size()) {
        while (begin < milestones.size() && milestones[begin].definition->kind != MilestoneKind::Mandatory) {
            ++begin;
        }
        if (begin == milestones.size()) {
            break;
        }
        const int rank = milestones[begin].definition->rank;
        int completed = 0;
        int sum = 0;
        std::size_t end = begin;
        while (end < milestones.size() && milestones[end].definition->kind == MilestoneKind::Mandatory &&
               milestones[end].definition->rank == rank) {
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
    std::vector<NodeId> route;
    std::vector<PlannedRouteStep> steps;
};

bool route_label_better(const RouteLabel& lhs, const RouteLabel& rhs, const std::vector<RouteMilestone>& milestones)
{
    const auto lhs_mandatory = mandatory_progress_score(milestones, lhs.progress);
    const auto rhs_mandatory = mandatory_progress_score(milestones, rhs.progress);
    if (score_greater(lhs_mandatory, rhs_mandatory)) {
        return true;
    }
    if (score_greater(rhs_mandatory, lhs_mandatory)) {
        return false;
    }
    const auto lhs_preferred = preferred_progress_score(milestones, lhs.progress);
    const auto rhs_preferred = preferred_progress_score(milestones, rhs.progress);
    for (std::size_t index = 0; index < std::min(lhs_preferred.size(), rhs_preferred.size()); ++index) {
        if (lhs_preferred[index] == rhs_preferred[index]) {
            continue;
        }
        const std::int64_t lhs_utility =
            lhs_preferred[index] - lhs.metric.duration - lhs.metric.battles - lhs.metric.processing_moves;
        const std::int64_t rhs_utility =
            rhs_preferred[index] - rhs.metric.duration - rhs.metric.battles - rhs.metric.processing_moves;
        if (lhs_utility != rhs_utility) {
            return lhs_utility > rhs_utility;
        }
        return lhs_preferred[index] > rhs_preferred[index];
    }
    if (std::tie(lhs.metric.battles, lhs.metric.processing_moves, lhs.metric.duration) !=
        std::tie(rhs.metric.battles, rhs.metric.processing_moves, rhs.metric.duration)) {
        return std::tie(lhs.metric.battles, lhs.metric.processing_moves, lhs.metric.duration) <
               std::tie(rhs.metric.battles, rhs.metric.processing_moves, rhs.metric.duration);
    }
    return lhs.action_points > rhs.action_points;
}

RouteLabel best_route_after_outcome(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    const SafetySolution& solution,
    const std::vector<RouteMilestone>& milestones,
    const MissionState& mission,
    const MoveCandidate& root_move,
    const SafetyAction& root_action,
    const SafetyOutcome& root_outcome,
    int initial_action_points,
    int remaining_action_points)
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
    initial.metric = move_metric(map, expanded, expanded.initial_state, root_move);
    initial.steps.emplace_back(
        PlannedRouteStep {
            root_move,
            initial_action_points,
            root_action.action_point_cost,
            root_outcome.action_point_gain,
            remaining_action_points,
        });
    NodeId entered_node = root_move.target;
    if (entered_node == InvalidNodeId && root_outcome.successor < expanded.planner_states.size()) {
        entered_node = expanded.planner_states[root_outcome.successor].node;
    }
    if (const Node* target = map.find_node(entered_node); target != nullptr) {
        const NodeType entered_type = route_node_type(map, expanded, expanded.initial_state, target->id);
        advance_milestones(
            target->id,
            entered_type,
            entered_type == NodeType::Empty ? std::string_view {} : std::string_view { target->name },
            milestones,
            mission,
            initial.progress,
            initial.counted);
        initial.route.emplace_back(target->id);
    }

    std::unordered_map<SafetyStateId, std::vector<const SafetyAction*>> actions;
    for (const SafetyAction& action : expanded.problem.actions) {
        actions[action.source].emplace_back(&action);
    }
    std::unordered_map<SafetyStateId, std::vector<RouteLabel>> labels;
    labels[initial.state].emplace_back(initial);
    std::deque<RouteLabel> pending;
    pending.emplace_back(initial);
    std::optional<RouteLabel> best;
    while (!pending.empty()) {
        RouteLabel current = std::move(pending.front());
        pending.pop_front();
        if (current.state < expanded.problem.states.size() && expanded.problem.states[current.state].safe_exit &&
            (!best.has_value() || route_label_better(current, *best, milestones))) {
            best = current;
        }
        const auto outgoing = actions.find(current.state);
        if (outgoing == actions.end()) {
            continue;
        }
        for (const SafetyAction* action : outgoing->second) {
            if (action->outcomes.size() != 1 || current.action_points < action_requirement(*action, solution)) {
                continue;
            }
            const auto move = expanded.action_candidates.find(action->id);
            if (move == expanded.action_candidates.end()) {
                continue;
            }
            const SafetyOutcome& outcome = action->outcomes.front();
            const int remaining =
                action_points_after(current.action_points, action->action_point_cost, outcome.action_point_gain);
            if (remaining < solution.requirement(outcome.successor)) {
                continue;
            }
            RouteLabel next = current;
            next.state = outcome.successor;
            next.action_points = remaining;
            next.metric = add_metric(next.metric, move_metric(map, expanded, action->source, move->second));
            MoveCandidate planned_move = move->second;
            planned_move.action_point_requirement = action_requirement(*action, solution);
            next.steps.emplace_back(
                PlannedRouteStep {
                    std::move(planned_move),
                    current.action_points,
                    action->action_point_cost,
                    outcome.action_point_gain,
                    remaining,
                });
            if (const Node* target = map.find_node(move->second.target); target != nullptr) {
                const NodeType entered_type = route_node_type(map, expanded, action->source, target->id);
                advance_milestones(
                    target->id,
                    entered_type,
                    entered_type == NodeType::Empty ? std::string_view {} : std::string_view { target->name },
                    milestones,
                    mission,
                    next.progress,
                    next.counted);
                next.route.emplace_back(target->id);
            }
            auto& existing = labels[next.state];
            const bool dominated = std::ranges::any_of(existing, [&](const RouteLabel& value) {
                return value.progress == next.progress && value.counted == next.counted &&
                       value.action_points >= next.action_points && value.metric.battles <= next.metric.battles &&
                       value.metric.processing_moves <= next.metric.processing_moves &&
                       value.metric.duration <= next.metric.duration;
            });
            if (dominated) {
                continue;
            }
            std::erase_if(existing, [&](const RouteLabel& value) {
                return value.progress == next.progress && value.counted == next.counted &&
                       next.action_points >= value.action_points && next.metric.battles <= value.metric.battles &&
                       next.metric.processing_moves <= value.metric.processing_moves &&
                       next.metric.duration <= value.metric.duration;
            });
            existing.emplace_back(next);
            pending.emplace_back(std::move(next));
        }
    }
    return best.value_or(initial);
}

bool boolean_fact(const FactStore& facts, std::string_view name)
{
    const FactValue* value = facts.find(name);
    return value != nullptr && std::holds_alternative<bool>(*value) && std::get<bool>(*value);
}
} // namespace

FactStore BlackFlowPlanner::candidate_facts(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    const SafetySolution& solution,
    const SafetyAction& root_action,
    int current_action_points) const
{
    FactStore facts;
    const auto move_iter = expanded.action_candidates.find(root_action.id);
    if (move_iter == expanded.action_candidates.end()) {
        return facts;
    }
    const MoveCandidate& move = move_iter->second;
    const Node* target = map.find_node(move.target);
    facts.set("candidate.node_type", std::string(target == nullptr ? "unclassified" : to_string(target->type)));
    facts.set("candidate.node_name", target == nullptr ? std::string() : target->name);
    facts.set("candidate.badged", target != nullptr && target->badged);
    const bool combat = target != nullptr && is_combat_node_type(target->type);
    facts.set("candidate.combat", combat);
    facts.set("candidate.boss", combat && target->type == NodeType::BattleBoss);
    facts.set("candidate.exit", target != nullptr && target->type == NodeType::Final);
    facts.set("candidate.uses_processing_item", move.movement != MovementKind::Walk);
    facts.set(
        "candidate.light_reveal_count",
        static_cast<std::int64_t>(
            target != nullptr && target->type == NodeType::Light ? unknown_big_nodes_revealed(map, target->id) : 0));

    ReachableFeatures union_features;
    std::optional<ReachableFeatures> guaranteed_features;
    for (const auto& outcome : root_action.outcomes) {
        const int remaining =
            action_points_after(current_action_points, root_action.action_point_cost, outcome.action_point_gain);
        auto features = reachable_features(map, expanded, solution, outcome.successor, remaining);
        union_features.node_types.insert(features.node_types.begin(), features.node_types.end());
        union_features.has_badged = union_features.has_badged || features.has_badged;
        union_features.has_badged_incident = union_features.has_badged_incident || features.has_badged_incident;
        if (!guaranteed_features.has_value()) {
            guaranteed_features = std::move(features);
        }
        else {
            std::erase_if(guaranteed_features->node_types, [&](const std::string& type) {
                return !features.node_types.contains(type);
            });
            guaranteed_features->has_badged = guaranteed_features->has_badged && features.has_badged;
            guaranteed_features->has_badged_incident =
                guaranteed_features->has_badged_incident && features.has_badged_incident;
        }
    }
    const ReachableFeatures guaranteed = guaranteed_features.value_or(ReachableFeatures {});
    facts.set("candidate.route_node_types", sorted_types(union_features.node_types));
    facts.set("candidate.guaranteed_route_node_types", sorted_types(guaranteed.node_types));
    facts.set("candidate.route_has_badged", union_features.has_badged);
    facts.set("candidate.guaranteed_route_has_badged", guaranteed.has_badged);
    facts.set("candidate.route_has_badged_incident", union_features.has_badged_incident);
    facts.set("candidate.guaranteed_route_has_badged_incident", guaranteed.has_badged_incident);
    return facts;
}

BlackFlowPlan BlackFlowPlanner::plan(const BlackFlowPlanRequest& request) const
{
    BlackFlowPlan result;
    if (request.map == nullptr || request.run == nullptr || request.policy == nullptr || request.facts == nullptr ||
        request.mission == nullptr) {
        result.error = "BlackFlow planner request is incomplete";
        return result;
    }
    result.map_revision = request.map->revision;
    result.cost_revision = request.run->costs.revision;
    if (request.mission->viability == MissionViability::Impossible) {
        result.error = "active strategy has an impossible mandatory milestone";
        return result;
    }

    StateExpansionOptions options;
    options.strategy_terminal_nodes = request.strategy_terminal_nodes;
    if (request.forbidden_actions != nullptr) {
        options.forbidden_action_ids = *request.forbidden_actions;
    }
    options.maximum_states = request.maximum_states;

    BlackFlowStateExpander expander;
    std::string error;
    auto expanded = expander.build(*request.map, *request.run, options, &error);
    if (!expanded.has_value()) {
        result.error = "state expansion failed: " + error;
        return result;
    }

    SafetyPlanner safety_planner;
    auto assessment = safety_planner.assess(expanded->problem, expanded->initial_state, &error);
    if (!assessment.has_value()) {
        result.error = "safety calculation failed: " + error;
        return result;
    }
    result.safety = std::move(*assessment);
    if (result.safety.first_action.has_value()) {
        const auto action = expanded->action_candidates.find(*result.safety.first_action);
        if (action != expanded->action_candidates.end()) {
            result.escape_first_action = action->second;
        }
    }

    const auto milestones = route_milestones(*request.policy, *request.mission, request.run->floor, *request.facts);
    std::vector<PolicyCandidate> policy_candidates;
    for (const SafetyAction& action : expanded->problem.actions) {
        if (action.source != expanded->initial_state) {
            continue;
        }
        const auto move = expanded->action_candidates.find(action.id);
        if (move == expanded->action_candidates.end()) {
            continue;
        }
        PolicyCandidate candidate;
        candidate.move = move->second;
        candidate.move.action_point_requirement = action_requirement(action, result.safety.solution);
        candidate.safe = request.run->resources.action_points >= candidate.move.action_point_requirement;
        candidate.facts = candidate_facts(
            *request.map,
            *expanded,
            result.safety.solution,
            action,
            request.run->resources.action_points);
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

        bool first_outcome = true;
        std::vector<int> guaranteed_progress;
        RouteMetric worst_metric;
        for (const SafetyOutcome& outcome : action.outcomes) {
            const int remaining = action_points_after(
                request.run->resources.action_points,
                action.action_point_cost,
                outcome.action_point_gain);
            RouteLabel route = best_route_after_outcome(
                *request.map,
                *expanded,
                result.safety.solution,
                milestones,
                *request.mission,
                candidate.move,
                action,
                outcome,
                request.run->resources.action_points,
                remaining);
            if (first_outcome) {
                guaranteed_progress = route.progress;
                candidate.planned_route = route.route;
                candidate.planned_route_steps = route.steps;
                worst_metric = route.metric;
                first_outcome = false;
            }
            else {
                for (std::size_t index = 0; index < guaranteed_progress.size(); ++index) {
                    guaranteed_progress[index] = std::min(guaranteed_progress[index], route.progress[index]);
                }
                worst_metric.battles = std::max(worst_metric.battles, route.metric.battles);
                worst_metric.processing_moves = std::max(worst_metric.processing_moves, route.metric.processing_moves);
                worst_metric.duration = std::max(worst_metric.duration, route.metric.duration);
                candidate.planned_route.clear();
                candidate.planned_route_steps.clear();
            }
        }
        for (std::size_t index = 0; index < milestones.size(); ++index) {
            candidate.milestone_progress.emplace(milestones[index].definition->id, guaranteed_progress[index]);
        }
        candidate.battle_count = worst_metric.battles;
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
    result.decision =
        executor.choose(*request.policy, policy_facts, *request.mission, *request.run, resources, policy_candidates);
    if (!result.decision.selected.has_value()) {
        result.error = result.decision.reason;
    }
    return result;
}
} // namespace asst::blackflow
