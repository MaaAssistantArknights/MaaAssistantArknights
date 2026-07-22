#include "BlackFlowSession.h"

#include "BlackFlowRoutingLoop.h"

#include <algorithm>
#include <limits>
#include <regex>
#include <tuple>
#include <unordered_map>

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

namespace asst::blackflow
{
namespace
{
FactValue default_fact_value(FactType type)
{
    switch (type) {
    case FactType::Boolean:
        return false;
    case FactType::Integer:
        return std::int64_t { 0 };
    case FactType::String:
        return std::string {};
    case FactType::StringList:
        return std::vector<std::string> {};
    }
    return false;
}

bool boolean_fact(const FactStore& facts, std::string_view name)
{
    const FactValue* value = facts.find(name);
    return value != nullptr && std::holds_alternative<bool>(*value) && std::get<bool>(*value);
}

std::int64_t integer_fact(const FactStore& facts, std::string_view name)
{
    const FactValue* value = facts.find(name);
    return value != nullptr && std::holds_alternative<std::int64_t>(*value) ? std::get<std::int64_t>(*value) : 0;
}

std::int64_t saturated_add(std::int64_t value, std::int64_t delta) noexcept
{
    if (delta > 0 && value > std::numeric_limits<std::int64_t>::max() - delta) {
        return std::numeric_limits<std::int64_t>::max();
    }
    if (delta < 0 && value < std::numeric_limits<std::int64_t>::min() - delta) {
        return std::numeric_limits<std::int64_t>::min();
    }
    return value + delta;
}

bool has_node_type(const MapSnapshot& map, NodeType type)
{
    return std::ranges::any_of(map.nodes(), [&](const auto& pair) {
        return pair.second.type == type && pair.second.progress != NodeProgress::Removed;
    });
}

std::unordered_set<NodeId>
    terminal_nodes_for(const std::string& profile, const FactStore& facts, const MapSnapshot& map)
{
    std::unordered_set<NodeId> result;
    const int floor = static_cast<int>(integer_fact(facts, "current_floor"));
    for (const auto& [id, node] : map.nodes()) {
        if (profile == "investment" && boolean_fact(facts, "unknown_presage_visited") &&
            node.type == NodeType::BattleShop) {
            result.emplace(id);
        }
        else if (profile == "burn" && floor == 3 && node.type == NodeType::BoskyPassage) {
            result.emplace(id);
        }
        else if (
            profile == "baby_animal" && floor == 3 && integer_fact(facts, "seeds") > 0 &&
            node.type == NodeType::ScrapShop) {
            result.emplace(id);
        }
        else if (profile == "ending2" && node.type == NodeType::Fate) {
            result.emplace(id);
        }
    }
    return result;
}

std::optional<NodeType> node_type_from_report(std::string_view value)
{
    if (const auto perception = BlackFlowObservationAdapter::map_node_type(value); perception.has_value()) {
        return perception;
    }
    static const std::unordered_map<std::string_view, NodeType> Mapping = {
        { "unknown", NodeType::Unknown },
        { "empty", NodeType::Empty },
        { "combat", NodeType::Combat },
        { "emergency_combat", NodeType::EmergencyCombat },
        { "boss", NodeType::Boss },
        { "battle_shop", NodeType::BattleShop },
        { "scrap_shop", NodeType::ScrapShop },
        { "encounter", NodeType::Encounter },
        { "mysterious_presage", NodeType::MysteriousPresage },
        { "ferocious_presage", NodeType::FerociousPresage },
        { "scout", NodeType::Scout },
        { "face_off", NodeType::FaceOff },
        { "emergency_aid", NodeType::EmergencyAid },
        { "rest", NodeType::Rest },
        { "feather_point", NodeType::FeatherPoint },
        { "winding_passage", NodeType::WindingPassage },
        { "sacrifice", NodeType::Sacrifice },
        { "wish", NodeType::Wish },
        { "bosky_passage", NodeType::BoskyPassage },
        { "resident_stronghold", NodeType::ResidentStronghold },
        { "final", NodeType::Final },
        { "fate", NodeType::Fate },
        { "evacuate", NodeType::Evacuate },
        { "teleporter", NodeType::Teleporter },
        { "other", NodeType::Other },
    };
    const auto found = Mapping.find(value);
    return found == Mapping.end() ? std::nullopt : std::optional<NodeType>(found->second);
}

std::string terminal_dispatch_task(const BlackFlowStrategyResult& result)
{
    if (result.outcome == "burn_completed") {
        return "BlackFlow@Roguelike@Event-BurnCompleted";
    }
    if (result.outcome == "baby_no_seed") {
        return "BlackFlow@Roguelike@Event-BabyNoSeed";
    }
    return result.succeeded ? "BlackFlow@Roguelike@Event-StrategyCompleted"
                            : "BlackFlow@Roguelike@Event-EndingPrerequisiteFailed";
}
} // namespace

json::object BlackFlowStrategyResult::to_json() const
{
    return {
        { "profile", profile },
        { "outcome", outcome },
        { "termination_reason", termination_reason },
        { "cultivated_animals", cultivated_animals },
        { "succeeded", succeeded },
        { "next_action", next_action },
    };
}

bool BlackFlowSession::initialize(std::string profile, std::string* error)
{
    const PolicyProfile* definition = BlackFlowStrategy.get_profile(profile);
    if (definition == nullptr) {
        if (error != nullptr) {
            *error = "unknown BlackFlow profile: " + profile;
        }
        return false;
    }
    auto resolved = BlackFlowStrategy.resolve_profile(profile, error);
    if (!resolved.has_value()) {
        return false;
    }

    m_profile = std::move(profile);
    m_policy = std::move(resolved);
    m_facts = FactContext {};
    for (const auto& [name, fact] : BlackFlowStrategy.facts()) {
        (void)name;
        if (!m_facts.define(fact, error)) {
            return false;
        }
    }
    m_facts.begin_run();
    for (const auto& [name, definition_value] : BlackFlowStrategy.facts()) {
        if (definition_value.scope != FactScope::Candidate &&
            !m_facts.set(definition_value.scope, name, default_fact_value(definition_value.type), error)) {
            return false;
        }
    }
    m_mission = MissionState {};
    m_map.reset();
    m_viewport.clear(0, 0);
    m_run = RunState {};
    m_unreachable_actions.clear();
    m_preferred_probe_node.reset();
    m_verified_arc.reset();
    m_transaction.reset();
    m_last_plan.reset();
    m_result.reset();
    m_persisted_image_packages = 0;
    ++m_run_revision;
    m_decision_sequence = 0;
    m_transaction_sequence = 0;
    m_artifact_sequence = 0;
    m_observed_current_node = InvalidNodeId;
    m_pending_event_node = InvalidNodeId;
    m_observation_id.clear();
    m_decision_id.clear();
    m_transaction_id.clear();
    m_telemetry_events.clear();
    m_diagnostic_requests.clear();
    return true;
}

void BlackFlowSession::reset_run()
{
    const std::string selected_profile = m_profile;
    std::string ignored;
    initialize(selected_profile, &ignored);
}

bool BlackFlowSession::configure_diagnostics(DiagnosticSettings settings, std::string* error)
{
    if (!settings.validate(error)) {
        return false;
    }
    m_diagnostics = settings;
    return true;
}

bool BlackFlowSession::set_fact(std::string_view name, FactValue value, std::string* error)
{
    const FactDefinition* definition = BlackFlowStrategy.get_fact_definition(std::string(name));
    if (definition == nullptr) {
        if (error != nullptr) {
            *error = "attempted to set unknown strategy fact: " + std::string(name);
        }
        return false;
    }
    return m_facts.set(definition->scope, std::string(name), std::move(value), error);
}

bool BlackFlowSession::synchronize_resource_facts(std::string* error)
{
    const std::vector<std::pair<std::string, std::int64_t>> values = {
        { "current_floor", m_run.floor },
        { "action_points", m_run.resources.action_points },
        { "ingots", m_run.resources.ingots },
        { "seeds", m_run.resources.seeds },
        { "sellable_scraps", m_run.resources.sellable_scraps },
        { "white_model_bird_count", m_run.resources.white_model_birds },
        { "painted_liberi_owned", m_run.resources.painted_liberi ? 1 : 0 },
        { "persistent_long_range_count", m_resources.read("persistent_long_range_movement", m_run).value_or(0) },
    };
    for (const auto& [name, value] : values) {
        if (name == "painted_liberi_owned") {
            if (!set_fact(name, value != 0, error)) {
                return false;
            }
        }
        else if (!set_fact(name, value, error)) {
            return false;
        }
    }
    return true;
}

void BlackFlowSession::refresh_mission()
{
    const auto previous = m_mission.milestones;
    m_mission.refresh(m_policy->milestones, m_run.floor, m_facts.merged());
    auto status_name = [](MilestoneStatus status) {
        switch (status) {
        case MilestoneStatus::Inactive:
            return "inactive";
        case MilestoneStatus::Available:
            return "available";
        case MilestoneStatus::Satisfied:
            return "satisfied";
        case MilestoneStatus::Missed:
            return "missed";
        case MilestoneStatus::Impossible:
            return "impossible";
        }
        return "inactive";
    };
    for (const auto& [id, status] : m_mission.milestones) {
        const auto old = previous.find(id);
        if (status == MilestoneStatus::Inactive || (old != previous.end() && old->second == status)) {
            continue;
        }
        json::object details {
            { "run_revision", m_run_revision }, { "observation_id", m_observation_id },
            { "floor", m_run.floor },           { "milestone_id", id },
            { "status", status_name(status) },
        };
        m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowMilestoneChanged", std::move(details) });
    }
}

bool BlackFlowSession::apply_observed_facts(const FactStore& facts, std::string* error)
{
    for (const auto& [name, value] : facts.values()) {
        const FactDefinition* definition = BlackFlowStrategy.get_fact_definition(name);
        if (definition == nullptr || definition->scope == FactScope::Candidate) {
            if (error != nullptr) {
                *error = "observed fact is undeclared or candidate-scoped: " + name;
            }
            return false;
        }
        if (!set_fact(name, value, error)) {
            return false;
        }
    }
    return true;
}

bool BlackFlowSession::apply_run_observation(const RunObservation& observation, std::string* error)
{
    auto assign_nonnegative = [&](const std::optional<int>& value, int& target, std::string_view name) {
        if (!value.has_value()) {
            return true;
        }
        if (*value < 0) {
            if (error != nullptr) {
                *error = std::string(name) + " cannot be negative";
            }
            return false;
        }
        target = *value;
        return true;
    };
    if (!assign_nonnegative(observation.action_points, m_run.resources.action_points, "action points") ||
        !assign_nonnegative(observation.hope, m_run.resources.hope, "hope") ||
        !assign_nonnegative(observation.ingots, m_run.resources.ingots, "ingots") ||
        !assign_nonnegative(observation.seeds, m_run.resources.seeds, "seeds") ||
        !assign_nonnegative(observation.sellable_scraps, m_run.resources.sellable_scraps, "sellable scraps") ||
        !assign_nonnegative(observation.white_model_birds, m_run.resources.white_model_birds, "white model birds")) {
        return false;
    }
    if (observation.painted_liberi.has_value()) {
        m_run.resources.painted_liberi = *observation.painted_liberi;
    }
    if (observation.movement_charges.has_value()) {
        if (std::ranges::any_of(*observation.movement_charges, [](const auto& pair) { return pair.second < 0; })) {
            if (error != nullptr) {
                *error = "movement charges cannot be negative";
            }
            return false;
        }
        m_run.resources.movement_charges = *observation.movement_charges;
    }
    if (observation.cross_floor_expired.has_value()) {
        m_run.cross_floor_expired = *observation.cross_floor_expired;
    }
    if (observation.costs.has_value()) {
        std::string validation_error;
        if (!observation.costs->validate(&validation_error)) {
            if (error != nullptr) {
                *error = validation_error;
            }
            return false;
        }
        m_run.costs = *observation.costs;
    }
    return true;
}

void BlackFlowSession::queue_map_summary(const PerceptionSummary& summary)
{
    json::object details {
        { "run_revision", m_run_revision },
        { "observation_id", summary.observation_id },
        { "map_revision", m_map.snapshot().revision },
        { "floor", summary.floor },
        { "current_node", summary.current_node },
        { "node_count", summary.node_count },
        { "confirmed_edge_count", summary.confirmed_edge_count },
        { "forced_edge_count", summary.forced_edge_count },
        { "unclassified_count", summary.unclassified_count },
        { "screenshot_us", summary.screenshot_us },
        { "recognition_us", summary.recognition_us },
        { "attempt_count", summary.attempt_count },
        { "retry_count", summary.retry_count },
    };
    Log.info(
        "BlackFlow map summary",
        "observation",
        summary.observation_id,
        "floor",
        summary.floor,
        "current",
        summary.current_node,
        "nodes",
        summary.node_count,
        "confirmed edges",
        summary.confirmed_edge_count,
        "inferred edges",
        summary.forced_edge_count,
        "unclassified",
        summary.unclassified_count,
        "attempts",
        summary.attempt_count);
    m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowMapSummary", details });

    std::vector<json::value> nodes;
    nodes.reserve(m_map.snapshot().nodes().size());
    for (const auto& [id, node] : m_map.snapshot().nodes()) {
        nodes.emplace_back(
            json::object {
                { "id", id },
                { "row", node.position.row },
                { "column", node.position.column },
                { "type", std::string(to_string(node.type)) },
                { "name", node.name },
                { "identity_revealed", node.identity_revealed },
                { "identity_state",
                  node.identity_state == NodeIdentityState::Classified
                      ? "classified"
                      : (node.identity_state == NodeIdentityState::Hidden ? "hidden" : "unclassified") },
                { "progress",
                  node.progress == NodeProgress::Active
                      ? "active"
                      : (node.progress == NodeProgress::Completed ? "completed" : "removed") },
            });
    }
    std::vector<json::value> edges;
    edges.reserve(m_map.snapshot().edges().size());
    for (const Edge& edge : m_map.snapshot().edges()) {
        edges.emplace_back(
            json::object {
                { "first", edge.first },
                { "second", edge.second },
                { "knowledge",
                  edge.knowledge == EdgeKnowledge::Confirmed
                      ? "confirmed"
                      : (edge.knowledge == EdgeKnowledge::Absent ? "absent" : "unknown") },
                { "probability", edge.evidence.probability },
                { "cnn_connected", edge.evidence.cnn_connected },
                { "forced_by_connectivity_constraint", edge.evidence.forced_by_connectivity_constraint },
                { "decision_source", edge.evidence.decision_source },
            });
    }
    json::object diagnostic = details;
    diagnostic["nodes"] = json::array(std::move(nodes));
    diagnostic["edges"] = json::array(std::move(edges));
    request_diagnostics(DiagnosticTrigger::RoutineObservation, std::move(diagnostic));
}

void BlackFlowSession::request_diagnostics(DiagnosticTrigger trigger, json::object snapshot)
{
    const bool routine = trigger == DiagnosticTrigger::RoutineObservation;
    if (routine && m_diagnostics.level == DiagnosticLevel::Normal) {
        return;
    }
    bool include_images = !routine || m_diagnostics.level == DiagnosticLevel::Full;
    if (include_images && m_persisted_image_packages >= m_diagnostics.image_package_limit) {
        include_images = false;
    }
    if (!include_images && m_diagnostics.level == DiagnosticLevel::Normal) {
        return;
    }
    const std::string artifact_id =
        "BF-A" + std::to_string(m_run_revision) + "-" + std::to_string(++m_artifact_sequence);
    snapshot["trigger"] = std::string(to_string(trigger));
    snapshot["run_revision"] = m_run_revision;
    snapshot["map_revision"] = m_map.snapshot().revision;
    m_diagnostic_requests.emplace_back(
        DiagnosticArtifactRequest {
            trigger,
            artifact_id,
            m_observation_id,
            m_decision_id,
            m_transaction_id,
            include_images,
            std::move(snapshot),
        });
    if (include_images) {
        ++m_persisted_image_packages;
    }
}

void BlackFlowSession::queue_warning(std::string code, std::string message, DiagnosticTrigger trigger)
{
    json::object details {
        { "run_revision", m_run_revision },
        { "observation_id", m_observation_id },
        { "decision_id", m_decision_id },
        { "transaction_id", m_transaction_id },
        { "map_revision", m_map.snapshot().revision },
        { "code", code },
        { "message", message },
    };
    Log.warn("BlackFlow routing warning", code, message);
    m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowRoutingWarning", details });
    request_diagnostics(trigger, std::move(details));
}

void BlackFlowSession::queue_decision()
{
    if (!m_transaction.has_value() || !m_last_plan.has_value()) {
        return;
    }
    const MoveCandidate& move = m_transaction->proposal();
    const Node* target = m_map.snapshot().find_node(move.target);
    const int cost = m_transaction->authoritative_cost();
    const int expected_after =
        action_points_after(m_run.resources.action_points, cost, move.predicted_action_point_gain);
    const int requirement = move.confirmed_action_point_requirement;
    const int margin = requirement >= UnreachableActionPointRequirement ? -UnreachableActionPointRequirement
                                                                        : m_run.resources.action_points - requirement;
    m_decision_id = "BF-D" + std::to_string(m_run_revision) + "-" + std::to_string(++m_decision_sequence);

    std::string reason_detail;
    const PolicyDecision& decision = m_last_plan->decision;
    if (!decision.decisive_rule_id.empty() && m_policy.has_value()) {
        const auto found = std::ranges::find(m_policy->rules, decision.decisive_rule_id, &PolicyRule::id);
        if (found != m_policy->rules.end()) {
            reason_detail = found->description;
        }
    }
    if (!decision.decisive_milestone_id.empty() && m_policy.has_value()) {
        const auto found = std::ranges::find(m_policy->milestones, decision.decisive_milestone_id, &Milestone::id);
        if (found != m_policy->milestones.end()) {
            reason_detail = found->description;
        }
    }
    if (reason_detail.empty()) {
        reason_detail = decision.reason;
    }

    std::vector<json::value> runners_up;
    for (const MoveCandidate& runner : decision.runners_up) {
        const Node* runner_target = m_map.snapshot().find_node(runner.target);
        runners_up.emplace_back(
            json::object {
                { "action_id", runner.action_id },
                { "target", runner.target },
                { "node_type", std::string(runner_target == nullptr ? "unknown" : to_string(runner_target->type)) },
                { "predicted_cost", runner.predicted_action_point_cost },
            });
    }
    json::object rejected;
    for (const auto& [category, count] : decision.rejection_counts) {
        rejected[category] = count;
    }

    json::object details {
        { "run_revision", m_run_revision },
        { "observation_id", m_observation_id },
        { "decision_id", m_decision_id },
        { "transaction_id", m_transaction_id },
        { "map_revision", m_map.snapshot().revision },
        { "floor", m_run.floor },
        { "source", move.source },
        { "target", move.target },
        { "landing", move.landing },
        { "node_name", target == nullptr ? std::string() : target->name },
        { "node_type", std::string(target == nullptr ? "unknown" : to_string(target->type)) },
        { "movement", std::string(to_string(move.movement)) },
        { "path_edge_count", move.path.size() },
        { "predicted_cost", move.predicted_action_point_cost },
        { "exact_cost", cost },
        { "action_points_before", m_run.resources.action_points },
        { "action_points_after", expected_after },
        { "safe_requirement", requirement },
        { "safety_margin", margin },
        { "reason_category", std::string(to_string(decision.reason_category)) },
        { "reason_detail", reason_detail },
        { "decisive_rule_id", decision.decisive_rule_id },
        { "decisive_milestone_id", decision.decisive_milestone_id },
        { "total_candidates", decision.total_candidates },
        { "eligible_candidates", decision.eligible_candidates },
        { "rejection_counts", std::move(rejected) },
        { "runners_up", json::array(std::move(runners_up)) },
        { "uses_inferred_edge", move.uses_inferred_edge },
        { "passes_unclassified", move.passes_unclassified },
        { "requires_preview_confirmation", move.requires_preview_confirmation },
    };
    Log.info(
        "BlackFlow decision",
        m_decision_id,
        "floor",
        m_run.floor,
        "target",
        move.target,
        "cost",
        cost,
        "margin",
        margin,
        "reason",
        to_string(decision.reason_category));
    m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowRoutingDecision", std::move(details) });
    if (move.uses_inferred_edge) {
        queue_warning(
            "inferred_edge_selected",
            "selected action depends on a connectivity-constraint edge",
            DiagnosticTrigger::InferredEdgeSelected);
    }
}

bool BlackFlowSession::merge_perception(
    const BlackFlowMapObservation& observation,
    const RunObservation& run,
    const FactStore& observed_facts,
    bool post_move,
    std::string* error)
{
    auto normalized = m_observation_adapter.normalize(observation, error);
    if (!normalized.has_value()) {
        return false;
    }
    const std::uint64_t previous_map_revision = m_map.snapshot().revision;
    const std::uint64_t previous_viewport_revision = m_viewport.viewport_revision();
    const NodeId previous_current_node = m_observed_current_node;
    const bool new_floor = m_run.floor != 0 && m_run.floor != normalized->map.floor;
    if (new_floor) {
        m_facts.begin_floor();
        m_unreachable_actions.clear();
        m_preferred_probe_node.reset();
        m_verified_arc.reset();
        m_transaction.reset();
    }
    else {
        m_facts.begin_page();
    }
    if (!m_map.merge(normalized->map, error)) {
        return false;
    }
    if (m_map.snapshot().find_node(normalized->current_node) == nullptr) {
        if (error != nullptr) {
            *error = "normalized current node is absent from the merged map";
        }
        return false;
    }

    m_observed_current_node = normalized->current_node;
    m_observation_id = normalized->summary.observation_id.empty()
                           ? "BF-O" + std::to_string(m_run_revision) + "-" + std::to_string(observation.sequence)
                           : normalized->summary.observation_id;
    normalized->summary.observation_id = m_observation_id;
    m_viewport.replace(std::move(normalized->viewport), m_map.snapshot().revision, normalized->viewport_revision);
    const bool routing_context_changed = !post_move && previous_map_revision != 0 &&
                                         (previous_map_revision != m_map.snapshot().revision ||
                                          previous_viewport_revision != m_viewport.viewport_revision() ||
                                          previous_current_node != normalized->current_node);
    if (routing_context_changed) {
        m_run.costs.clear_action_cost_overrides();
        m_unreachable_actions.clear();
        m_preferred_probe_node.reset();
        m_verified_arc.reset();
        if (m_transaction.has_value()) {
            m_transaction->invalidate();
            m_transaction.reset();
        }
    }
    if (!post_move) {
        m_run.floor = normalized->map.floor;
        m_run.current_node = normalized->current_node;
        RunObservation effective = run;
        if (!effective.action_points.has_value()) {
            effective.action_points = normalized->hud_action_points;
        }
        if (run.action_points.has_value() && normalized->hud_action_points.has_value() &&
            *run.action_points != *normalized->hud_action_points) {
            if (error != nullptr) {
                *error = "HUD action points conflict with the state observation";
            }
            return false;
        }
        if (!apply_run_observation(effective, error)) {
            return false;
        }
    }

    for (const auto& [node_id, node] : m_map.snapshot().nodes()) {
        if (node.identity_revealed) {
            m_run.revealed_nodes.emplace(node_id);
        }
        if (node.type == NodeType::FeatherPoint) {
            const auto statically_revealed = m_map.snapshot().nodes_within_manhattan(node_id, 1);
            m_run.revealed_nodes.insert(statically_revealed.begin(), statically_revealed.end());
        }
    }
    if (!apply_observed_facts(observed_facts, error) ||
        !set_fact("map_full_coverage", normalized->map.coverage == ObservationCoverage::FullMap, error) ||
        !set_fact("bosky_available", has_node_type(m_map.snapshot(), NodeType::BoskyPassage), error)) {
        return false;
    }
    queue_map_summary(normalized->summary);
    return true;
}

bool BlackFlowSession::update(const BlackFlowPerceptionSnapshot& snapshot, std::string* error)
{
    if (!merge_perception(snapshot.observation, snapshot.run, snapshot.observed_facts, false, error)) {
        return false;
    }
    if (!synchronize_resource_facts(error)) {
        return false;
    }

    const FactStore merged = m_facts.merged();
    refresh_mission();
    if (m_profile == "burn" && m_run.floor >= 2 && !set_fact("burn_floor1_done", true, error)) {
        return false;
    }
    if (m_profile == "burn" && m_run.floor >= 3 && !set_fact("burn_floor2_done", true, error)) {
        return false;
    }
    if (m_profile == "ending3" && m_run.floor >= 6 && !set_fact("floor6_entered", true, error)) {
        return false;
    }
    if (m_profile == "ending2" && m_run.floor >= 5) {
        const bool a = boolean_fact(merged, "sandtable_a");
        const bool b = boolean_fact(merged, "sandtable_b");
        if (!((a && b) || ((a || b) && m_run.resources.ingots >= 50))) {
            fail("ending2_prerequisite_failed", "fifth_floor_reached_without_valid_sandtable_payment");
            return true;
        }
    }
    if (m_profile == "ending3" && m_run.floor >= 5 && !boolean_fact(merged, "ending3_device")) {
        fail("ending3_prerequisite_failed", "fifth_floor_reached_without_special_device");
        return true;
    }
    if (m_profile == "burn" && m_run.floor == 3 && snapshot.observation.coverage == ObservationCoverage::FullMap &&
        !has_node_type(m_map.snapshot(), NodeType::BoskyPassage)) {
        m_result = BlackFlowStrategyResult {
            m_profile, "burn_completed", "third_floor_has_no_bosky_passage", 0, true,
        };
    }
    if (m_profile == "baby_animal" && m_run.floor == 3 && m_run.resources.seeds > 0 &&
        snapshot.observation.coverage == ObservationCoverage::FullMap &&
        !has_node_type(m_map.snapshot(), NodeType::ScrapShop)) {
        fail("baby_scrap_shop_missing", "third_floor_has_no_scrap_shop");
    }
    if (!m_result.has_value() && m_mission.viability == MissionViability::Impossible) {
        fail("mandatory_milestone_failed", "a mandatory strategy milestone was missed or became impossible");
    }
    return true;
}

BlackFlowPlan BlackFlowSession::plan(std::string* error)
{
    BlackFlowPlan result;
    if (!m_policy.has_value()) {
        result.error = "BlackFlow session has no resolved policy";
    }
    else if (m_result.has_value()) {
        result.error = "BlackFlow session has already terminated";
    }
    else {
        const FactStore merged = m_facts.merged();
        BlackFlowPlanRequest request;
        request.map = &m_map.snapshot();
        request.run = &m_run;
        request.policy = &*m_policy;
        request.facts = &merged;
        request.mission = &m_mission;
        request.strategy_terminal_nodes = terminal_nodes_for(m_profile, merged, m_map.snapshot());
        request.fate_is_safe_terminal = m_profile == "ending2";
        request.forbidden_actions = &m_unreachable_actions;
        request.verified_arc = m_verified_arc.has_value() ? &*m_verified_arc : nullptr;
        request.preferred_probe_node = m_preferred_probe_node;
        request.viewport_revision = m_viewport.viewport_revision();
        result = BlackFlowPlanner {}.plan(request);
    }
    if (!result.error.empty() && error != nullptr) {
        *error = result.error;
    }
    if (result) {
        m_last_plan = result;
    }
    return result;
}

bool BlackFlowSession::begin_transaction(const MoveCandidate& candidate, std::string* error)
{
    auto proposed = MoveTransaction::propose(candidate, m_map.snapshot(), m_viewport, error);
    if (!proposed.has_value()) {
        return false;
    }
    m_transaction = std::move(*proposed);
    m_transaction_id = "BF-T" + std::to_string(m_run_revision) + "-" + std::to_string(++m_transaction_sequence);
    return true;
}

PreviewDisposition BlackFlowSession::accept_preview(MovePreview preview, std::string* error)
{
    if (!m_transaction.has_value() || !m_transaction->record_preview(preview, error)) {
        return PreviewDisposition::Failed;
    }
    const MoveCandidate proposal = m_transaction->proposal();
    if (m_transaction->stage() == MoveTransactionStage::Cancelled) {
        m_unreachable_actions.emplace(proposal.action_id);
        for (const NodeId node_id : proposal.path) {
            const Node* node = m_map.snapshot().find_node(node_id);
            if (node != nullptr && node->identity_state == NodeIdentityState::Unclassified) {
                m_preferred_probe_node = node_id;
                break;
            }
        }
        queue_warning(
            "target_unreachable",
            "move preview reported that the selected target is currently unreachable",
            DiagnosticTrigger::RebuildConflict);
        m_transaction.reset();
        return PreviewDisposition::Replan;
    }

    bool changed = false;
    const Node* existing = proposal.target == InvalidNodeId ? nullptr : m_map.snapshot().find_node(proposal.target);
    if (proposal.controllable && existing == nullptr) {
        if (error != nullptr) {
            *error = "preview target disappeared from normalized map";
        }
        return PreviewDisposition::Failed;
    }
    if (existing != nullptr && preview.displayed_type != NodeType::Unknown) {
        const bool identity_conflict =
            existing->identity_revealed && preview.identity_revealed && existing->type != preview.displayed_type;
        if (identity_conflict) {
            queue_warning(
                "identity_conflict",
                "preview node identity conflicts with the current normalized map",
                DiagnosticTrigger::IdentityConflict);
        }
        if (existing->type != preview.displayed_type || existing->name != preview.displayed_name ||
            existing->identity_revealed != preview.identity_revealed) {
            Node updated = *existing;
            updated.type = preview.displayed_type;
            updated.event_mask = event_mask_for(preview.displayed_type);
            updated.name = preview.displayed_name;
            updated.identity_revealed = preview.identity_revealed;
            updated.identity_state =
                preview.identity_revealed ? NodeIdentityState::Classified : NodeIdentityState::Hidden;
            if (!updated.traversal.repeatable && updated.progress == NodeProgress::Active) {
                updated.traversal = default_traversal_for(updated.type);
            }
            changed = m_map.snapshot().upsert_node(std::move(updated));
            if (changed) {
                std::vector<NodeObservation> observations;
                observations.reserve(m_viewport.nodes().size());
                for (const auto& [id, observation] : m_viewport.nodes()) {
                    (void)id;
                    observations.emplace_back(observation);
                }
                m_viewport.replace(std::move(observations), m_map.snapshot().revision, m_viewport.viewport_revision());
            }
        }
    }

    if (preview.exact_action_point_cost != proposal.predicted_action_point_cost) {
        queue_warning(
            "preview_cost_changed",
            "preview action-point cost differs from the planner prediction",
            DiagnosticTrigger::PreviewCostMismatch);
        m_run.costs.action_cost_overrides.insert_or_assign(proposal.action_id, preview.exact_action_point_cost);
        if (proposal.movement == MovementKind::Walk && !proposal.path.empty() &&
            preview.exact_action_point_cost % static_cast<int>(proposal.path.size()) == 0) {
            m_run.costs.walk_cost_per_edge = preview.exact_action_point_cost / static_cast<int>(proposal.path.size());
        }
        else if (proposal.movement != MovementKind::Walk) {
            m_run.costs.movement_cost_overrides.insert_or_assign(proposal.movement, preview.exact_action_point_cost);
        }
        ++m_run.costs.revision;
        changed = true;
    }

    if (proposal.requires_preview_confirmation || proposal.probe_only) {
        m_verified_arc = VerifiedMoveArc {
            proposal.source,
            proposal.target,
            proposal.landing,
            proposal.movement,
            preview.exact_action_point_cost,
            m_map.snapshot().revision,
            m_run.costs.revision,
            m_viewport.viewport_revision(),
        };
    }
    if (proposal.probe_only) {
        m_preferred_probe_node.reset();
        m_transaction->cancel();
        m_transaction.reset();
        return PreviewDisposition::Replan;
    }
    if (changed) {
        m_transaction->invalidate();
        m_transaction.reset();
        return PreviewDisposition::Replan;
    }
    if (!set_fact("page_kind", std::string(to_string(preview.displayed_type)), error)) {
        return PreviewDisposition::Failed;
    }
    return PreviewDisposition::ReadyToCommit;
}

bool BlackFlowSession::commit(std::string* error)
{
    const bool committed = m_transaction.has_value() &&
                           m_transaction->commit(m_map.snapshot().revision, m_viewport.viewport_revision(), error);
    if (committed) {
        queue_decision();
    }
    return committed;
}

bool BlackFlowSession::apply_post_move(const BlackFlowPostMoveSnapshot& snapshot, std::string* error)
{
    if (!m_transaction.has_value() || m_transaction->stage() != MoveTransactionStage::Committed) {
        if (error != nullptr) {
            *error = "post-move observation arrived without a committed transaction";
        }
        return false;
    }
    if (!merge_perception(snapshot.observation, snapshot.run, snapshot.observed_facts, true, error)) {
        queue_warning(
            "post_move_mismatch",
            error == nullptr ? "post-move map merge failed" : *error,
            DiagnosticTrigger::PostMoveMismatch);
        return false;
    }

    MoveObservation observation = snapshot.move;
    observation.current_node = m_observed_current_node;
    observation.map_revision = m_map.snapshot().revision;
    if (snapshot.run.action_points.has_value()) {
        observation.action_points = *snapshot.run.action_points;
    }
    else if (snapshot.observation.hud_action_points.has_value()) {
        observation.action_points = *snapshot.observation.hud_action_points;
    }
    const Node* landed_node = m_map.snapshot().find_node(observation.current_node);
    if (landed_node == nullptr) {
        if (error != nullptr) {
            *error = "post-move observation landing is absent from normalized map";
        }
        queue_warning("post_move_mismatch", *error, DiagnosticTrigger::PostMoveMismatch);
        return false;
    }
    if (observation.landed_type == NodeType::Unknown) {
        observation.landed_type = landed_node->type;
    }
    else if (landed_node->identity_revealed && observation.landed_type != landed_node->type) {
        if (error != nullptr) {
            *error = "post-move node type differs from normalized map";
        }
        queue_warning("post_move_mismatch", *error, DiagnosticTrigger::PostMoveMismatch);
        return false;
    }
    if (snapshot.move.target_progress == NodeProgress::Active) {
        observation.target_progress = landed_node->progress;
    }
    if (!m_transaction->observe(observation, error) || !m_transaction->apply(m_run, error)) {
        queue_warning(
            "post_move_mismatch",
            error == nullptr ? "post-move transaction validation failed" : *error,
            DiagnosticTrigger::PostMoveMismatch);
        return false;
    }
    if (!apply_run_observation(snapshot.run, error) ||
        !set_fact("page_kind", std::string(to_string(observation.landed_type)), error)) {
        return false;
    }

    const NodeId entered_node =
        m_transaction->proposal().target == InvalidNodeId ? observation.current_node : m_transaction->proposal().target;
    m_pending_event_node = entered_node;
    if (const Node* current = m_map.snapshot().find_node(entered_node); current != nullptr) {
        Node updated = *current;
        if (m_run.visited_nodes.contains(entered_node)) {
            updated.traversal.blocks_walk = false;
            updated.traversal.blocks_vision = false;
        }
        m_map.snapshot().upsert_node(std::move(updated));
    }
    for (const auto& [node_id, node] : m_map.snapshot().nodes()) {
        if (node.identity_revealed) {
            m_run.revealed_nodes.emplace(node_id);
        }
        if (node.type == NodeType::FeatherPoint) {
            const auto statically_revealed = m_map.snapshot().nodes_within_manhattan(node_id, 1);
            m_run.revealed_nodes.insert(statically_revealed.begin(), statically_revealed.end());
        }
    }
    if (observation.landed_type == NodeType::FeatherPoint) {
        const auto entered_reveal = m_map.snapshot().nodes_within_manhattan(observation.current_node, 2);
        m_run.revealed_nodes.insert(entered_reveal.begin(), entered_reveal.end());
    }
    Log.info(
        "BlackFlowApplied",
        m_transaction_id,
        "node",
        observation.current_node,
        "action_points",
        m_run.resources.action_points);
    m_transaction.reset();
    m_verified_arc.reset();
    m_preferred_probe_node.reset();
    m_unreachable_actions.clear();
    return synchronize_resource_facts(error);
}

bool BlackFlowSession::bind_page_route(std::string* error)
{
    const auto* routes = BlackFlowStrategy.get_page_routes(m_profile);
    if (routes == nullptr || routes->empty()) {
        if (error != nullptr) {
            *error = "profile has no page route";
        }
        Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", "BlackFlow@Roguelike@NodeDispatch-Unconfigured");
        return false;
    }
    std::vector<const PageRoute*> ordered;
    for (const auto& route : *routes) {
        ordered.emplace_back(&route);
    }
    std::ranges::sort(ordered, [](const PageRoute* lhs, const PageRoute* rhs) {
        return std::tie(lhs->rank, lhs->id) < std::tie(rhs->rank, rhs->id);
    });
    const FactStore merged = m_facts.merged();
    for (const PageRoute* route : ordered) {
        if (route->when.evaluate(merged)) {
            Task.set_task_base(route->alias, route->task);
            Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
            return true;
        }
    }
    Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", "BlackFlow@Roguelike@NodeDispatch-Unconfigured");
    if (error != nullptr) {
        *error = "no page route condition matched";
    }
    return false;
}

bool BlackFlowSession::queue_node_resolution(const json::value& callback_details, std::string* error)
{
    const std::string event_name = callback_details.get("details", "blackflow_resolution", "event_name", "");
    const std::string type_text = callback_details.get("details", "blackflow_resolution", "node_type", "");
    const std::string reported_decision = callback_details.get("details", "blackflow_resolution", "decision_id", "");
    const std::string reported_transaction =
        callback_details.get("details", "blackflow_resolution", "transaction_id", "");
    const std::string progress = callback_details.get("details", "blackflow_resolution", "progress", "");
    const bool becomes_empty = callback_details.get("details", "blackflow_resolution", "becomes_empty", false);
    const bool repeatable_present = callback_details.contains("details") &&
                                    callback_details.at("details").is_object() &&
                                    callback_details.at("details").contains("blackflow_resolution") &&
                                    callback_details.at("details").at("blackflow_resolution").is_object() &&
                                    callback_details.at("details").at("blackflow_resolution").contains("repeatable");
    const bool present = !event_name.empty() || !type_text.empty() || !progress.empty() || becomes_empty ||
                         repeatable_present || !reported_decision.empty() || !reported_transaction.empty();
    if (!present) {
        return true;
    }
    const bool correlation_reported = !reported_decision.empty() || !reported_transaction.empty();
    if (correlation_reported && (reported_decision.empty() || reported_decision != m_decision_id) &&
        (reported_transaction.empty() || reported_transaction != m_transaction_id)) {
        if (error != nullptr) {
            *error = "BlackFlow node resolution does not match the pending decision or transaction";
        }
        return false;
    }
    if (m_pending_event_node == InvalidNodeId) {
        if (error != nullptr) {
            *error = "BlackFlow node resolution arrived without a pending entered node";
        }
        return false;
    }
    const Node* current = m_map.snapshot().find_node(m_pending_event_node);
    if (current == nullptr) {
        if (error != nullptr) {
            *error = "BlackFlow node resolution references a node absent from the current map";
        }
        return false;
    }

    Node updated = *current;
    if (!type_text.empty()) {
        const auto type = node_type_from_report(type_text);
        if (!type.has_value()) {
            if (error != nullptr) {
                *error = "BlackFlow node resolution contains an unknown node type: " + type_text;
            }
            return false;
        }
        updated.type = *type;
        updated.event_mask = event_mask_for(*type);
        updated.identity_state = NodeIdentityState::Classified;
        updated.identity_revealed = true;
    }
    if (!event_name.empty()) {
        updated.name = event_name;
    }
    if (!progress.empty()) {
        if (progress == "active") {
            updated.progress = NodeProgress::Active;
        }
        else if (progress == "completed") {
            updated.progress = NodeProgress::Completed;
        }
        else if (progress == "removed") {
            updated.progress = NodeProgress::Removed;
        }
        else {
            if (error != nullptr) {
                *error = "BlackFlow node resolution contains an unknown progress value: " + progress;
            }
            return false;
        }
    }
    const bool repeatable =
        callback_details.get("details", "blackflow_resolution", "repeatable", updated.traversal.repeatable);
    updated.traversal.repeatable = repeatable;
    if (updated.progress == NodeProgress::Completed || becomes_empty || m_run.visited_nodes.contains(updated.id)) {
        updated.traversal.blocks_walk = false;
        updated.traversal.blocks_vision = false;
    }
    if (becomes_empty) {
        updated.type = NodeType::Empty;
        updated.event_mask = event_mask_for(NodeType::Empty);
        updated.progress = NodeProgress::Active;
        updated.traversal = default_traversal_for(NodeType::Empty);
        updated.identity_state = NodeIdentityState::Classified;
        updated.identity_revealed = true;
        updated.teleport_target.reset();
    }
    m_run.node_progress.insert_or_assign(m_pending_event_node, updated.progress);
    m_run.visited_nodes.emplace(m_pending_event_node);
    m_map.snapshot().upsert_node(updated);

    json::object details {
        { "run_revision", m_run_revision },
        { "observation_id", m_observation_id },
        { "decision_id", m_decision_id },
        { "transaction_id", m_transaction_id },
        { "node", m_pending_event_node },
        { "event_name", event_name },
        { "node_type", std::string(to_string(updated.type)) },
        { "progress", progress },
        { "repeatable", repeatable },
        { "becomes_empty", becomes_empty },
    };
    Log.info(
        "BlackFlow node resolution",
        "node",
        m_pending_event_node,
        "event",
        event_name,
        "type",
        to_string(updated.type),
        "progress",
        progress);
    m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowNodeResolution", std::move(details) });
    return true;
}

bool BlackFlowSession::apply_event_effect(
    const TaskEventEffect& effect,
    const json::value& callback_details,
    std::string* error)
{
    if (effect.kind == TaskEventEffectKind::Set) {
        return set_fact(effect.fact, *effect.value, error);
    }
    if (effect.kind == TaskEventEffectKind::Add) {
        const FactValue* current = m_facts.find(effect.fact);
        if (current == nullptr || !std::holds_alternative<std::int64_t>(*current)) {
            if (error != nullptr) {
                *error = "add effect target is not an initialized integer fact";
            }
            return false;
        }
        const auto delta = std::get<std::int64_t>(*effect.value);
        return set_fact(effect.fact, saturated_add(std::get<std::int64_t>(*current), delta), error);
    }

    const std::string text = callback_details.get("details", "result", "text", "");
    std::smatch match;
    if (!std::regex_search(text, match, std::regex(R"([-+]?\d+)"))) {
        if (error != nullptr) {
            *error = "capture_int task event found no integer in details.result.text";
        }
        return false;
    }
    std::int64_t parsed = 0;
    try {
        parsed = std::stoll(match.str());
    }
    catch (const std::exception&) {
        if (error != nullptr) {
            *error = "capture_int task event integer is outside the supported range";
        }
        return false;
    }
    parsed = std::clamp<std::int64_t>(parsed, effect.minimum, effect.maximum);
    return set_fact(effect.fact, parsed, error);
}

bool BlackFlowSession::apply_task_event(const json::value& callback_details, std::string* error)
{
    const std::string task = callback_details.get("details", "task", "");
    const TaskEvent* event = BlackFlowStrategy.get_task_event(task);
    if (event == nullptr) {
        if (error != nullptr) {
            *error = "completed task has no BlackFlow task event: " + task;
        }
        return false;
    }
    if (!queue_node_resolution(callback_details, error)) {
        return false;
    }
    for (const auto& effect : event->effects) {
        if (!apply_event_effect(effect, callback_details, error)) {
            return false;
        }
    }
    refresh_mission();
    m_pending_event_node = InvalidNodeId;
    if (event->terminate) {
        const int cultivated = static_cast<int>(integer_fact(m_facts.merged(), "cultivated_animals"));
        m_result = BlackFlowStrategyResult {
            m_profile,
            event->outcome_code,
            event->termination_reason,
            std::clamp(cultivated, 0, 3),
            event->outcome_code != "ending_prerequisite_failed" && event->outcome_code != "page_recovery_failed" &&
                event->outcome_code != "baby_no_seed",
        };
        if (!m_result->succeeded && m_policy.has_value()) {
            m_result->next_action = m_policy->failure_action;
        }
    }
    return true;
}

void BlackFlowSession::fail(std::string outcome, std::string reason)
{
    if (outcome == "map_rebuild_failed") {
        queue_warning("map_rebuild_failed", reason, DiagnosticTrigger::MapRebuildFailed);
    }
    else if (outcome == "page_recovery_failed") {
        queue_warning("page_recovery_failed", reason, DiagnosticTrigger::PageRecoveryFailed);
    }
    m_result = BlackFlowStrategyResult {
        m_profile,
        std::move(outcome),
        std::move(reason),
        static_cast<int>(std::clamp<std::int64_t>(integer_fact(m_facts.merged(), "cultivated_animals"), 0, 3)),
        false,
    };
    if (m_policy.has_value()) {
        m_result->next_action = m_policy->failure_action;
    }
}

std::vector<BlackFlowTelemetryEvent> BlackFlowSession::take_telemetry_events()
{
    std::vector<BlackFlowTelemetryEvent> result;
    result.swap(m_telemetry_events);
    return result;
}

std::vector<DiagnosticArtifactRequest> BlackFlowSession::take_diagnostic_requests()
{
    std::vector<DiagnosticArtifactRequest> result;
    result.swap(m_diagnostic_requests);
    return result;
}

BlackFlowTaskPlugin::BlackFlowTaskPlugin(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    const std::shared_ptr<RoguelikeConfig>& config,
    const std::shared_ptr<RoguelikeControlTaskPlugin>& control,
    std::shared_ptr<BlackFlowSession> session,
    std::shared_ptr<IBlackFlowTaskPort> port) :
    AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config, control),
    m_session(std::move(session)),
    m_port(std::move(port))
{
    set_block(true);
}

