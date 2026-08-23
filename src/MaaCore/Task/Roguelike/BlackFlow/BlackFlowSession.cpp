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
std::string_view to_string(CultivatedAnimalType type) noexcept
{
    switch (type) {
    case CultivatedAnimalType::Cat:
        return "swaddled_cat";
    case CultivatedAnimalType::FeatheredSerpent:
        return "swaddled_feathered_serpent";
    case CultivatedAnimalType::Dog:
        return "swaddled_dog";
    case CultivatedAnimalType::Cerberus:
        return "swaddled_cerberus";
    }
    return "swaddled_cat";
}

std::optional<CultivatedAnimalType> parse_cultivated_animal_type(std::string_view value) noexcept
{
    if (value == "swaddled_cat") {
        return CultivatedAnimalType::Cat;
    }
    if (value == "swaddled_feathered_serpent") {
        return CultivatedAnimalType::FeatheredSerpent;
    }
    if (value == "swaddled_dog") {
        return CultivatedAnimalType::Dog;
    }
    if (value == "swaddled_cerberus") {
        return CultivatedAnimalType::Cerberus;
    }
    return std::nullopt;
}

std::optional<CultivatedAnimalType> cultivated_animal_type_from_name(std::string_view name) noexcept
{
    if (name == "襁褓中的猫") {
        return CultivatedAnimalType::Cat;
    }
    if (name == "襁褓羽蛇") {
        return CultivatedAnimalType::FeatheredSerpent;
    }
    if (name == "襁褓中的狗") {
        return CultivatedAnimalType::Dog;
    }
    if (name == "襁褓三头犬") {
        return CultivatedAnimalType::Cerberus;
    }
    return std::nullopt;
}

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

struct StrategyGoals
{
    // 声明「达成即收工」的目标节点。它与物理出口取并集，所以端点集合恒非空。
    std::unordered_set<NodeId> terminal_nodes;
    // 待锁定的强制目标，按优先级从高到低。前 undemotable_count 条是无条件必达的，阶梯不会降级它们。
    std::vector<std::string> binding_candidates;
    std::size_t undemotable_count = 0;
};

StrategyGoals strategy_goals_for(
    const ResolvedPolicy& policy,
    const MissionState& mission,
    const FactStore& facts,
    const MapSnapshot& map,
    int floor)
{
    StrategyGoals result;
    std::vector<const Milestone*> candidates;
    for (const Milestone& milestone : policy.milestones) {
        if (!milestone_is_active(milestone, floor, facts, mission)) {
            continue;
        }
        bool has_matching_node = false;
        for (const auto& [id, node] : map.nodes()) {
            if (node.progress == NodeProgress::Removed || !milestone_matches_node(milestone, node)) {
                continue;
            }
            has_matching_node = true;
            if (milestone.terminality == MilestoneTerminality::IsTerminal) {
                result.terminal_nodes.emplace(id);
            }
        }
        if (!milestone.binding_candidate() || mission.progress(milestone.id) >= milestone.required_count) {
            continue;
        }
        // 目标在本层地图上没有任何匹配节点时，可行性求解一定得出无解。可行则必达的目标直接跳过，
        // 省掉一次白跑的安全求解；无条件必达的目标仍然送进去，让它按声明把本层判成无解。
        if (milestone.enforcement == MilestoneEnforcement::FeasibleHard && !has_matching_node) {
            continue;
        }
        candidates.emplace_back(&milestone);
    }
    std::ranges::sort(candidates, [](const Milestone* lhs, const Milestone* rhs) {
        if (lhs->enforcement != rhs->enforcement) {
            // Hard 排在 FeasibleHard 之前，阶梯从末尾开始降级，因此永远不会降到 Hard。
            return lhs->enforcement > rhs->enforcement;
        }
        return std::tie(lhs->rank, lhs->id) < std::tie(rhs->rank, rhs->id);
    });
    for (const Milestone* milestone : candidates) {
        result.binding_candidates.emplace_back(milestone->id);
        result.undemotable_count += milestone->enforcement == MilestoneEnforcement::Hard ? 1 : 0;
    }
    return result;
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
        { "cultivation_target", cultivation_target },
        { "cultivated_animal_types", json::array(cultivated_animal_types) },
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
    m_current_floor.reset();
    m_cultivated_animal_types.clear();
    m_unreachable_actions.clear();
    m_pending_probe_target.reset();
    m_verified_move_arc.reset();
    m_pending_candidate.reset();
    m_transaction.reset();
    m_last_plan.reset();
    m_page_context.reset();
    m_result.reset();
    m_result_reported = false;
    m_movement_inventory_refresh_required = true;
    m_persisted_image_packages = 0;
    ++m_run_revision;
    m_page_revision = 0;
    m_decision_sequence = 0;
    m_transaction_sequence = 0;
    m_artifact_sequence = 0;
    m_observed_current_node = InvalidNodeId;
    m_observation_id.clear();
    m_decision_id.clear();
    m_transaction_id.clear();
    m_telemetry_events.clear();
    m_diagnostic_requests.clear();
    // 开局配置整局不变，但事实刚被重置成默认值，所以每次初始化都要重新写回去；
    // reset_run() 走的也是这里，否则重开一局后依赖开局配置的策略条件会静默失效。
    if (!set_fact("start_core_char", m_start_core_char, error) || !set_fact("start_squad", m_start_squad, error) ||
        !set_fact("start_roles", m_start_roles, error)) {
        return false;
    }
    return apply_granted_scraps(error);
}

bool BlackFlowSession::apply_granted_scraps(std::string* error)
{
    m_movement_inventory_assumed = false;
    if (!m_policy.has_value() || m_policy->granted_scraps.empty()) {
        return true;
    }
    const FactStore facts = m_facts.merged();
    for (const GrantedScrap& scrap : m_policy->granted_scraps) {
        if (!scrap.when.evaluate(facts)) {
            continue;
        }
        const MovementSpec* spec = find_movement_spec(scrap.movement);
        if (spec == nullptr) {
            continue;
        }
        m_run.resources.movement_charges.insert_or_assign(scrap.movement, spec->initial_charges);
        m_run.resources.movement_pieces.insert_or_assign(scrap.movement, 1);
        m_movement_inventory_assumed = true;
    }
    if (!m_movement_inventory_assumed) {
        return true;
    }
    ++m_run.resources_revision;
    return synchronize_resource_facts(error);
}

void BlackFlowSession::set_start_loadout(std::string core_char, std::string squad, std::string roles)
{
    m_start_core_char = std::move(core_char);
    m_start_squad = std::move(squad);
    m_start_roles = std::move(roles);
}

void BlackFlowSession::set_cultivated_animal_types(std::vector<CultivatedAnimalType> types)
{
    m_cultivated_animal_types.clear();
    m_cultivated_animal_types.reserve(types.size());
    for (const CultivatedAnimalType type : types) {
        if (std::find(m_cultivated_animal_types.begin(), m_cultivated_animal_types.end(), type) ==
            m_cultivated_animal_types.end()) {
            m_cultivated_animal_types.emplace_back(type);
        }
    }
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
        { "persistent_full_map_count", m_resources.read("persistent_full_map_movement", m_run).value_or(0) },
        { "persistent_reach_scrap_shop_count", m_resources.read("persistent_reach_scrap_shop", m_run).value_or(0) },
        { "persistent_reach_battle_boss_count", m_resources.read("persistent_reach_battle_boss", m_run).value_or(0) },
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
    publish_milestone_facts();
    for (const auto& [id, status] : m_mission.milestones) {
        const auto old = previous.find(id);
        if (status == MilestoneStatus::Inactive || (old != previous.end() && old->second == status)) {
            continue;
        }
        json::object details {
            { "run_revision", m_run_revision },
            { "observation_id", m_observation_id },
            { "floor", m_run.floor },
            { "milestone_id", id },
            { "status", std::string(to_string(status)) },
            { "progress", m_mission.progress(id) },
        };
        m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowMilestoneChanged", std::move(details) });
    }
}

