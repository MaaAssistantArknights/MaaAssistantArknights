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

struct ScoreOrigin
{
    DecisionReasonCategory category = DecisionReasonCategory::TieBreak;
    std::string id;
    bool milestone = false;
};

struct RankedCandidate
{
    const PolicyCandidate* candidate = nullptr;
    std::vector<int> score;
    std::vector<ScoreOrigin> origins;
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
    decision.total_candidates = candidates.size();
    auto reject = [&](const PolicyCandidate& candidate, std::string category, std::string detail) {
        ++decision.rejection_counts[category];
        decision.rejected.emplace_back(candidate.move.action_id + ": " + std::move(detail));
    };
    auto category_for_tier = [](PolicyTier tier) {
        switch (tier) {
        case PolicyTier::MandatoryMilestone:
            return DecisionReasonCategory::MandatoryGoal;
        case PolicyTier::ResourceReserve:
            return DecisionReasonCategory::ResourceReserve;
        case PolicyTier::PreferredMilestone:
            return DecisionReasonCategory::PreferredGoal;
        case PolicyTier::Development:
            return DecisionReasonCategory::Development;
        case PolicyTier::Risk:
            return DecisionReasonCategory::RiskAvoidance;
        case PolicyTier::Safety:
            return DecisionReasonCategory::SafetyFallback;
        case PolicyTier::Legality:
        case PolicyTier::TieBreak:
            return DecisionReasonCategory::TieBreak;
        }
        return DecisionReasonCategory::TieBreak;
    };

    std::vector<const PolicyCandidate*> eligible;
    for (const auto& candidate : candidates) {
        if (!candidate.legal) {
            reject(candidate, "legality", "illegal");
        }
        else if (!candidate.confirmed_safe) {
            reject(candidate, "safety", "no confirmed safe terminal route");
        }
        else {
            eligible.emplace_back(&candidate);
        }
    }
    const std::size_t safe_count = eligible.size();

    std::vector<const PolicyRule*> active_rules;
    for (const auto& rule : policy.rules) {
        if (rule_is_active(rule, facts)) {
            active_rules.emplace_back(&rule);
        }
    }
    std::ranges::sort(active_rules, [](const PolicyRule* lhs, const PolicyRule* rhs) {
        return std::tie(lhs->tier, lhs->rank, lhs->id) < std::tie(rhs->tier, rhs->rank, rhs->id);
    });