bool BlackFlowTaskPlugin::load_params(const json::value& params)
{
    if (m_config->get_theme() != RoguelikeTheme::BlackFlow || m_session == nullptr) {
        return false;
    }
    std::string error;
    const std::string profile = params.get("blackflow_strategy", std::string("ending1"));
    if (!m_session->initialize(profile, &error)) {
        Log.error("BlackFlow strategy initialization failed:", error);
        return false;
    }
    const std::string diagnostics_text = params.get("blackflow_diagnostics", std::string("normal"));
    const auto diagnostics = parse_diagnostic_level(diagnostics_text);
    const int image_limit = params.get("blackflow_diagnostic_image_limit", 3);
    if (!diagnostics.has_value() || image_limit < 0 || image_limit > 100) {
        Log.error("Invalid BlackFlow diagnostics parameters");
        return false;
    }
    DiagnosticSettings settings { *diagnostics, static_cast<std::size_t>(image_limit) };
    if (!m_session->configure_diagnostics(settings, &error)) {
        Log.error("BlackFlow diagnostics initialization failed:", error);
        return false;
    }
    if (m_port != nullptr) {
        m_port->configure_diagnostics(settings);
    }
    return true;
}

bool BlackFlowTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }
    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart && task == "BlackFlow@Roguelike@RoutingAction") {
        m_pending = PendingWork::Routing;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == "BlackFlow@Roguelike@PageFailure") {
        m_pending = PendingWork::RecoverMap;
        m_pending_details = details;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && BlackFlowStrategy.get_task_event(task) != nullptr) {
        m_pending = PendingWork::TaskEvent;
        m_pending_details = details;
        return true;
    }
    return false;
}