// 里程碑状态发布成事实，条件语言和终止规则才能读到它。事实名由配置解析时自动登记，
// 形如 milestone.<id>.status 与 milestone.<id>.progress。
void BlackFlowSession::publish_milestone_facts()
{
    if (!m_policy.has_value()) {
        return;
    }
    std::string ignored;
    for (const Milestone& milestone : m_policy->milestones) {
        (void)set_fact(
            "milestone." + milestone.id + ".status",
            std::string(to_string(m_mission.status(milestone.id))),
            &ignored);
        (void)set_fact(
            "milestone." + milestone.id + ".progress",
            static_cast<std::int64_t>(m_mission.progress(milestone.id)),
            &ignored);
    }
}

// 声明了 on_miss 的里程碑一旦错过或不可能，直接结算本局，省得每条策略再写一遍同样的终止规则。
void BlackFlowSession::evaluate_milestone_miss_actions()
{
    if (m_result.has_value() || !m_policy.has_value()) {
        return;
    }
    for (const Milestone& milestone : m_policy->milestones) {
        if (milestone.on_miss != MilestoneMissAction::Terminate) {
            continue;
        }
        const MilestoneStatus status = m_mission.status(milestone.id);
        if (status != MilestoneStatus::Missed && status != MilestoneStatus::Impossible) {
            continue;
        }
        const FactStore facts = m_facts.merged();
        const int cultivated =
            static_cast<int>(std::clamp<std::int64_t>(integer_fact(facts, "cultivated_animals"), 0, 3));
        BlackFlowStrategyResult result {
            m_profile, milestone.miss_outcome, milestone.miss_reason, cultivated, milestone.miss_succeeded,
        };
        result.next_action = milestone.miss_succeeded ? "stop_run" : m_policy->failure_action;
        m_result = std::move(result);
        return;
    }
}

// 页面意图按进入之后确定的真实身份重新解析：取第一条匹配该节点且带意图的活跃里程碑。
// 排序与策略排序一致，锁定候选优先，因此硬目标的意图压过顺路目标。
std::string BlackFlowSession::resolve_page_intent(const PageIdentityResolution& identity, NodeId node, int floor) const
{
    if (!m_policy.has_value()) {
        return "default";
    }
    Node probe;
    if (const Node* stored = m_map.snapshot().find_node(node); stored != nullptr) {
        probe = *stored;
    }
    probe.id = node;
    probe.floor = floor;
    probe.type = identity.type;
    probe.name = identity.name;
    probe.identity_revealed = true;
    probe.identity_state = NodeIdentityState::Classified;

    const FactStore facts = m_facts.merged();
    std::vector<const Milestone*> active;
    for (const Milestone& milestone : m_policy->milestones) {
        if (milestone.page_intent.empty() || !milestone_is_active(milestone, floor, facts, m_mission)) {
            continue;
        }
        active.emplace_back(&milestone);
    }
    std::ranges::sort(active, [](const Milestone* lhs, const Milestone* rhs) {
        if (lhs->binding_candidate() != rhs->binding_candidate()) {
            return lhs->binding_candidate();
        }
        return std::tie(lhs->kind, lhs->rank, lhs->id) < std::tie(rhs->kind, rhs->rank, rhs->id);
    });
    for (const Milestone* milestone : active) {
        if (milestone_matches_node(*milestone, probe)) {
            return milestone->page_intent;
        }
    }
    return "default";
}

