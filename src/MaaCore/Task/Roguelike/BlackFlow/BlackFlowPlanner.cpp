#include "BlackFlowPlanner.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <queue>
#include <tuple>
#include <unordered_map>

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
        const std::int64_t outcome_requirement =
            static_cast<std::int64_t>(successor) + action.action_point_cost - outcome.action_point_gain;
        requirement = std::max(requirement, outcome_requirement);
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
    bool has_badged_encounter = false;
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
            result.has_badged_encounter =
                result.has_badged_encounter || (node->badged && node->type == NodeType::Encounter);
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

bool combat_type(NodeType type)
{
    return type == NodeType::Combat || type == NodeType::EmergencyCombat || type == NodeType::Boss;
}

bool boolean_fact(const FactStore& facts, std::string_view name)
{
    const FactValue* value = facts.find(name);
    return value != nullptr && std::holds_alternative<bool>(*value) && std::get<bool>(*value);
}

struct RouteMetric
{
    int battles = std::numeric_limits<int>::max();
    int duration = std::numeric_limits<int>::max();

    [[nodiscard]] bool reachable() const noexcept { return battles != std::numeric_limits<int>::max(); }
};

bool metric_less(const RouteMetric& lhs, const RouteMetric& rhs) noexcept
{
    return std::tie(lhs.battles, lhs.duration) < std::tie(rhs.battles, rhs.duration);
}

RouteMetric add_metric(RouteMetric lhs, RouteMetric rhs) noexcept
{
    if (!lhs.reachable() || !rhs.reachable()) {
        return {};
    }
    const auto saturated = [](int first, int second) {
        return static_cast<int>(
            std::min<std::int64_t>(static_cast<std::int64_t>(first) + second, std::numeric_limits<int>::max()));
    };
    return { saturated(lhs.battles, rhs.battles), saturated(lhs.duration, rhs.duration) };
}

RouteMetric move_metric(const MapSnapshot& map, const MoveCandidate& move)
{
    const bool battle = std::ranges::any_of(move.possible_landings, [&](NodeId landing) {
        const Node* node = map.find_node(landing);
        return node != nullptr && combat_type(node->type);
    });
    const int duration = move.movement == MovementKind::Walk ? std::max(1, static_cast<int>(move.path.size())) : 1;
    return { battle ? 1 : 0, duration };
}

RouteMetric shortest_confirmed_metric(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    const SafetySolution& solution,
    SafetyStateId initial,
    int initial_action_points,
    NodeType target_type)
{
    struct Label
    {
        SafetyStateId state = 0;
        int action_points = 0;
        RouteMetric metric;
    };

    struct Later
    {
        bool operator()(const Label& lhs, const Label& rhs) const noexcept
        {
            return metric_less(rhs.metric, lhs.metric);
        }
    };

    std::unordered_map<SafetyStateId, std::vector<Label>> labels;
    std::unordered_map<SafetyStateId, std::vector<const SafetyAction*>> actions;
    for (const auto& action : expanded.problem.actions) {
        actions[action.source].emplace_back(&action);
    }

    std::priority_queue<Label, std::vector<Label>, Later> pending;
    Label initial_label { initial, initial_action_points, { 0, 0 } };
    labels[initial].emplace_back(initial_label);
    pending.emplace(initial_label);

    while (!pending.empty()) {
        const Label current = pending.top();
        pending.pop();
        if (current.state >= expanded.planner_states.size()) {
            continue;
        }
        const Node* current_node = map.find_node(expanded.planner_states[current.state].node);
        if (current_node != nullptr && current_node->type == target_type) {
            return current.metric;
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

            Label next {
                outcome.successor,
                remaining,
                add_metric(current.metric, move_metric(map, move->second)),
            };
            auto& existing = labels[next.state];
            const bool dominated = std::ranges::any_of(existing, [&](const Label& value) {
                return value.action_points >= next.action_points && !metric_less(next.metric, value.metric);
            });
            if (dominated) {
                continue;
            }
            std::erase_if(existing, [&](const Label& value) {
                return next.action_points >= value.action_points && !metric_less(value.metric, next.metric);
            });
            existing.emplace_back(next);
            pending.emplace(next);
        }
    }
    return {};
}