    const PolicyRule* decisive_hard_rule = nullptr;
    std::erase_if(eligible, [&](const PolicyCandidate* candidate) {
        for (const PolicyRule* rule : active_rules) {
            const bool matches = rule_matches_candidate(*rule, facts, candidate->facts);
            if ((rule->kind == RuleKind::Forbid && matches) || (rule->kind == RuleKind::Require && !matches)) {
                reject(*candidate, "hard_policy", "hard policy " + rule->id);
                decisive_hard_rule = decisive_hard_rule == nullptr ? rule : decisive_hard_rule;
                return true;
            }
        }
        return false;
    });
    const std::size_t after_hard_policy = eligible.size();

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
            reject(*candidate, "mandatory_goal", "mandatory milestone " + mandatory->id);
            return true;
        });
    }
    const std::size_t after_mandatory = eligible.size();

    const ResourceReserve* decisive_reserve = nullptr;
    std::erase_if(eligible, [&](const PolicyCandidate* candidate) {
        for (const auto& reserve : policy.reserves) {
            const FactStore reserve_facts = facts.overlay(candidate->facts);
            if (!reserve.active_if.evaluate(facts) || reserve.release_if.evaluate(reserve_facts)) {
                continue;
            }
            const auto after = resources.read_after(reserve.resource, run, candidate->move);
            if (!after.has_value() || *after < reserve.minimum) {
                reject(*candidate, "resource_reserve", "resource reserve " + reserve.id);
                decisive_reserve = decisive_reserve == nullptr ? &reserve : decisive_reserve;
                return true;
            }
        }
        return false;
    });

    decision.eligible_candidates = eligible.size();
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
        auto add_score = [&](int value, DecisionReasonCategory category, std::string id, bool milestone = false) {
            entry.score.emplace_back(value);
            entry.origins.emplace_back(ScoreOrigin { category, std::move(id), milestone });
        };
        for (const PolicyTier tier : SoftTiers) {
            for (const PolicyRule* rule : active_rules) {
                if (rule->kind == RuleKind::Prefer && rule->tier == tier) {
                    add_score(
                        rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1,
                        category_for_tier(tier),
                        rule->id);
                }
            }
            if (tier == PolicyTier::PreferredMilestone) {
                for (const Milestone* milestone : preferred) {
                    add_score(
                        milestone_matches_candidate(*milestone, facts, candidate->facts) ? 0 : 1,
                        DecisionReasonCategory::PreferredGoal,
                        milestone->id,
                        true);
                }
            }
            else if (tier == PolicyTier::Development) {
                add_score(candidate->development_score, DecisionReasonCategory::Development, "development_score");
            }
            else if (tier == PolicyTier::Risk) {
                add_score(candidate->risk_score, DecisionReasonCategory::RiskAvoidance, "risk_score");
            }
            else if (tier == PolicyTier::TieBreak) {
                for (const PolicyRule* rule : active_rules) {
                    if (rule->kind == RuleKind::TieBreak) {
                        add_score(
                            rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1,
                            DecisionReasonCategory::TieBreak,
                            rule->id);
                    }
                }
            }
        }
        add_score(candidate->battle_count, DecisionReasonCategory::RiskAvoidance, "battle_count");
        add_score(candidate->estimated_duration, DecisionReasonCategory::TieBreak, "estimated_duration");
        ranked.emplace_back(std::move(entry));
    }
    std::ranges::stable_sort(ranked, [](const RankedCandidate& lhs, const RankedCandidate& rhs) {
        if (score_less(lhs, rhs)) {
            return true;
        }
        if (score_less(rhs, lhs)) {
            return false;
        }
        return lhs.candidate->move.action_id < rhs.candidate->move.action_id;
    });

    decision.selected = ranked.front().candidate->move;
    for (std::size_t index = 1; index < std::min<std::size_t>(ranked.size(), 3); ++index) {
        decision.runners_up.emplace_back(ranked[index].candidate->move);
    }

    if (ranked.size() >= 2) {
        const std::size_t dimensions = std::min(ranked[0].score.size(), ranked[1].score.size());
        for (std::size_t index = 0; index < dimensions; ++index) {
            if (ranked[0].score[index] == ranked[1].score[index]) {
                continue;
            }
            const ScoreOrigin& origin = ranked[0].origins[index];
            decision.reason_category = origin.category;
            if (origin.milestone) {
                decision.decisive_milestone_id = origin.id;
            }
            else {
                decision.decisive_rule_id = origin.id;
            }
            break;
        }
    }
    else if (mandatory != nullptr && after_hard_policy > after_mandatory) {
        decision.reason_category = DecisionReasonCategory::MandatoryGoal;
        decision.decisive_milestone_id = mandatory->id;
    }
    else if (decisive_reserve != nullptr && after_mandatory > eligible.size()) {
        decision.reason_category = DecisionReasonCategory::ResourceReserve;
        decision.decisive_rule_id = decisive_reserve->id;
    }
    else if (decisive_hard_rule != nullptr) {
        decision.reason_category = category_for_tier(decisive_hard_rule->tier);
        decision.decisive_rule_id = decisive_hard_rule->id;
    }
    else if (safe_count == 1 && candidates.size() > 1) {
        decision.reason_category = DecisionReasonCategory::SafetyFallback;
        decision.decisive_rule_id = "confirmed_safety";
    }

    decision.reason = "selected by lexicographic policy order";
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

std::string_view to_string(DecisionReasonCategory category) noexcept
{
    switch (category) {
    case DecisionReasonCategory::MandatoryGoal:
        return "mandatory_goal";
    case DecisionReasonCategory::ResourceReserve:
        return "resource_reserve";
    case DecisionReasonCategory::PreferredGoal:
        return "preferred_goal";
    case DecisionReasonCategory::Development:
        return "development";
    case DecisionReasonCategory::RiskAvoidance:
        return "risk_avoidance";
    case DecisionReasonCategory::SafetyFallback:
        return "safety_fallback";
    case DecisionReasonCategory::TieBreak:
        return "tie_break";
    }
    return "tie_break";
}
} // namespace asst::blackflow