void BlackFlowSession::evaluate_terminal_rules()
{
    if (m_result.has_value() || !m_policy.has_value()) {
        return;
    }
    const FactStore facts = m_facts.merged();
    for (const StrategyTerminalRule& rule : m_policy->terminal_rules) {
        if (!rule.when.evaluate(facts)) {
            continue;
        }
        const int cultivated =
            static_cast<int>(std::clamp<std::int64_t>(integer_fact(facts, "cultivated_animals"), 0, 3));
        BlackFlowStrategyResult result {
            m_profile, rule.outcome, rule.reason, cultivated, rule.succeeded,
        };
        result.next_action =
            rule.next_action.empty() ? (rule.succeeded ? "stop_run" : m_policy->failure_action) : rule.next_action;
        if (m_profile == "baby_animal" && result.outcome == "baby_cultivation_completed") {
            result.cultivation_target = std::string(to_string(m_cultivation_target));
            result.cultivated_animal_types.reserve(m_cultivated_animal_types.size());
            for (const CultivatedAnimalType type : m_cultivated_animal_types) {
                result.cultivated_animal_types.emplace_back(to_string(type));
            }

            const bool target_obtained =
                std::find(m_cultivated_animal_types.begin(), m_cultivated_animal_types.end(), m_cultivation_target) !=
                m_cultivated_animal_types.end();
            if (target_obtained) {
                result.termination_reason = "cultivation_target_obtained";
                result.succeeded = true;
                result.next_action = "stop_run";
            }
            else {
                result.outcome = "baby_cultivation_target_missed";
                result.termination_reason = "cultivation_target_not_obtained";
                result.succeeded = false;
                result.next_action = m_policy->failure_action;
            }
        }
        m_result = std::move(result);
        return;
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
    const RunResources resources_before = m_run.resources;
    const auto cross_floor_expired_before = m_run.cross_floor_expired;
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
    if (observation.action_points.has_value()) {
        if (*observation.action_points < 0 || *observation.action_points > 64) {
            if (error != nullptr) {
                *error = "action points must be between 0 and 64";
            }
            return false;
        }
        m_run.resources.action_points = *observation.action_points;
    }
    if (!assign_nonnegative(observation.hope, m_run.resources.hope, "hope") ||
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
        std::unordered_map<MovementKind, int> observed_charges;
        std::unordered_map<MovementKind, int> observed_pieces;
        for (const auto& [movement, charges] : *observation.movement_charges) {
            const MovementSpec* spec = find_movement_spec(movement);
            if (movement == MovementKind::Walk || spec == nullptr || charges < 0) {
                if (error != nullptr) {
                    *error = "movement charges contain an invalid movement or negative count";
                }
                return false;
            }
            if (charges > 0) {
                observed_charges.insert_or_assign(movement, charges);
                const int per_piece = std::max(1, spec->initial_charges);
                observed_pieces.insert_or_assign(movement, (charges + per_piece - 1) / per_piece);
            }
        }
        m_run.resources.movement_charges = std::move(observed_charges);
        m_run.resources.movement_pieces = std::move(observed_pieces);
        if (m_run.active_movement.has_value() && *m_run.active_movement != MovementKind::Walk &&
            !m_run.resources.movement_charges.contains(*m_run.active_movement)) {
            m_run.active_movement.reset();
        }
    }
    if (observation.movement_panel.has_value()) {
        const MovementPanelObservation& panel = *observation.movement_panel;
        if (find_movement_spec(panel.target) == nullptr || !movement_panel_observation_is_structurally_valid(panel)) {
            if (error != nullptr) {
                *error = "movement panel observation is inconsistent";
            }
            return false;
        }
        const auto owned = m_run.resources.movement_charges.find(panel.target);
        if (panel.target != MovementKind::Walk && owned != m_run.resources.movement_charges.end()) {
            const auto held = m_run.resources.movement_pieces.find(panel.target);
            const bool single_piece = held == m_run.resources.movement_pieces.end() || held->second <= 1;
            const bool reliable_count = movement_panel_has_reliable_count(panel);
            const bool confirmed_absent = movement_panel_confirms_absent(panel);
            if (reliable_count && single_piece) {
                if (*panel.remaining_charges == 0) {
                    m_run.resources.movement_charges.erase(owned);
                    m_run.resources.movement_pieces.erase(panel.target);
                    if (m_run.active_movement == panel.target) {
                        m_run.active_movement.reset();
                    }
                }
                else {
                    owned->second = *panel.remaining_charges;
                }
            }
            else if (confirmed_absent) {
                m_run.resources.movement_charges.erase(owned);
                m_run.resources.movement_pieces.erase(panel.target);
                if (m_run.active_movement == panel.target) {
                    m_run.active_movement.reset();
                }
            }
        }
    }
    if (observation.active_movement.has_value()) {
        if (find_movement_spec(*observation.active_movement) == nullptr) {
            if (error != nullptr) {
                *error = "active movement observation is invalid";
            }
            return false;
        }
        m_run.active_movement = *observation.active_movement;
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
    if (m_run.resources != resources_before || m_run.cross_floor_expired != cross_floor_expired_before) {
        ++m_run.resources_revision;
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
        { "floor_from_ocr", summary.floor_from_ocr },
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
        "floor source",
        summary.floor_from_ocr ? "ocr" : "fallback",
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
        summary.attempt_count,
        "retries",
        summary.retry_count,
        "screenshot us",
        summary.screenshot_us,
        "recognition us",
        summary.recognition_us);
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
    const int requirement = move.action_point_requirement;
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
    if (!decision.decisive_milestone_ids.empty() && m_policy.has_value()) {
        for (const std::string& milestone_id : decision.decisive_milestone_ids) {
            const auto found = std::ranges::find(m_policy->milestones, milestone_id, &Milestone::id);
            if (found == m_policy->milestones.end()) {
                continue;
            }
            if (!reason_detail.empty()) {
                reason_detail += "; ";
            }
            reason_detail += found->description;
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
                { "node_type",
                  std::string(runner_target == nullptr ? "unclassified" : to_string(runner_target->type)) },
                { "predicted_cost", runner.predicted_action_point_cost },
            });
    }
    json::object planned_progress;
    for (const auto& [milestone, progress] : decision.planned_milestone_progress) {
        planned_progress[milestone] = progress;
    }
    json::object rejected;
    for (const auto& [category, count] : decision.rejection_counts) {
        rejected[category] = count;
    }
    std::vector<json::value> decisive_milestones;
    decisive_milestones.reserve(decision.decisive_milestone_ids.size());
    for (const std::string& milestone_id : decision.decisive_milestone_ids) {
        decisive_milestones.emplace_back(milestone_id);
    }
    std::vector<json::value> released_reserves;
    released_reserves.reserve(decision.released_reserve_ids.size());
    for (const std::string& reserve_id : decision.released_reserve_ids) {
        released_reserves.emplace_back(reserve_id);
    }

    json::object details {
        { "run_revision", m_run_revision },
        { "observation_id", m_observation_id },
        { "decision_id", m_decision_id },
        { "profile", m_profile },
        { "transaction_id", m_transaction_id },
        { "map_revision", m_map.snapshot().revision },
        { "floor", m_run.floor },
        { "source", move.source },
        { "target", move.target },
        { "landing", move.landing },
        { "node_name", target == nullptr ? std::string() : target->name },
        { "node_type", std::string(target == nullptr ? "unclassified" : to_string(target->type)) },
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
        { "decisive_milestone_ids", json::array(std::move(decisive_milestones)) },
        { "total_candidates", decision.total_candidates },
        { "eligible_candidates", decision.eligible_candidates },
        { "rejection_counts", std::move(rejected) },
        { "released_reserve_ids", json::array(std::move(released_reserves)) },
        { "runners_up", json::array(std::move(runners_up)) },
        { "planned_milestone_progress", std::move(planned_progress) },
        { "uses_inferred_edge", move.uses_inferred_edge },
        { "confirmed_state_count", static_cast<std::int64_t>(m_last_plan->confirmed_state_count) },
        { "relaxed_state_count", static_cast<std::int64_t>(m_last_plan->relaxed_state_count) },
        { "route_search_expansions", static_cast<std::int64_t>(m_last_plan->route_search_expansions) },
        { "route_search_time_exhausted", m_last_plan->route_search_time_exhausted },
        { "route_search_expansions_exhausted", m_last_plan->route_search_expansions_exhausted },
    };
    if (includes_full_routing_details(m_diagnostics.level)) {
        const auto node_details = [&](NodeId id) {
            const Node* node = m_map.snapshot().find_node(id);
            if (node == nullptr) {
                return json::object { { "id", id } };
            }
            return json::object {
                { "id", id },
                { "row", node->position.row },
                { "column", node->position.column },
                { "node_type", std::string(to_string(node->type)) },
                { "node_name", node->name },
            };
        };
        std::vector<json::value> planned_route_steps;
        planned_route_steps.reserve(decision.planned_route_steps.size());
        for (const PlannedRouteStep& step : decision.planned_route_steps) {
            std::vector<json::value> path;
            path.reserve(step.move.path.size());
            for (const NodeId node : step.move.path) {
                path.emplace_back(node_details(node));
            }
            planned_route_steps.emplace_back(
                json::object {
                    { "action_id", step.move.action_id },
                    { "movement", std::string(to_string(step.move.movement)) },
                    { "source", node_details(step.move.source) },
                    { "target", node_details(step.move.target) },
                    { "landing", node_details(step.move.landing) },
                    { "path", json::array(std::move(path)) },
                    { "action_point_requirement", step.move.action_point_requirement },
                    { "action_points_before", step.action_points_before },
                    { "action_point_cost", step.action_point_cost },
                    { "action_point_gain", step.action_point_gain },
                    { "action_points_after", step.action_points_after },
                    { "uses_processing_item", step.move.movement != MovementKind::Walk },
                    { "uses_inferred_edge", step.move.uses_inferred_edge },
                });
        }
        details["planned_route_steps"] = json::array(std::move(planned_route_steps));
    }
    Log.info(
        "BlackFlow decision",
        m_decision_id,
        "profile",
        m_profile,
        "rule",
        decision.decisive_rule_id,
        "milestone",
        decision.decisive_milestone_id,
        "floor",
        m_run.floor,
        "target",
        move.target,
        "cost",
        cost,
        "margin",
        margin,
        "reason",
        to_string(decision.reason_category),
        "confirmed states",
        m_last_plan->confirmed_state_count,
        "relaxed states",
        m_last_plan->relaxed_state_count,
        "route expansions",
        m_last_plan->route_search_expansions,
        "time exhausted",
        m_last_plan->route_search_time_exhausted,
        "expansions exhausted",
        m_last_plan->route_search_expansions_exhausted);
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
    bool reconcile_move,
    std::string* error)
{
    auto normalized = m_observation_adapter.normalize(observation, error);
    if (!normalized.has_value()) {
        return false;
    }

    const NodeId previous_current_node = m_observed_current_node;
    const bool new_floor = m_run.floor != 0 && m_run.floor != normalized->map.floor;
    if (new_floor) {
        m_movement_inventory_refresh_required = true;
        m_facts.begin_floor();
        m_unreachable_actions.clear();
        m_pending_probe_target.reset();
        m_verified_move_arc.reset();
        m_pending_candidate.reset();
        if (!reconcile_move) {
            m_transaction.reset();
            m_page_context.reset();
        }
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
    const bool routing_context_changed =
        !reconcile_move && previous_current_node != InvalidNodeId && previous_current_node != normalized->current_node;
    if (routing_context_changed) {
        m_run.costs.clear_action_cost_overrides();
        m_unreachable_actions.clear();
        m_pending_probe_target.reset();
        m_verified_move_arc.reset();
        m_pending_candidate.reset();
        if (m_transaction.has_value()) {
            m_transaction->invalidate();
            m_transaction.reset();
        }
    }
    if (!reconcile_move) {
        m_run.floor = normalized->map.floor;
        m_current_floor = normalized->map.floor;
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
        if (node.type == NodeType::Light) {
            const auto statically_revealed = m_map.snapshot().nodes_within_manhattan(node_id, 1);
            m_run.revealed_nodes.insert(statically_revealed.begin(), statically_revealed.end());
        }
    }
    if (!apply_observed_facts(observed_facts, error) ||
        !set_fact("map_full_coverage", normalized->map.coverage == ObservationCoverage::FullMap, error) ||
        !set_fact("portal_available", has_node_type(m_map.snapshot(), NodeType::Portal), error) ||
        !set_fact("scrap_shop_available", has_node_type(m_map.snapshot(), NodeType::ScrapShop), error)) {
        return false;
    }
    queue_map_summary(normalized->summary);
    return true;
}

void BlackFlowSession::finalize_entered_node(const PageExecutionContext& context, bool page_completed)
{
    Node entered;
    if (const Node* stored = m_map.snapshot().find_node(context.node);
        stored != nullptr && stored->floor == context.floor) {
        entered = *stored;
    }
    else {
        entered.id = context.node;
        entered.floor = context.floor;
        entered.type = context.node_type;
        entered.name = context.node_name;
        entered.traversal = default_traversal_for(entered.type);
    }

    if (context.result.has_value()) {
        const NodeStateUpdate& update = *context.result;
        if (update.actual_type.has_value()) {
            entered.type = *update.actual_type;
        }
        if (update.actual_name.has_value()) {
            entered.name = *update.actual_name;
        }
        if (update.identity_revealed.has_value()) {
            entered.identity_revealed = *update.identity_revealed;
            entered.identity_state =
                *update.identity_revealed ? NodeIdentityState::Classified : NodeIdentityState::Hidden;
        }
        if (page_completed && update.repeatable.has_value()) {
            entered.traversal.repeatable = *update.repeatable;
        }
        if (page_completed && update.progress.has_value()) {
            entered.progress = *update.progress;
        }
        if (page_completed && update.becomes_empty.value_or(false)) {
            entered.type = NodeType::Empty;
            entered.progress = NodeProgress::Completed;
            entered.traversal = default_traversal_for(NodeType::Empty);
        }
    }
    else if (page_completed) {
        entered.progress = NodeProgress::Completed;
    }

    m_run.visited_nodes.emplace(context.node);
    m_run.node_progress.insert_or_assign(context.node, entered.progress);
    if (entered.type == NodeType::Light) {
        m_run.consumed_one_time_nodes.emplace(context.node);
    }
    if (page_completed && entered.type != NodeType::Empty) {
        m_mission.record_node(m_policy->milestones, m_facts.merged(), entered);
    }

    if (context.floor == m_run.floor) {
        const Node* observed = m_map.snapshot().find_node(context.node);
        if (observed != nullptr) {
            Node updated = *observed;
            if (context.result.has_value() && context.result->actual_type.has_value()) {
                updated.type = *context.result->actual_type;
            }
            if (context.result.has_value() && context.result->identity_revealed.has_value()) {
                updated.identity_revealed = *context.result->identity_revealed;
                updated.identity_state =
                    updated.identity_revealed ? NodeIdentityState::Classified : NodeIdentityState::Hidden;
            }
            if (page_completed && context.result.has_value() && context.result->repeatable.has_value()) {
                updated.traversal.repeatable = *context.result->repeatable;
            }
            updated.progress = entered.progress;
            if (page_completed && context.result.has_value() && context.result->becomes_empty.value_or(false)) {
                updated.type = NodeType::Empty;
                updated.traversal = default_traversal_for(NodeType::Empty);
            }
            else if (page_completed && updated.progress == NodeProgress::Completed) {
                updated.traversal.blocks_walk = false;
                updated.traversal.blocks_vision = false;
            }
            m_map.snapshot().upsert_node(std::move(updated));
        }
    }

    if (page_completed && !context.resolution_reported) {
        queue_node_resolution(context);
    }
}

bool BlackFlowSession::reconcile_committed_move(const BlackFlowPerceptionSnapshot& snapshot, std::string* error)
{
    if (!m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "map return has no committed BlackFlow movement";
        }
        return false;
    }
    const MoveTransactionStage stage = m_transaction->stage();
    if (stage != MoveTransactionStage::Committed && stage != MoveTransactionStage::PageResolved) {
        if (error != nullptr) {
            *error = "map return cannot reconcile the current movement stage";
        }
        return false;
    }
    if (stage == MoveTransactionStage::Committed && !m_page_context.has_value()) {
        if (error != nullptr) {
            *error = "committed node movement has no BlackFlow page context";
        }
        return false;
    }
    if (!merge_perception(snapshot.observation, snapshot.run, snapshot.observed_facts, true, error)) {
        return false;
    }

    MoveObservation observation;
    observation.current_node = m_observed_current_node;
    observation.floor = m_map.floor();
    observation.map_revision = m_map.snapshot().revision;
    observation.viewport_revision = m_viewport.viewport_revision();
    if (snapshot.run.action_points.has_value()) {
        observation.action_points = *snapshot.run.action_points;
    }
    else if (snapshot.observation.hud_action_points.has_value()) {
        observation.action_points = *snapshot.observation.hud_action_points;
    }
    else {
        const MoveCandidate& proposal = m_transaction->proposal();
        int gain = proposal.predicted_action_point_gain;
        if (!proposal.controllable) {
            const auto found = proposal.landing_action_point_gains.find(observation.current_node);
            gain = found == proposal.landing_action_point_gains.end() ? 0 : found->second;
        }
        observation.action_points =
            action_points_after(m_run.resources.action_points, m_transaction->authoritative_cost(), gain);
    }

    if (m_page_context.has_value()) {
        if (observation.floor == m_page_context->floor) {
            if (const Node* landed = m_map.snapshot().find_node(observation.current_node); landed != nullptr) {
                observation.landed_type = landed->type;
                observation.target_progress = landed->progress;
            }
        }
        else {
            observation.landed_type = m_page_context->node_type;
            observation.target_progress =
                m_page_context->result.has_value() && m_page_context->result->progress.has_value()
                    ? *m_page_context->result->progress
                    : NodeProgress::Completed;
        }
    }
    else if (const Node* landed = m_map.snapshot().find_node(observation.current_node); landed != nullptr) {
        observation.landed_type = landed->type;
        observation.target_progress = landed->progress;
    }

    if (!m_transaction->observe(observation, error) || !m_transaction->apply(m_run, error)) {
        queue_warning(
            "post_move_mismatch",
            error == nullptr ? "map return does not match the committed movement" : *error,
            DiagnosticTrigger::PostMoveMismatch);
        return false;
    }

    m_run.floor = observation.floor;
    m_current_floor = observation.floor;
    m_run.current_node = observation.current_node;
    RunObservation effective = snapshot.run;
    if (!effective.action_points.has_value()) {
        effective.action_points = observation.action_points;
    }
    if (snapshot.run.action_points.has_value() && snapshot.observation.hud_action_points.has_value() &&
        *snapshot.run.action_points != *snapshot.observation.hud_action_points) {
        if (error != nullptr) {
            *error = "HUD action points conflict with the state observation";
        }
        return false;
    }
    if (!apply_run_observation(effective, error)) {
        return false;
    }
    if (m_page_context.has_value()) {
        const bool page_completed = m_transaction->stage() == MoveTransactionStage::Applied &&
                                    m_page_context->stage == PageExecutionStage::Resolved;
        finalize_entered_node(*m_page_context, page_completed);
    }

    m_transaction.reset();
    m_page_context.reset();
    m_transaction_id.clear();
    m_verified_move_arc.reset();
    m_pending_candidate.reset();
    m_pending_probe_target.reset();
    m_run.costs.clear_action_cost_overrides();
    m_unreachable_actions.clear();
    return true;
}

bool BlackFlowSession::apply_movement_panel_observation(
    MovementPanelObservation panel,
    std::optional<MovementKind> active_movement,
    std::string* error)
{
    const MovementKind target = panel.target;
    const bool reliable_count = movement_panel_has_reliable_count(panel);
    const bool confirmed_absent = movement_panel_confirms_absent(panel);

    BlackFlowSession staged = *this;
    RunObservation observation;
    observation.active_movement = active_movement;
    observation.movement_panel = std::move(panel);
    if (!staged.apply_run_observation(observation, error)) {
        return false;
    }
    if (staged.m_pending_candidate.has_value() && staged.m_pending_candidate->candidate.movement == target) {
        if (confirmed_absent) {
            staged.m_pending_candidate.reset();
        }
        else if (reliable_count) {
            const auto charges = staged.m_run.resources.movement_charges.find(target);
            if (charges == staged.m_run.resources.movement_charges.end() || charges->second <= 0) {
                staged.m_pending_candidate.reset();
            }
            else {
                staged.m_pending_candidate->resources_revision = staged.m_run.resources_revision;
            }
        }
    }
    if (!staged.synchronize_resource_facts(error)) {
        return false;
    }
    *this = std::move(staged);
    return true;
}

bool BlackFlowSession::apply_movement_inventory_observation(
    const std::unordered_map<MovementKind, int>& visible_movements,
    std::string* error)
{
    BlackFlowSession staged = *this;
    const RunResources resources_before = staged.m_run.resources;
    for (const auto& [movement, pieces] : visible_movements) {
        const MovementSpec* spec = find_movement_spec(movement);
        if (movement == MovementKind::Walk || spec == nullptr || spec->initial_charges <= 0 || pieces <= 0) {
            if (error != nullptr) {
                *error = "movement inventory contains an invalid processing item";
            }
            return false;
        }
        int& charges = staged.m_run.resources.movement_charges[movement];
        const auto known = staged.m_run.resources.movement_pieces.find(movement);
        const int previous_pieces =
            charges > 0 && known != staged.m_run.resources.movement_pieces.end() ? known->second : 0;
        if (pieces > previous_pieces) {
            charges += (pieces - previous_pieces) * spec->initial_charges;
        }
        charges = std::clamp(charges, pieces, pieces * spec->initial_charges);
        staged.m_run.resources.movement_pieces.insert_or_assign(movement, pieces);
    }
    std::erase_if(staged.m_run.resources.movement_pieces, [&](const auto& entry) {
        const auto charges = staged.m_run.resources.movement_charges.find(entry.first);
        return charges == staged.m_run.resources.movement_charges.end() || charges->second <= 0;
    });
    if (staged.m_run.resources != resources_before) {
        ++staged.m_run.resources_revision;
    }
    staged.m_movement_inventory_refresh_required = false;
    if (!staged.synchronize_resource_facts(error)) {
        return false;
    }
    *this = std::move(staged);
    return true;
}

bool BlackFlowSession::report_movement_unavailable(MovementKind target, std::string* error)
{
    MovementPanelObservation panel;
    panel.target = target;
    panel.completed_swipes = 3;
    panel.complete = true;
    return apply_movement_panel_observation(std::move(panel), std::nullopt, error);
}

bool BlackFlowSession::update(const BlackFlowPerceptionSnapshot& snapshot, std::string* error)
{
    BlackFlowSession staged = *this;
    if (!staged.update_in_place(snapshot, error)) {
        return false;
    }
    *this = std::move(staged);
    return true;
}

bool BlackFlowSession::update_in_place(const BlackFlowPerceptionSnapshot& snapshot, std::string* error)
{
    const bool pending_move =
        m_transaction.has_value() && (m_transaction->stage() == MoveTransactionStage::Committed ||
                                      m_transaction->stage() == MoveTransactionStage::PageResolved);
    if (pending_move) {
        if (!reconcile_committed_move(snapshot, error)) {
            return false;
        }
    }
    else if (!merge_perception(snapshot.observation, snapshot.run, snapshot.observed_facts, false, error)) {
        return false;
    }
    if (!synchronize_resource_facts(error)) {
        return false;
    }

    refresh_mission();
    // 结算只放在观测这一拍。提交事务途中也会刷新里程碑，那时候终止会把页面丢在半路。
    evaluate_milestone_miss_actions();
    evaluate_terminal_rules();
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
        const StrategyGoals goals = strategy_goals_for(*m_policy, m_mission, merged, m_map.snapshot(), m_run.floor);
        request.strategy_terminal_nodes = goals.terminal_nodes;
        request.binding_milestone_candidates = goals.binding_candidates;
        request.undemotable_binding_count = goals.undemotable_count;
        request.no_AP_is_terminal = m_policy->no_AP_is_terminal_floors.contains(m_run.floor);
        request.forbidden_actions = &m_unreachable_actions;
        request.probe_target = m_pending_probe_target;
        result = BlackFlowPlanner {}.plan(request);
        Log.info(
            "BlackFlow strategy goals",
            "profile",
            m_profile,
            "floor",
            m_run.floor,
            "binding candidates",
            goals.binding_candidates.size(),
            "locked",
            result.binding_milestone_ids.size(),
            "demoted",
            result.demoted_milestone_ids.size(),
            "strategy terminal nodes",
            goals.terminal_nodes.size());
        if (result && m_pending_probe_target.has_value() &&
            result.decision.selected->target != *m_pending_probe_target) {
            m_pending_probe_target.reset();
        }
    }
    if (!result.error.empty() && error != nullptr) {
        *error = result.error;
    }
    if (result) {
        m_last_plan = result;
    }
    return result;
}