RouteMetric confirmed_metric_after_root(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& expanded,
    const SafetySolution& solution,
    const SafetyAction& root,
    int current_action_points,
    NodeType target_type)
{
    const auto move = expanded.action_candidates.find(root.id);
    if (move == expanded.action_candidates.end()) {
        return {};
    }
    const RouteMetric first = move_metric(map, move->second);
    RouteMetric worst { 0, 0 };
    for (const auto& outcome : root.outcomes) {
        const int remaining =
            action_points_after(current_action_points, root.action_point_cost, outcome.action_point_gain);
        const RouteMetric suffix =
            shortest_confirmed_metric(map, expanded, solution, outcome.successor, remaining, target_type);
        const RouteMetric total = add_metric(first, suffix);
        if (!total.reachable()) {
            return {};
        }
        if (metric_less(worst, total)) {
            worst = total;
        }
    }
    return worst;
}

struct HypotheticalSuffix
{
    int full_requirement = UnreachableActionPointRequirement;
    ReachableFeatures possible;
    std::optional<ReachableFeatures> guaranteed;
};

RunState run_after_candidate(const MapSnapshot& map, const RunState& run, const MoveCandidate& move, NodeId landing)
{
    RunState next = run;
    next.current_node = landing;
    if (move.movement != MovementKind::Walk) {
        auto charge = next.resources.movement_charges.find(move.movement);
        if (charge != next.resources.movement_charges.end() && charge->second > 0) {
            --charge->second;
        }
    }
    NodeId entered = move.target == InvalidNodeId ? landing : move.target;
    next.visited_nodes.emplace(entered);
    const Node* node = map.find_node(entered);
    if (node != nullptr && node->type != NodeType::Empty && !node->traversal.repeatable) {
        next.node_progress.insert_or_assign(entered, NodeProgress::Completed);
    }
    if (node != nullptr && node->type == NodeType::FeatherPoint && !next.consumed_one_time_nodes.contains(entered)) {
        next.consumed_one_time_nodes.emplace(entered);
        const auto revealed = map.nodes_within_manhattan(entered, 2);
        next.revealed_nodes.insert(revealed.begin(), revealed.end());
    }
    return next;
}

std::optional<HypotheticalSuffix> analyze_relaxed_root(
    const MapSnapshot& map,
    const RunState& run,
    MoveCandidate move,
    const StateExpansionOptions& confirmed_options)
{
    HypotheticalSuffix result;
    result.full_requirement = std::max(1, move.predicted_action_point_cost);
    std::vector<NodeId> landings = move.possible_landings;
    if (landings.empty() && move.landing != InvalidNodeId) {
        landings.emplace_back(move.landing);
    }
    if (landings.empty()) {
        return std::nullopt;
    }

    for (const NodeId landing : landings) {
        RunState next = run_after_candidate(map, run, move, landing);
        int gain = move.predicted_action_point_gain;
        if (const auto found = move.landing_action_point_gains.find(landing);
            found != move.landing_action_point_gains.end()) {
            gain = found->second;
        }
        const int remaining = action_points_after(run.resources.action_points, move.predicted_action_point_cost, gain);
        BlackFlowStateExpander expander;
        std::string ignored;
        auto expanded = expander.build(map, next, confirmed_options, &ignored);
        if (!expanded.has_value()) {
            return std::nullopt;
        }
        auto solution_result = SafetyPlanner {}.solve(expanded->problem);
        if (!solution_result) {
            return std::nullopt;
        }
        const int suffix_requirement = solution_result.solution->requirement(expanded->initial_state);
        if (suffix_requirement >= UnreachableActionPointRequirement) {
            return std::nullopt;
        }
        const std::int64_t full =
            static_cast<std::int64_t>(suffix_requirement) + move.predicted_action_point_cost - gain;
        result.full_requirement = std::max(
            result.full_requirement,
            static_cast<int>(std::clamp<std::int64_t>(full, 0, UnreachableActionPointRequirement)));

        ReachableFeatures features =
            reachable_features(map, *expanded, *solution_result.solution, expanded->initial_state, remaining);
        result.possible.node_types.insert(features.node_types.begin(), features.node_types.end());
        result.possible.has_badged = result.possible.has_badged || features.has_badged;
        result.possible.has_badged_encounter = result.possible.has_badged_encounter || features.has_badged_encounter;
        if (!result.guaranteed.has_value()) {
            result.guaranteed = features;
        }
        else {
            std::erase_if(result.guaranteed->node_types, [&](const std::string& type) {
                return !features.node_types.contains(type);
            });
            result.guaranteed->has_badged = result.guaranteed->has_badged && features.has_badged;
            result.guaranteed->has_badged_encounter =
                result.guaranteed->has_badged_encounter && features.has_badged_encounter;
        }
    }
    return result;
}