void BlackFlowTaskPlugin::reset_in_run_variables()
{
    if (m_session != nullptr) {
        m_session->reset_run();
    }
    m_pending = PendingWork::None;
    m_pending_details = json::value {};
    m_reported = false;
}

void BlackFlowTaskPlugin::report_result()
{
    if (m_reported || m_session == nullptr || !m_session->result().has_value()) {
        return;
    }
    auto info = basic_info_with_what("BlackFlowStrategyResult");
    info["details"] = m_session->result()->to_json();
    callback(AsstMsg::SubTaskExtraInfo, info);
    m_reported = true;
}

void BlackFlowTaskPlugin::report_telemetry()
{
    if (m_session == nullptr) {
        return;
    }
    for (auto& event : m_session->take_telemetry_events()) {
        auto info = basic_info_with_what(event.what);
        info["details"] = std::move(event.details);
        callback(AsstMsg::SubTaskExtraInfo, info);
    }
}

void BlackFlowTaskPlugin::persist_diagnostics()
{
    if (m_session == nullptr || m_port == nullptr) {
        return;
    }
    for (const auto& request : m_session->take_diagnostic_requests()) {
        std::string error;
        if (!m_port->persist_diagnostics(request, &error)) {
            Log.warn("BlackFlow diagnostic artifact persistence failed:", error);
        }
    }
}