bool BlackFlowSession::save_pending_candidate(const MoveCandidate& candidate, std::string* error)
{
    if (m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "cannot save a pending candidate while a movement transaction exists";
        }
        return false;
    }
    if (candidate.source != m_run.current_node || m_map.snapshot().find_node(candidate.source) == nullptr) {
        if (error != nullptr) {
            *error = "pending candidate does not start at the current node";
        }
        return false;
    }
    if (find_movement_spec(candidate.movement) == nullptr) {
        if (error != nullptr) {
            *error = "pending candidate references an unknown movement";
        }
        return false;
    }
    if (candidate.movement != MovementKind::Walk) {
        const auto charges = m_run.resources.movement_charges.find(candidate.movement);
        if (charges == m_run.resources.movement_charges.end() || charges->second <= 0) {
            if (error != nullptr) {
                *error = "pending candidate movement has no remaining charges";
            }
            return false;
        }
    }
    if (candidate.controllable &&
        !m_viewport.clickable_rect(candidate.target, m_map.snapshot().revision, m_viewport.viewport_revision())
             .has_value()) {
        if (error != nullptr) {
            *error = "pending candidate has no current viewport coordinate";
        }
        return false;
    }
    m_pending_candidate = PendingMoveCandidate {
        candidate,
        m_run_revision,
        m_map.snapshot().revision,
        m_run.costs.revision,
        m_run.resources_revision,
        m_viewport.viewport_revision(),
    };
    return true;
}

