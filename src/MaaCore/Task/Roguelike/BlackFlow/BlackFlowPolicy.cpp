#include "BlackFlowPolicy.h"

#include <algorithm>
#include <array>
#include <tuple>

namespace asst::blackflow
{
namespace
{
bool value_matches_type(FactType type, const FactValue& value)
{
    switch (type) {
    case FactType::Boolean:
        return std::holds_alternative<bool>(value);
    case FactType::Integer:
        return std::holds_alternative<std::int64_t>(value);
    case FactType::String:
        return std::holds_alternative<std::string>(value);
    case FactType::StringList:
        return std::holds_alternative<std::vector<std::string>>(value);
    }
    return false;
}

bool equal_values(const FactValue& lhs, const FactValue& rhs)
{
    return lhs.index() == rhs.index() && lhs == rhs;
}

bool ordered_compare(const FactValue& lhs, const FactValue& rhs, CompareOperator operation)
{
    if (!std::holds_alternative<std::int64_t>(lhs) || !std::holds_alternative<std::int64_t>(rhs)) {
        return false;
    }
    const auto left = std::get<std::int64_t>(lhs);
    const auto right = std::get<std::int64_t>(rhs);
    switch (operation) {
    case CompareOperator::Less:
        return left < right;
    case CompareOperator::LessEqual:
        return left <= right;
    case CompareOperator::Greater:
        return left > right;
    case CompareOperator::GreaterEqual:
        return left >= right;
    default:
        return false;
    }
}

bool contains_value(const FactValue& container, const FactValue& expected)
{
    if (!std::holds_alternative<std::string>(expected)) {
        return false;
    }
    const auto& needle = std::get<std::string>(expected);
    if (std::holds_alternative<std::string>(container)) {
        return std::get<std::string>(container).find(needle) != std::string::npos;
    }
    if (std::holds_alternative<std::vector<std::string>>(container)) {
        const auto& values = std::get<std::vector<std::string>>(container);
        return std::ranges::find(values, needle) != values.end();
    }
    return false;
}

std::int64_t movement_count(const RunState& state, MovementKind kind)
{
    const auto found = state.resources.movement_charges.find(kind);
    return found == state.resources.movement_charges.end() ? 0 : std::max(found->second, 0);
}

bool prerequisites_satisfied(const Milestone& milestone, const MissionState& mission)
{
    return std::ranges::all_of(milestone.prerequisites, [&](const std::string& id) {
        return mission.status(id) == MilestoneStatus::Satisfied;
    });
}

struct RankedCandidate
{
    const PolicyCandidate* candidate = nullptr;
    std::vector<int> score;
};

bool score_less(const RankedCandidate& lhs, const RankedCandidate& rhs)
{
    return std::lexicographical_compare(lhs.score.begin(), lhs.score.end(), rhs.score.begin(), rhs.score.end());
}
} // namespace

void FactStore::set(std::string key, FactValue value)
{
    m_values.insert_or_assign(std::move(key), std::move(value));
}

bool FactStore::erase(std::string_view key)
{
    return m_values.erase(std::string(key)) != 0;
}

void FactStore::clear() noexcept
{
    m_values.clear();
}

const FactValue* FactStore::find(std::string_view key) const noexcept
{
    const auto found = m_values.find(std::string(key));
    return found == m_values.end() ? nullptr : &found->second;
}

FactStore FactStore::overlay(const FactStore& higher_priority) const
{
    FactStore result = *this;
    for (const auto& [key, value] : higher_priority.m_values) {
        result.m_values.insert_or_assign(key, value);
    }
    return result;
}

bool FactContext::define(FactDefinition definition, std::string* error)
{
    if (definition.name.empty() || m_definitions.contains(definition.name)) {
        if (error != nullptr) {
            *error = "fact definition is empty or duplicated";
        }
        return false;
    }
    m_definitions.emplace(definition.name, std::move(definition));
    return true;
}

bool FactContext::set(FactScope scope, std::string key, FactValue value, std::string* error)
{
    const auto definition = m_definitions.find(key);
    if (definition == m_definitions.end()) {
        if (error != nullptr) {
            *error = "attempted to set an undeclared fact: " + key;
        }
        return false;
    }
    if (definition->second.scope != scope || !value_matches_type(definition->second.type, value)) {
        if (error != nullptr) {
            *error = "fact scope or value type differs from its declaration: " + key;
        }
        return false;
    }
    store(scope).set(std::move(key), std::move(value));
    return true;
}

const FactValue* FactContext::find(std::string_view key) const noexcept
{
    for (const FactScope scope : { FactScope::Candidate, FactScope::Page, FactScope::Floor, FactScope::Run }) {
        if (const FactValue* value = store(scope).find(key); value != nullptr) {
            return value;
        }
    }
    return nullptr;
}

const FactValue* FactContext::find(FactScope scope, std::string_view key) const noexcept
{
    return store(scope).find(key);
}

FactStore FactContext::merged() const
{
    return m_run.overlay(m_floor).overlay(m_page).overlay(m_candidate);
}

void FactContext::clear_scope(FactScope scope) noexcept
{
    store(scope).clear();
}

void FactContext::begin_run() noexcept
{
    m_run.clear();
    m_floor.clear();
    m_page.clear();
    m_candidate.clear();
}

void FactContext::begin_floor() noexcept
{
    m_floor.clear();
    m_page.clear();
    m_candidate.clear();
}

void FactContext::begin_page() noexcept
{
    m_page.clear();
    m_candidate.clear();
}

void FactContext::begin_candidate() noexcept
{
    m_candidate.clear();
}

FactStore& FactContext::store(FactScope scope) noexcept
{
    switch (scope) {
    case FactScope::Run:
        return m_run;
    case FactScope::Floor:
        return m_floor;
    case FactScope::Page:
        return m_page;
    case FactScope::Candidate:
        return m_candidate;
    }
    return m_run;
}

const FactStore& FactContext::store(FactScope scope) const noexcept
{
    switch (scope) {
    case FactScope::Run:
        return m_run;
    case FactScope::Floor:
        return m_floor;
    case FactScope::Page:
        return m_page;
    case FactScope::Candidate:
        return m_candidate;
    }
    return m_run;
}

bool Condition::evaluate(const FactStore& facts) const
{
    switch (kind) {
    case ConditionKind::Constant:
        return constant;
    case ConditionKind::All:
        return std::ranges::all_of(children, [&](const Condition& child) { return child.evaluate(facts); });
    case ConditionKind::Any:
        return std::ranges::any_of(children, [&](const Condition& child) { return child.evaluate(facts); });
    case ConditionKind::Not:
        return children.size() == 1 && !children.front().evaluate(facts);
    case ConditionKind::Predicate:
        break;
    }

    const FactValue* actual = facts.find(fact);
    if (compare == CompareOperator::Exists) {
        return actual != nullptr;
    }
    if (compare == CompareOperator::NotExists) {
        return actual == nullptr;
    }
    if (actual == nullptr || !value.has_value()) {
        return false;
    }
    switch (compare) {
    case CompareOperator::Equal:
        return equal_values(*actual, *value);
    case CompareOperator::NotEqual:
        return !equal_values(*actual, *value);
    case CompareOperator::Less:
    case CompareOperator::LessEqual:
    case CompareOperator::Greater:
    case CompareOperator::GreaterEqual:
        return ordered_compare(*actual, *value, compare);
    case CompareOperator::Contains:
        return contains_value(*actual, *value);
    case CompareOperator::Exists:
    case CompareOperator::NotExists:
        return false;
    }
    return false;
}

MilestoneStatus MissionState::status(std::string_view id) const noexcept
{
    const auto found = milestones.find(std::string(id));
    return found == milestones.end() ? MilestoneStatus::Inactive : found->second;
}

void MissionState::set_status(std::string id, MilestoneStatus status_value)
{
    milestones.insert_or_assign(std::move(id), status_value);
}

void MissionState::refresh(const std::vector<Milestone>& definitions, int floor, const FactStore& facts)
{
    viability = MissionViability::Possible;
    for (const auto& milestone : definitions) {
        const MilestoneStatus previous = status(milestone.id);
        if (previous == MilestoneStatus::Satisfied || previous == MilestoneStatus::Impossible) {
            if (previous == MilestoneStatus::Impossible && milestone.kind == MilestoneKind::Mandatory) {
                viability = MissionViability::Impossible;
            }
            continue;
        }
        if (milestone.complete_if.evaluate(facts)) {
            set_status(milestone.id, MilestoneStatus::Satisfied);
            continue;
        }
        if (!prerequisites_satisfied(milestone, *this)) {
            const bool prerequisite_failed = std::ranges::any_of(milestone.prerequisites, [&](const std::string& id) {
                const auto value = status(id);
                return value == MilestoneStatus::Missed || value == MilestoneStatus::Impossible;
            });
            set_status(milestone.id, prerequisite_failed ? MilestoneStatus::Impossible : MilestoneStatus::Inactive);
        }
        else if (floor > milestone.floor_end) {
            set_status(milestone.id, MilestoneStatus::Missed);
        }
        else if (floor >= milestone.floor_begin && milestone.active_if.evaluate(facts)) {
            set_status(milestone.id, MilestoneStatus::Available);
        }
        else {
            set_status(milestone.id, MilestoneStatus::Inactive);
        }
        const auto current = status(milestone.id);
        if (milestone.kind == MilestoneKind::Mandatory &&
            (current == MilestoneStatus::Missed || current == MilestoneStatus::Impossible)) {
            viability = MissionViability::Impossible;
        }
    }
    if (viability != MissionViability::Impossible && std::ranges::all_of(definitions, [&](const Milestone& milestone) {
            return milestone.kind != MilestoneKind::Mandatory || status(milestone.id) == MilestoneStatus::Satisfied;
        })) {
        viability = MissionViability::Confirmed;
    }
}

ResourceRegistry::ResourceRegistry()
{
    register_resource("action_points", [](const RunState& state) { return state.resources.action_points; });
    register_resource("hope", [](const RunState& state) { return state.resources.hope; });
    register_resource("ingots", [](const RunState& state) { return state.resources.ingots; });
    register_resource("seeds", [](const RunState& state) { return state.resources.seeds; });
    register_resource("sellable_scraps", [](const RunState& state) { return state.resources.sellable_scraps; });
    register_resource("white_model_bird", [](const RunState& state) { return state.resources.white_model_birds; });
    register_resource("painted_liberi", [](const RunState& state) { return state.resources.painted_liberi ? 1 : 0; });
    register_resource("persistent_full_map_movement", [](const RunState& state) {
        std::int64_t total = 0;
        for (const auto& spec : movement_specs()) {
            if (spec.range == MovementRange::FullMap && !spec.expires_on_floor_end) {
                total += movement_count(state, spec.kind);
            }
        }
        return total;
    });
    register_resource("persistent_long_range_movement", [](const RunState& state) {
        std::int64_t total = 0;
        for (const auto& spec : movement_specs()) {
            const bool long_range =
                spec.range == MovementRange::FullMap || spec.range == MovementRange::OrthogonalThree;
            if (long_range && !spec.expires_on_floor_end) {
                total += movement_count(state, spec.kind);
            }
        }
        return total;
    });
    for (const auto& spec : movement_specs()) {
        if (spec.kind == MovementKind::Walk) {
            continue;
        }
        const MovementKind kind = spec.kind;
        register_resource(std::string(spec.id), [kind](const RunState& state) { return movement_count(state, kind); });
    }
}

bool ResourceRegistry::register_resource(std::string id, Reader reader)
{
    return !id.empty() && static_cast<bool>(reader) && m_readers.emplace(std::move(id), std::move(reader)).second;
}

bool ResourceRegistry::contains(std::string_view id) const noexcept
{
    return m_readers.contains(std::string(id));
}

std::optional<std::int64_t> ResourceRegistry::read(std::string_view id, const RunState& state) const
{
    const auto found = m_readers.find(std::string(id));
    return found == m_readers.end() ? std::nullopt : std::optional<std::int64_t>(found->second(state));
}

std::optional<std::int64_t>
    ResourceRegistry::read_after(std::string_view id, const RunState& state, const MoveCandidate& candidate) const
{
    const auto before = read(id, state);
    if (!before.has_value()) {
        return std::nullopt;
    }
    std::int64_t after = *before;
    if (id == "action_points") {
        after = action_points_after(
            static_cast<int>(*before),
            candidate.predicted_action_point_cost,
            candidate.predicted_action_point_gain);
    }
    if (candidate.movement != MovementKind::Walk) {
        const MovementSpec* movement = find_movement_spec(candidate.movement);
        if (movement != nullptr) {
            if (id == movement->id) {
                --after;
            }
            const bool persistent_full_map =
                movement->range == MovementRange::FullMap && !movement->expires_on_floor_end;
            const bool persistent_long_range =
                (movement->range == MovementRange::FullMap || movement->range == MovementRange::OrthogonalThree) &&
                !movement->expires_on_floor_end;
            if (id == "persistent_full_map_movement" && persistent_full_map) {
                --after;
            }
            if (id == "persistent_long_range_movement" && persistent_long_range) {
                --after;
            }
        }
    }
    return after;
}

bool rule_is_active(const PolicyRule& rule, const FactStore& facts)
{
    return rule.when.evaluate(facts);
}

bool rule_matches_candidate(const PolicyRule& rule, const FactStore& facts, const FactStore& candidate_facts)
{
    return rule.target.evaluate(facts.overlay(candidate_facts));
}

bool milestone_is_active(
    const Milestone& milestone,
    int floor,
    const FactStore& facts,
    const MissionState& mission_state)
{
    return floor >= milestone.floor_begin && floor <= milestone.floor_end &&
           mission_state.status(milestone.id) == MilestoneStatus::Available &&
           prerequisites_satisfied(milestone, mission_state) && milestone.active_if.evaluate(facts);
}

bool milestone_is_complete(const Milestone& milestone, const FactStore& facts)
{
    return milestone.complete_if.evaluate(facts);
}

bool milestone_matches_candidate(const Milestone& milestone, const FactStore& facts, const FactStore& candidate_facts)
{
    return milestone.target.evaluate(facts.overlay(candidate_facts));
}

PolicyDecision PolicyExecutor::choose(
    const ResolvedPolicy& policy,
    const FactStore& facts,
    const MissionState& mission,
    const RunState& run,
    const ResourceRegistry& resources,
    const std::vector<PolicyCandidate>& candidates) const
{
    PolicyDecision decision;
    std::vector<const PolicyCandidate*> eligible;
    for (const auto& candidate : candidates) {
        if (!candidate.legal) {
            decision.rejected.emplace_back(candidate.move.action_id + ": illegal");
        }
        else if (!candidate.confirmed_safe) {
            decision.rejected.emplace_back(candidate.move.action_id + ": no confirmed safe terminal route");
        }
        else {
            eligible.emplace_back(&candidate);
        }
    }

    std::vector<const PolicyRule*> active_rules;
    for (const auto& rule : policy.rules) {
        if (rule_is_active(rule, facts)) {
            active_rules.emplace_back(&rule);
        }
    }
    std::ranges::sort(active_rules, [](const PolicyRule* lhs, const PolicyRule* rhs) {
        return std::tie(lhs->tier, lhs->rank, lhs->id) < std::tie(rhs->tier, rhs->rank, rhs->id);
    });

    std::erase_if(eligible, [&](const PolicyCandidate* candidate) {
        for (const PolicyRule* rule : active_rules) {
            const bool matches = rule_matches_candidate(*rule, facts, candidate->facts);
            if ((rule->kind == RuleKind::Forbid && matches) || (rule->kind == RuleKind::Require && !matches)) {
                decision.rejected.emplace_back(candidate->move.action_id + ": hard policy " + rule->id);
                return true;
            }
        }
        return false;
    });

    const Milestone* mandatory = nullptr;
    for (const auto& milestone : policy.milestones) {
        if (milestone.kind == MilestoneKind::Mandatory && milestone_is_active(milestone, run.floor, facts, mission) &&
            (mandatory == nullptr ||
             std::tie(milestone.rank, milestone.id) < std::tie(mandatory->rank, mandatory->id))) {
            mandatory = &milestone;
        }
    }
    if (mandatory != nullptr) {
        std::erase_if(eligible, [&](const PolicyCandidate* candidate) {
            if (milestone_matches_candidate(*mandatory, facts, candidate->facts)) {
                return false;
            }
            decision.rejected.emplace_back(candidate->move.action_id + ": mandatory milestone " + mandatory->id);
            return true;
        });
    }

    std::erase_if(eligible, [&](const PolicyCandidate* candidate) {
        for (const auto& reserve : policy.reserves) {
            const FactStore reserve_facts = facts.overlay(candidate->facts);
            if (!reserve.active_if.evaluate(facts) || reserve.release_if.evaluate(reserve_facts)) {
                continue;
            }
            const auto after = resources.read_after(reserve.resource, run, candidate->move);
            if (!after.has_value() || *after < reserve.minimum) {
                decision.rejected.emplace_back(candidate->move.action_id + ": resource reserve " + reserve.id);
                return true;
            }
        }
        return false;
    });

    if (eligible.empty()) {
        decision.reason = mandatory == nullptr ? "no eligible safe candidate"
                                               : "no candidate can advance the active mandatory milestone";
        return decision;
    }

    std::vector<const Milestone*> preferred;
    for (const auto& milestone : policy.milestones) {
        if (milestone.kind != MilestoneKind::Mandatory && milestone_is_active(milestone, run.floor, facts, mission)) {
            preferred.emplace_back(&milestone);
        }
    }
    std::ranges::sort(preferred, [](const Milestone* lhs, const Milestone* rhs) {
        return std::tie(lhs->rank, lhs->id) < std::tie(rhs->rank, rhs->id);
    });

    static constexpr std::array SoftTiers = {
        PolicyTier::Legality,
        PolicyTier::Safety,
        PolicyTier::MandatoryMilestone,
        PolicyTier::ResourceReserve,
        PolicyTier::PreferredMilestone,
        PolicyTier::Development,
        PolicyTier::Risk,
        PolicyTier::TieBreak,
    };
    std::vector<RankedCandidate> ranked;
    for (const PolicyCandidate* candidate : eligible) {
        RankedCandidate entry;
        entry.candidate = candidate;
        for (const PolicyTier tier : SoftTiers) {
            for (const PolicyRule* rule : active_rules) {
                if (rule->kind == RuleKind::Prefer && rule->tier == tier) {
                    entry.score.emplace_back(rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1);
                }
            }
            if (tier == PolicyTier::PreferredMilestone) {
                for (const Milestone* milestone : preferred) {
                    entry.score.emplace_back(milestone_matches_candidate(*milestone, facts, candidate->facts) ? 0 : 1);
                }
            }
            else if (tier == PolicyTier::Development) {
                entry.score.emplace_back(candidate->development_score);
            }
            else if (tier == PolicyTier::Risk) {
                entry.score.emplace_back(candidate->risk_score);
            }
            else if (tier == PolicyTier::TieBreak) {
                for (const PolicyRule* rule : active_rules) {
                    if (rule->kind == RuleKind::TieBreak) {
                        entry.score.emplace_back(rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1);
                    }
                }
            }
        }
        entry.score.emplace_back(candidate->battle_count);
        entry.score.emplace_back(candidate->estimated_duration);
        ranked.emplace_back(std::move(entry));
    }
    std::ranges::stable_sort(ranked, score_less);
    decision.selected = ranked.front().candidate->move;
    decision.reason = mandatory == nullptr ? "selected by lexicographic policy order"
                                           : "selected for mandatory milestone " + mandatory->id;
    return decision;
}

std::string_view to_string(RuleKind kind) noexcept
{
    switch (kind) {
    case RuleKind::Forbid:
        return "forbid";
    case RuleKind::Require:
        return "require";
    case RuleKind::Prefer:
        return "prefer";
    case RuleKind::TieBreak:
        return "tie_break";
    }
    return "prefer";
}

std::string_view to_string(PolicyTier tier) noexcept
{
    switch (tier) {
    case PolicyTier::Legality:
        return "legality";
    case PolicyTier::Safety:
        return "safety";
    case PolicyTier::MandatoryMilestone:
        return "mandatory_milestone";
    case PolicyTier::ResourceReserve:
        return "resource_reserve";
    case PolicyTier::PreferredMilestone:
        return "preferred_milestone";
    case PolicyTier::Development:
        return "development";
    case PolicyTier::Risk:
        return "risk";
    case PolicyTier::TieBreak:
        return "tie_break";
    }
    return "development";
}
} // namespace asst::blackflow