bool BlackFlowTaskPlugin::_run()
{
    LogTraceFunction;
    if (m_session == nullptr) {
        Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", "BlackFlow@Roguelike@RecoveryFailed");
        m_pending = PendingWork::None;
        return true;
    }

    const PendingWork work = m_pending;
    m_pending = PendingWork::None;
    if (work == PendingWork::TaskEvent) {
        std::string error;
        if (!execute_routing_task_event(*m_session, m_pending_details, &error)) {
            m_session->fail("task_event_failed", error);
        }
        report_telemetry();
        persist_diagnostics();
        report_result();
        return true;
    }

    if (work == PendingWork::RecoverMap) {
        std::string error;
        const bool recovered = m_port != nullptr && m_port->recover_map(&error);
        Task.set_task_base(
            "BlackFlow@Roguelike@RecoverMap",
            recovered ? "BlackFlow@Roguelike@RoutingAction" : "BlackFlow@Roguelike@RecoveryFailed");
        if (!recovered) {
            m_session->fail("page_recovery_failed", error.empty() ? "map recovery port is unavailable" : error);
            report_result();
        }
        report_telemetry();
        persist_diagnostics();
        return true;
    }

    if (work != PendingWork::Routing) {
        return true;
    }
    if (m_port == nullptr) {
        m_session->fail("perception_port_missing", "BlackFlow perception and task port is not attached");
        Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", "BlackFlow@Roguelike@RecoveryFailed");
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
        report_telemetry();
        persist_diagnostics();
        report_result();
        return true;
    }

    const RoutingCycleOutcome cycle = execute_routing_cycle(*m_session, *m_port);
    report_telemetry();
    persist_diagnostics();
    if (cycle.status == RoutingCycleStatus::SessionTerminated) {
        Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", terminal_dispatch_task(*m_session->result()));
        Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
        report_result();
        return true;
    }
    if (cycle.status == RoutingCycleStatus::MovedToNode) {
        return true;
    }

    m_session->fail(cycle.failure_code, cycle.error);
    report_telemetry();
    persist_diagnostics();
    Task.set_task_base("BlackFlow@Roguelike@NodeDispatch", "BlackFlow@Roguelike@RecoveryFailed");
    Task.set_task_base("BlackFlow@Roguelike@RoutingAction", "BlackFlow@Roguelike@NodeDispatch");
    report_result();
    return true;
}
} // namespace asst::blackflow