bool BlackFlowSession::validate_pending_candidate(std::string* error) const
{
    if (!m_pending_candidate.has_value()) {
        if (error != nullptr) {
            *error = "no pending movement candidate is saved";
        }
        return false;
    }
    const PendingMoveCandidate& pending = *m_pending_candidate;
    if (pending.run_revision != m_run_revision || pending.map_revision != m_map.snapshot().revision ||
        pending.cost_revision != m_run.costs.revision || pending.resources_revision != m_run.resources_revision ||
        pending.viewport_revision != m_viewport.viewport_revision()) {
        if (error != nullptr) {
            *error = "pending movement candidate revisions no longer match the session";
        }
        return false;
    }
    if (pending.candidate.source != m_run.current_node) {
        if (error != nullptr) {
            *error = "pending movement candidate source is no longer current";
        }
        return false;
    }
    if (!m_run.active_movement.has_value() || *m_run.active_movement != pending.candidate.movement) {
        if (error != nullptr) {
            *error = "active movement does not match the pending candidate";
        }
        return false;
    }
    if (pending.candidate.movement != MovementKind::Walk) {
        const auto charges = m_run.resources.movement_charges.find(pending.candidate.movement);
        if (charges == m_run.resources.movement_charges.end() || charges->second <= 0) {
            if (error != nullptr) {
                *error = "pending candidate movement has no remaining charges";
            }
            return false;
        }
    }
    if (pending.candidate.controllable &&
        !m_viewport.clickable_rect(pending.candidate.target, pending.map_revision, pending.viewport_revision)
             .has_value()) {
        if (error != nullptr) {
            *error = "pending movement candidate viewport coordinate is stale";
        }
        return false;
    }
    return true;
}