FactStore relaxed_candidate_facts(const MapSnapshot& map, const MoveCandidate& move, const HypotheticalSuffix& suffix)
{
    FactStore facts;
    const Node* target = map.find_node(move.target);
    facts.set("candidate.node_type", std::string(target == nullptr ? "unknown" : to_string(target->type)));
    facts.set("candidate.node_name", target == nullptr ? std::string() : target->name);
    facts.set("candidate.badged", target != nullptr && target->badged);
    const bool combat = target != nullptr && combat_type(target->type);
    facts.set("candidate.combat", combat);
    facts.set("candidate.boss", combat && target->type == NodeType::Boss);
    facts.set(
        "candidate.exit",
        target != nullptr && (target->type == NodeType::Final || target->type == NodeType::Fate));
    facts.set("candidate.uses_processing_item", move.movement != MovementKind::Walk);
    facts.set(
        "candidate.feather_reveal_count",
        static_cast<std::int64_t>(
            target != nullptr && target->type == NodeType::FeatherPoint ? unknown_big_nodes_revealed(map, target->id)
                                                                        : 0));
    const ReachableFeatures guaranteed = suffix.guaranteed.value_or(ReachableFeatures {});
    facts.set("candidate.route_node_types", sorted_types(suffix.possible.node_types));
    facts.set("candidate.guaranteed_route_node_types", sorted_types(guaranteed.node_types));
    facts.set("candidate.route_has_badged", suffix.possible.has_badged);
    facts.set("candidate.guaranteed_route_has_badged", guaranteed.has_badged);
    facts.set("candidate.route_has_badged_encounter", suffix.possible.has_badged_encounter);
    facts.set("candidate.guaranteed_route_has_badged_encounter", guaranteed.has_badged_encounter);
    return facts;
}

} // namespace