bool BlackFlowSession::begin_pending_transaction(std::string* error)
{
    if (!validate_pending_candidate(error)) {
        return false;
    }
    const MoveCandidate candidate = m_pending_candidate->candidate;
    if (!begin_transaction(candidate, error)) {
        return false;
    }
    m_pending_candidate.reset();
    return true;
}

bool BlackFlowSession::begin_transaction(const MoveCandidate& candidate, std::string* error)
{
    auto proposed = MoveTransaction::propose(candidate, m_map.snapshot(), m_viewport, error);
    if (!proposed.has_value()) {
        return false;
    }
    m_verified_move_arc.reset();
    m_pending_candidate.reset();
    m_transaction = std::move(*proposed);
    m_transaction_id = "BF-T" + std::to_string(m_run_revision) + "-" + std::to_string(++m_transaction_sequence);
    return true;
}

PreviewDisposition BlackFlowSession::accept_preview(MovePreview preview, std::string* error)
{
    if (!m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "move preview arrived without a proposed transaction";
        }
        return PreviewDisposition::Failed;
    }
    if (preview.reachability == PreviewReachability::Reachable &&
        preview.exact_action_point_cost > m_run.resources.action_points) {
        preview.reachability = PreviewReachability::InsufficientActionPoints;
    }
    const PreviewReachability reachability = preview.reachability;
    if (!m_transaction->record_preview(preview, error)) {
        return PreviewDisposition::Failed;
    }
    const MoveCandidate proposal = m_transaction->proposal();
    if (m_transaction->stage() == MoveTransactionStage::Cancelled) {
        m_unreachable_actions.emplace(proposal.action_id);
        m_verified_move_arc.reset();
        if (reachability == PreviewReachability::Blocked && proposal.first_unclassified.has_value() &&
            *proposal.first_unclassified != proposal.target) {
            m_pending_probe_target = proposal.first_unclassified;
        }
        else if (m_pending_probe_target == proposal.target) {
            m_pending_probe_target.reset();
        }
        if (reachability == PreviewReachability::Blocked) {
            queue_warning(
                "route_blocked",
                "move preview shows that a blocking node prevents this move",
                DiagnosticTrigger::RebuildConflict);
        }
        else if (reachability == PreviewReachability::InsufficientActionPoints) {
            queue_warning(
                "insufficient_action_points",
                "move preview cost exceeds the remaining action points",
                DiagnosticTrigger::PreviewCostMismatch);
        }
        else {
            queue_warning(
                "target_state_changed",
                "move preview no longer accepts the selected target",
                DiagnosticTrigger::RebuildConflict);
        }
        m_transaction.reset();
        m_transaction_id.clear();
        return PreviewDisposition::ReplanAfterDismiss;
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
        const bool identity_unresolved = existing->type == NodeType::Unknown ||
                                         existing->type == NodeType::HideInvisible ||
                                         existing->type == NodeType::HideBattle;
        if (identity_unresolved &&
            (existing->type != preview.displayed_type || existing->name != preview.displayed_name ||
             existing->identity_revealed != preview.identity_revealed)) {
            Node updated = *existing;
            updated.type = preview.displayed_type;
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

        ++m_run.costs.revision;
        m_verified_move_arc.reset();
        changed = true;
    }
    if (changed) {
        m_transaction->invalidate();
        m_transaction.reset();
        m_transaction_id.clear();
        return PreviewDisposition::ReplanAfterDismiss;
    }

    if (proposal.requires_preview_verification) {
        const FactStore merged = m_facts.merged();
        BlackFlowPlanRequest request;
        request.map = &m_map.snapshot();
        request.run = &m_run;
        request.policy = &*m_policy;
        request.facts = &merged;
        request.mission = &m_mission;
        const StrategyGoals goals = strategy_goals_for(*m_policy, m_mission, merged, m_map.snapshot(), m_run.floor);
        request.strategy_terminal_nodes = goals.terminal_nodes;
        // 预览验证沿用上一次规划锁定的目标，锁定集合的变化留给下一次规划。
        if (m_last_plan.has_value()) {
            request.binding_milestone_candidates.assign(
                m_last_plan->binding_milestone_ids.begin(),
                m_last_plan->binding_milestone_ids.end());
            std::ranges::sort(request.binding_milestone_candidates);
        }
        request.undemotable_binding_count = request.binding_milestone_candidates.size();
        request.no_AP_is_terminal = m_policy->no_AP_is_terminal_floors.contains(m_run.floor);
        request.forbidden_actions = &m_unreachable_actions;
        const PreviewSafetyVerification verification =
            BlackFlowPlanner {}.verify_previewed_move(request, proposal, preview.exact_action_point_cost);
        if (!verification.error.empty()) {
            if (error != nullptr) {
                *error = verification.error;
            }
            return PreviewDisposition::Failed;
        }
        if (!verification.safe) {
            m_unreachable_actions.emplace(proposal.action_id);
            queue_warning(
                "preview_has_no_confirmed_safe_route",
                "the previewed move has no confirmed safe route from its landing",
                DiagnosticTrigger::RebuildConflict);
            m_transaction->cancel();
            m_transaction.reset();
            m_transaction_id.clear();
            return PreviewDisposition::ReplanAfterDismiss;
        }
        m_verified_move_arc = VerifiedMoveArc {
            proposal.action_id,
            proposal.source,
            proposal.target,
            proposal.landing,
            proposal.movement,
            preview.exact_action_point_cost,
            verification.required_action_points_after,
            verification.proof_depth,
            m_map.snapshot().revision,
            m_run.costs.revision,
            m_run.resources_revision,
            m_viewport.viewport_revision(),
        };
    }
    if (!set_fact("page_kind", std::string(to_string(preview.displayed_type)), error)) {
        return PreviewDisposition::Failed;
    }
    return PreviewDisposition::ReadyToCommit;
}

bool BlackFlowSession::validate_commit(std::string* error) const
{
    if (!m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "move commit has no active transaction";
        }
        return false;
    }
    if (m_transaction->stage() != MoveTransactionStage::Previewed || !m_transaction->preview().has_value() ||
        m_transaction->preview()->reachability != PreviewReachability::Reachable) {
        if (error != nullptr) {
            *error = "only a reachable previewed transaction can be committed";
        }
        return false;
    }
    if (m_transaction->map_revision() != m_map.snapshot().revision ||
        m_transaction->viewport_revision() != m_viewport.viewport_revision()) {
        if (error != nullptr) {
            *error = "map or viewport revision changed before commit";
        }
        return false;
    }

    const MoveCandidate& pending = m_transaction->proposal();
    if (!pending.requires_preview_verification) {
        return true;
    }
    const bool verified =
        m_verified_move_arc.has_value() && m_verified_move_arc->action_id == pending.action_id &&
        m_verified_move_arc->source == pending.source && m_verified_move_arc->target == pending.target &&
        m_verified_move_arc->landing == pending.landing && m_verified_move_arc->movement == pending.movement &&
        m_verified_move_arc->map_revision == m_map.snapshot().revision &&
        m_verified_move_arc->cost_revision == m_run.costs.revision &&
        m_verified_move_arc->resources_revision == m_run.resources_revision &&
        m_verified_move_arc->viewport_revision == m_viewport.viewport_revision() &&
        m_verified_move_arc->exact_action_point_cost == m_transaction->preview()->exact_action_point_cost;
    if (!verified && error != nullptr) {
        *error = "preview-verified move arc expired before commit";
    }
    return verified;
}

bool BlackFlowSession::commit(EnteredPageObservation entered_page, std::string* error)
{
    if (!m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "move commit has no active transaction";
        }
        return false;
    }
    const MoveCandidate& pending = m_transaction->proposal();
    if (pending.requires_preview_verification) {
        const bool verified =
            m_verified_move_arc.has_value() && m_verified_move_arc->action_id == pending.action_id &&
            m_verified_move_arc->source == pending.source && m_verified_move_arc->target == pending.target &&
            m_verified_move_arc->landing == pending.landing && m_verified_move_arc->movement == pending.movement &&
            m_verified_move_arc->map_revision == m_map.snapshot().revision &&
            m_verified_move_arc->cost_revision == m_run.costs.revision &&
            m_verified_move_arc->resources_revision == m_run.resources_revision &&
            m_verified_move_arc->viewport_revision == m_viewport.viewport_revision() &&
            m_transaction->preview().has_value() &&
            m_verified_move_arc->exact_action_point_cost == m_transaction->preview()->exact_action_point_cost;
        if (!verified) {
            m_transaction->invalidate();
            if (error != nullptr) {
                *error = "preview-verified move arc expired before commit";
            }
            return false;
        }
    }

    const bool committed = m_transaction->commit(m_map.snapshot().revision, m_viewport.viewport_revision(), error);
    if (!committed) {
        return false;
    }

    const MoveCandidate& proposal = m_transaction->proposal();
    const NodeId page_node = proposal.controllable ? proposal.target : InvalidNodeId;
    const Node* target = page_node == InvalidNodeId ? nullptr : m_map.snapshot().find_node(page_node);
    if (target != nullptr && (target->type == NodeType::Empty || is_transfer_node(target->type))) {
        if (!m_transaction->mark_page_resolved(error)) {
            return false;
        }
        m_page_context.reset();
        if (m_pending_probe_target == proposal.target) {
            m_pending_probe_target.reset();
        }
        queue_decision();
        return true;
    }

    const NodeType map_type = target == nullptr ? NodeType::Unknown : target->type;
    const std::string map_name = target == nullptr ? std::string {} : target->name;
    const int page_floor = target == nullptr ? m_run.floor : target->floor;
    const MovePreview* preview = m_transaction->preview().has_value() ? &*m_transaction->preview() : nullptr;
    const bool entered_identity_conflict =
        entered_page.classification_conflict ||
        (target != nullptr && target->identity_revealed && entered_page.classified_type.has_value() &&
         target->type != *entered_page.classified_type);
    if (entered_identity_conflict) {
        queue_warning(
            "entered_page_identity_conflict",
            "entered-page classification conflicts with the current normalized map",
            DiagnosticTrigger::IdentityConflict);
    }
    PageIdentityResolution identity = resolve_page_identity(map_type, map_name, preview, entered_page);

    // 隐藏节点要进来才认得出身份。地图阶段算出的意图是按 hide_invisible 定的，沿用它会把
    // 秘境行商这类节点分流到通用页面。这里按真实身份重新解析一次。
    //
    // 顺序不能颠倒：意图来自里程碑，而里程碑的 active_if 可能依赖身份带来的派生事实，
    // 所以要先补事实、再刷新里程碑、最后解析意图。地图节点本身仍由 finalize_entered_node
    // 在页面结束后写回，避免在事务提交之后改动地图版本。
    if (identity.type != map_type) {
        std::string identity_error;
        if (identity.type == NodeType::ScrapShop) {
            (void)set_fact("scrap_shop_available", true, &identity_error);
        }
        else if (identity.type == NodeType::Portal) {
            (void)set_fact("portal_available", true, &identity_error);
        }
        refresh_mission();
    }
    std::string page_intent = resolve_page_intent(identity, page_node, page_floor);

    m_page_context = PageExecutionContext {
        m_run_revision,
        ++m_page_revision,
        m_decision_id,
        m_transaction_id,
        page_floor,
        page_node,
        identity.type,
        std::move(identity.name),
        std::move(page_intent),
        std::move(entered_page.matched_texts),
        PageExecutionStage::PendingDispatch,
    };
    if (m_pending_probe_target == proposal.target) {
        m_pending_probe_target.reset();
    }
    queue_decision();
    return true;
}

void BlackFlowSession::cancel_transaction()
{
    if (m_transaction.has_value()) {
        m_transaction->cancel();
        m_transaction.reset();
    }
    m_verified_move_arc.reset();
    m_transaction_id.clear();
}

bool BlackFlowSession::set_current_floor(int floor, std::string* error)
{
    if (floor <= 0) {
        if (error != nullptr) {
            *error = "recognized floor must be positive";
        }
        m_current_floor.reset();
        return false;
    }
    m_current_floor = floor;
    // 进了新楼层，旧楼层与旧页面的观测立即作废，否则终止规则会拿上一层的覆盖率、商店有无来判这一层。
    m_facts.begin_floor();
    if (!set_fact("current_floor", static_cast<std::int64_t>(floor), error)) {
        return false;
    }
    // 有些终点只看楼层号，认出标题就该收工，不必等这一层的地图重建成功。
    // 这里不刷新里程碑：它按 m_run.floor 算窗口，而那还是上一层的值。
    evaluate_terminal_rules();
    return true;
}

bool BlackFlowSession::completed_page_changes_floor() const noexcept
{
    if (!m_transaction.has_value() || !m_page_context.has_value() ||
        m_transaction->stage() != MoveTransactionStage::PageResolved ||
        m_page_context->stage != PageExecutionStage::Resolved) {
        return false;
    }

    const NodeType node_type = m_page_context->node_type;
    // Portal returns to the same main-map floor and does not require NextLevel.
    return node_type == NodeType::Final || node_type == NodeType::Evacuate || node_type == NodeType::BattleBoss;
}