FactStore BlackFlowPlanner::candidate_facts(
    const MapSnapshot& map,
    const ExpandedSafetyProblem& confirmed,
    const SafetySolution& solution,
    const SafetyAction& root_action,
    int current_action_points) const
{
    FactStore facts;
    const auto move_iter = confirmed.action_candidates.find(root_action.id);
    if (move_iter == confirmed.action_candidates.end()) {
        return facts;
    }
    const MoveCandidate& move = move_iter->second;
    const Node* target = map.find_node(move.target);
    facts.set("candidate.node_type", std::string(target == nullptr ? "unknown" : to_string(target->type)));
    facts.set("candidate.node_name", target == nullptr ? std::string() : target->name);
    facts.set("candidate.badged", target != nullptr && target->badged);
    bool enters_combat = false;
    bool combat_landings_are_bosses = true;
    for (const NodeId landing : move.possible_landings) {
        const Node* landing_node = map.find_node(landing);
        if (landing_node != nullptr && combat_type(landing_node->type)) {
            enters_combat = true;
            combat_landings_are_bosses = combat_landings_are_bosses && landing_node->type == NodeType::Boss;
        }
    }
    if (move.possible_landings.empty() && target != nullptr && combat_type(target->type)) {
        enters_combat = true;
        combat_landings_are_bosses = target->type == NodeType::Boss;
    }
    facts.set("candidate.combat", enters_combat);
    facts.set("candidate.boss", enters_combat && combat_landings_are_bosses);
    facts.set(
        "candidate.exit",
        target != nullptr && (target->type == NodeType::Final || target->type == NodeType::Fate));
    facts.set("candidate.uses_processing_item", move.movement != MovementKind::Walk);
    facts.set(
        "candidate.feather_reveal_count",
        static_cast<std::int64_t>(
            target != nullptr && target->type == NodeType::FeatherPoint ? unknown_big_nodes_revealed(map, target->id)
                                                                        : 0));

    ReachableFeatures union_features;
    std::optional<ReachableFeatures> guaranteed_features;
    for (const auto& outcome : root_action.outcomes) {
        const int remaining =
            action_points_after(current_action_points, root_action.action_point_cost, outcome.action_point_gain);
        auto features = reachable_features(map, confirmed, solution, outcome.successor, remaining);
        union_features.node_types.insert(features.node_types.begin(), features.node_types.end());
        union_features.has_badged = union_features.has_badged || features.has_badged;
        union_features.has_badged_encounter = union_features.has_badged_encounter || features.has_badged_encounter;
        if (!guaranteed_features.has_value()) {
            guaranteed_features = std::move(features);
        }
        else {
            std::erase_if(guaranteed_features->node_types, [&](const std::string& type) {
                return !features.node_types.contains(type);
            });
            guaranteed_features->has_badged = guaranteed_features->has_badged && features.has_badged;
            guaranteed_features->has_badged_encounter =
                guaranteed_features->has_badged_encounter && features.has_badged_encounter;
        }
    }
    const ReachableFeatures guaranteed = guaranteed_features.value_or(ReachableFeatures {});
    facts.set("candidate.route_node_types", sorted_types(union_features.node_types));
    facts.set("candidate.guaranteed_route_node_types", sorted_types(guaranteed.node_types));
    facts.set("candidate.route_has_badged", union_features.has_badged);
    facts.set("candidate.guaranteed_route_has_badged", guaranteed.has_badged);
    facts.set("candidate.route_has_badged_encounter", union_features.has_badged_encounter);
    facts.set("candidate.guaranteed_route_has_badged_encounter", guaranteed.has_badged_encounter);
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

    StateExpansionOptions confirmed_options;
    confirmed_options.knowledge = MapKnowledgeMode::Confirmed;
    confirmed_options.strategy_terminal_nodes = request.strategy_terminal_nodes;
    if (request.forbidden_actions != nullptr) {
        confirmed_options.forbidden_action_ids = *request.forbidden_actions;
    }
    confirmed_options.fate_is_terminal = request.fate_is_safe_terminal;
    confirmed_options.maximum_states = request.maximum_states;

    StateExpansionOptions relaxed_options = confirmed_options;
    relaxed_options.knowledge = MapKnowledgeMode::Relaxed;

    BlackFlowStateExpander expander;
    std::string error;
    auto confirmed = expander.build(*request.map, *request.run, confirmed_options, &error);
    if (!confirmed.has_value()) {
        result.error = "confirmed state expansion failed: " + error;
        return result;
    }
    auto relaxed = expander.build(*request.map, *request.run, relaxed_options, &error);
    if (!relaxed.has_value()) {
        result.error = "relaxed state expansion failed: " + error;
        return result;
    }

    SafetyPlanner safety_planner;
    auto bounds = safety_planner.solve_bounds(
        relaxed->problem,
        confirmed->problem,
        relaxed->initial_state,
        confirmed->initial_state,
        &error);
    if (!bounds.has_value()) {
        result.error = "safety bounds failed: " + error;
        return result;
    }
    result.safety = std::move(*bounds);
    if (result.safety.confirmed_first_action.has_value()) {
        const auto action = confirmed->action_candidates.find(*result.safety.confirmed_first_action);
        if (action != confirmed->action_candidates.end()) {
            result.confirmed_escape_first_action = action->second;
        }
    }

    std::vector<PolicyCandidate> policy_candidates;
    for (const auto& action : confirmed->problem.actions) {
        if (action.source != confirmed->initial_state) {
            continue;
        }
        const auto move = confirmed->action_candidates.find(action.id);
        if (move == confirmed->action_candidates.end()) {
            continue;
        }
        PolicyCandidate candidate;
        candidate.move = move->second;
        candidate.move.confirmed_action_point_requirement =
            action_requirement(action, result.safety.confirmed_solution);
        candidate.confirmed_safe =
            request.run->resources.action_points >= candidate.move.confirmed_action_point_requirement;
        candidate.facts = candidate_facts(
            *request.map,
            *confirmed,
            result.safety.confirmed_solution,
            action,
            request.run->resources.action_points);
        const Node* target = request.map->find_node(candidate.move.target);
        candidate.battle_count = target != nullptr && combat_type(target->type) ? 1 : 0;
        candidate.estimated_duration =
            candidate.move.movement == MovementKind::Walk ? static_cast<int>(candidate.move.path.size()) : 1;
        const auto route_types = candidate.facts.find("candidate.route_node_types");
        candidate.development_score =
            route_types != nullptr && std::holds_alternative<std::vector<std::string>>(*route_types)
                ? -static_cast<int>(std::get<std::vector<std::string>>(*route_types).size())
                : 0;
        candidate.risk_score = target == nullptr || !target->identity_revealed
                                   ? 5
                                   : (target->type == NodeType::EmergencyCombat || target->type == NodeType::Boss
                                          ? 10
                                          : candidate.battle_count * 4);
        if (request.policy->profile_id == "investment") {
            const NodeType route_target = boolean_fact(*request.facts, "unknown_presage_visited")
                                              ? NodeType::BattleShop
                                              : NodeType::MysteriousPresage;
            const RouteMetric metric = confirmed_metric_after_root(
                *request.map,
                *confirmed,
                result.safety.confirmed_solution,
                action,
                request.run->resources.action_points,
                route_target);
            candidate.battle_count = metric.reachable() ? metric.battles : std::numeric_limits<int>::max();
            candidate.estimated_duration = metric.reachable() ? metric.duration : std::numeric_limits<int>::max();
            candidate.development_score = 0;
            candidate.risk_score = 0;
        }
        policy_candidates.emplace_back(std::move(candidate));
    }

    std::unordered_set<std::string> confirmed_root_actions;
    for (const auto& candidate : policy_candidates) {
        confirmed_root_actions.emplace(candidate.move.action_id);
    }
    std::optional<MoveCandidate> preferred_probe;
    for (const auto& action : relaxed->problem.actions) {
        if (action.source != relaxed->initial_state) {
            continue;
        }
        const auto move_iter = relaxed->action_candidates.find(action.id);
        if (move_iter == relaxed->action_candidates.end() ||
            confirmed_root_actions.contains(move_iter->second.action_id)) {
            continue;
        }
        MoveCandidate move = move_iter->second;
        move.requires_preview_confirmation = true;
        if (request.verified_arc != nullptr &&
            request.verified_arc
                ->matches(move, request.map->revision, request.run->costs.revision, request.viewport_revision)) {
            move.predicted_action_point_cost = request.verified_arc->exact_action_point_cost;
            move.requires_preview_confirmation = false;
        }
        auto suffix = analyze_relaxed_root(*request.map, *request.run, move, confirmed_options);
        if (!suffix.has_value()) {
            continue;
        }
        move.confirmed_action_point_requirement = suffix->full_requirement;
        PolicyCandidate candidate;
        candidate.move = move;
        candidate.confirmed_safe = request.run->resources.action_points >= suffix->full_requirement;
        candidate.facts = relaxed_candidate_facts(*request.map, move, *suffix);
        const Node* target = request.map->find_node(move.target);
        candidate.battle_count = target != nullptr && combat_type(target->type) ? 1 : 0;
        candidate.estimated_duration =
            move.movement == MovementKind::Walk ? std::max(1, static_cast<int>(move.path.size())) : 1;
        candidate.development_score = -static_cast<int>(suffix->possible.node_types.size());
        candidate.risk_score = target == nullptr || !target->identity_revealed
                                   ? 5
                                   : (target->type == NodeType::EmergencyCombat || target->type == NodeType::Boss
                                          ? 10
                                          : candidate.battle_count * 4);
        if (request.preferred_probe_node.has_value() && move.target == *request.preferred_probe_node &&
            candidate.confirmed_safe) {
            move.probe_only = true;
            preferred_probe = move;
        }
        policy_candidates.emplace_back(std::move(candidate));
    }

    if (request.preferred_probe_node.has_value()) {
        if (!preferred_probe.has_value()) {
            result.error = "no safe preview action reaches the requested unclassified frontier";
            return result;
        }
        result.decision.selected = *preferred_probe;
        result.decision.total_candidates = policy_candidates.size();
        result.decision.eligible_candidates = 1;
        result.decision.reason_category = DecisionReasonCategory::SafetyFallback;
        result.decision.decisive_rule_id = "unclassified_frontier_probe";
        result.decision.reason = "selected unclassified frontier probe";
        result.selected_requires_probe = true;
        return result;
    }

    const bool has_safe_noncombat_alternative =
        std::ranges::any_of(policy_candidates, [](const PolicyCandidate& candidate) {
            return candidate.confirmed_safe && !boolean_fact(candidate.facts, "candidate.combat");
        });
    for (auto& candidate : policy_candidates) {
        candidate.facts.set(
            "candidate.combat_is_optional",
            candidate.confirmed_safe && boolean_fact(candidate.facts, "candidate.combat") &&
                has_safe_noncombat_alternative);
    }

    bool has_safe_direct_exit = false;
    bool has_safe_non_exit = false;
    for (const auto& candidate : policy_candidates) {
        if (!candidate.confirmed_safe) {
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
    else {
        result.selected_requires_probe = result.decision.selected->probe_only;
    }
    return result;
}
} // namespace asst::blackflow