bool BlackFlowSession::mark_page_running(std::string* error)
{
    if (!m_page_context.has_value() || !m_transaction.has_value() ||
        m_transaction->stage() != MoveTransactionStage::Committed ||
        m_page_context->stage != PageExecutionStage::PendingDispatch) {
        if (error != nullptr) {
            *error = "node dispatch has no committed BlackFlow page";
        }
        return false;
    }
    m_page_context->stage = PageExecutionStage::Running;
    return true;
}

bool BlackFlowSession::apply_node_signal(
    const NodeStrategySignal& signal,
    const json::value& callback_details,
    std::string* error)
{
    if (signal.kind == NodeSignalKind::Set) {
        if (!signal.value.has_value()) {
            if (error != nullptr) {
                *error = "set signal has no value";
            }
            return false;
        }
        FactValue value = std::visit([](const auto& item) -> FactValue { return item; }, *signal.value);
        return set_fact(signal.fact, std::move(value), error);
    }
    if (signal.kind == NodeSignalKind::Add) {
        if (!signal.value.has_value() || !std::holds_alternative<std::int64_t>(*signal.value)) {
            if (error != nullptr) {
                *error = "add signal has no integer value";
            }
            return false;
        }
        const FactValue* current = m_facts.find(signal.fact);
        if (current == nullptr || !std::holds_alternative<std::int64_t>(*current)) {
            if (error != nullptr) {
                *error = "add signal target is not an initialized integer fact";
            }
            return false;
        }
        const std::int64_t delta = std::get<std::int64_t>(*signal.value);
        return set_fact(signal.fact, saturated_add(std::get<std::int64_t>(*current), delta), error);
    }

    const std::string text = callback_details.get("details", "result", "text", "");
    std::smatch match;
    if (!std::regex_search(text, match, std::regex(R"([-+]?\d+)"))) {
        if (error != nullptr) {
            *error = "capture_integer node result found no integer in details.result.text";
        }
        return false;
    }
    std::int64_t parsed = 0;
    try {
        parsed = std::stoll(match.str());
    }
    catch (const std::exception&) {
        if (error != nullptr) {
            *error = "captured integer is outside the supported range";
        }
        return false;
    }
    parsed = std::clamp<std::int64_t>(parsed, signal.minimum, signal.maximum);
    return set_fact(signal.fact, parsed, error);
}

void BlackFlowSession::queue_node_resolution(const PageExecutionContext& context)
{
    NodeType resolved_type = context.node_type;
    NodeProgress progress = NodeProgress::Completed;
    bool repeatable = false;
    bool becomes_empty = false;
    if (context.result.has_value()) {
        const NodeStateUpdate& result = *context.result;
        resolved_type = result.actual_type.value_or(resolved_type);
        progress = result.progress.value_or(progress);
        repeatable = result.repeatable.value_or(false);
        becomes_empty = result.becomes_empty.value_or(false);
    }

    json::object details {
        { "run_revision", context.run_revision },
        { "observation_id", m_observation_id },
        { "decision_id", context.decision_id },
        { "transaction_id", context.transaction_id },
        { "page_revision", context.page_revision },
        { "floor", context.floor },
        { "node", context.node },
        { "event_name", context.node_name },
        { "node_type", std::string(to_string(resolved_type)) },
        { "progress",
          progress == NodeProgress::Active ? "active"
                                           : (progress == NodeProgress::Completed ? "completed" : "removed") },
        { "repeatable", repeatable },
        { "becomes_empty", becomes_empty },
    };
    Log.info(
        "BlackFlow node resolution",
        "floor",
        context.floor,
        "node",
        context.node,
        "event",
        context.node_name,
        "type",
        to_string(resolved_type));
    m_telemetry_events.emplace_back(BlackFlowTelemetryEvent { "BlackFlowNodeResolution", std::move(details) });
}

bool BlackFlowSession::apply_node_task_result(
    const NodeTaskResult& result,
    const json::value& callback_details,
    std::string* error)
{
    if (!m_page_context.has_value() || !m_transaction.has_value()) {
        if (error != nullptr) {
            *error = "node task result arrived without an active BlackFlow page";
        }
        return false;
    }

    for (const NodeStrategySignal& signal : result.signals) {
        if (!apply_node_signal(signal, callback_details, error)) {
            return false;
        }
    }

    NodeStateUpdate update = result.node;
    if (!update.actual_name_source.empty()) {
        const std::string captured_name = callback_details.get("details", "result", "text", "");
        if (captured_name.empty()) {
            if (error != nullptr) {
                *error = "node result found no event name in details.result.text";
            }
            return false;
        }
        update.actual_name = captured_name;
    }
    const bool has_node_update = update.progress.has_value() || update.actual_type.has_value() ||
                                 update.actual_name.has_value() || update.identity_revealed.has_value() ||
                                 update.repeatable.has_value() || update.becomes_empty.has_value();
    if (result.redispatch && !update.actual_type.has_value() && !update.actual_name.has_value()) {
        if (error != nullptr) {
            *error = "node redispatch requires an updated event name or node type";
        }
        return false;
    }
    if (has_node_update) {
        NodeStateUpdate merged = m_page_context->result.value_or(NodeStateUpdate {});
        if (update.progress.has_value()) {
            merged.progress = update.progress;
        }
        if (update.actual_type.has_value()) {
            merged.actual_type = update.actual_type;
            m_page_context->node_type = *update.actual_type;
        }
        if (update.actual_name.has_value()) {
            merged.actual_name = update.actual_name;
            m_page_context->node_name = *update.actual_name;
        }
        if (update.identity_revealed.has_value()) {
            merged.identity_revealed = update.identity_revealed;
        }
        if (update.repeatable.has_value()) {
            merged.repeatable = update.repeatable;
        }
        if (update.becomes_empty.has_value()) {
            merged.becomes_empty = update.becomes_empty;
        }
        m_page_context->result = std::move(merged);
    }

    if (result.kind == NodeTaskResultKind::PageCompleted) {
        if (!m_transaction->mark_page_resolved(error)) {
            return false;
        }
        m_page_context->stage = PageExecutionStage::Resolved;
        m_movement_inventory_refresh_required = true;

        queue_node_resolution(*m_page_context);
        m_page_context->resolution_reported = true;
    }
    refresh_mission();
    evaluate_terminal_rules();

    if (result.terminate && !m_result.has_value()) {
        const int cultivated = static_cast<int>(integer_fact(m_facts.merged(), "cultivated_animals"));
        m_result = BlackFlowStrategyResult {
            m_profile, result.outcome_code, result.termination_reason, std::clamp(cultivated, 0, 3), result.succeeded,
        };
        if (!result.succeeded && m_policy.has_value()) {
            m_result->next_action = m_policy->failure_action;
        }
    }
    return true;
}

void BlackFlowSession::fail(std::string outcome, std::string reason, FailureDisposition disposition)
{
    if (m_result.has_value()) {
        // 终局规则可能已写入结果（如培育完成）；后到的失败路径不得改写已定局的本局结局，
        // 与 apply_node_task_result 成功路径的 !m_result.has_value() 保护保持一致
        Log.warn(__FUNCTION__, "ignore failure; strategy result already present", outcome, reason);
        return;
    }
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
    m_result->next_action = disposition == FailureDisposition::RestartRun ? "restart_current_run" : "stop_run";
}

bool BlackFlowSession::claim_result_report() noexcept
{
    if (!m_result.has_value() || m_result_reported) {
        return false;
    }
    m_result_reported = true;
    return true;
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

} // namespace asst::blackflow
