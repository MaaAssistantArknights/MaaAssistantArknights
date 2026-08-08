#include "BlackFlowPolicy.h"

#include <algorithm>
#include <iterator>
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
    std::vector<std::string> milestone_ids;
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

bool NodeSelector::empty() const noexcept
{
    return node_types.empty() && node_names.empty() && marker_types.empty() && !badged.has_value() &&
           !identity_state.has_value() && !identity_revealed.has_value();
}

bool NodeSelector::matches(const Node& node) const noexcept
{
    if (!node_types.empty() && std::ranges::find(node_types, node.type) == node_types.end()) {
        return false;
    }
    if (!node_names.empty() && std::ranges::find(node_names, node.name) == node_names.end()) {
        return false;
    }
    if (!marker_types.empty() && std::ranges::find(marker_types, node.marker_type) == marker_types.end()) {
        return false;
    }
    if (badged.has_value() && node.badged != *badged) {
        return false;
    }
    if (identity_state.has_value() && node.identity_state != *identity_state) {
        return false;
    }
    if (identity_revealed.has_value() && node.identity_revealed != *identity_revealed) {
        return false;
    }
    return !empty();
}

MilestoneStatus MissionState::status(std::string_view id) const noexcept
{
    const auto found = milestones.find(std::string(id));
    return found == milestones.end() ? MilestoneStatus::Inactive : found->second;
}

int MissionState::progress(std::string_view id) const noexcept
{
    const auto found = milestone_progress.find(std::string(id));
    return found == milestone_progress.end() ? 0 : std::max(found->second, 0);
}

void MissionState::set_status(std::string id, MilestoneStatus status_value)
{
    milestones.insert_or_assign(std::move(id), status_value);
}

void MissionState::set_progress(std::string id, int value)
{
    milestone_progress.insert_or_assign(std::move(id), std::max(value, 0));
}

bool MissionState::record_node(const std::vector<Milestone>& definitions, const FactStore& facts, const Node& node)
{
    bool changed = false;
    for (const Milestone& milestone : definitions) {
        if (milestone.completion != MilestoneCompletion::VisitCount ||
            !milestone_is_active(milestone, node.floor, facts, *this) || !milestone_matches_node(milestone, node)) {
            continue;
        }
        auto& counted = milestone_nodes[milestone.id];
        if (!counted.emplace(node.id).second) {
            continue;
        }
        const int next = std::min(milestone.required_count, progress(milestone.id) + 1);
        if (next != progress(milestone.id)) {
            set_progress(milestone.id, next);
            changed = true;
        }
    }
    if (changed) {
        refresh(definitions, node.floor, facts);
    }
    return changed;
}

void MissionState::refresh(const std::vector<Milestone>& definitions, int floor, const FactStore& facts)
{
    for (std::size_t pass = 0; pass <= definitions.size(); ++pass) {
        bool changed = false;
        for (const Milestone& milestone : definitions) {
            const MilestoneStatus previous = status(milestone.id);
            if (previous == MilestoneStatus::Satisfied || previous == MilestoneStatus::Missed ||
                previous == MilestoneStatus::Impossible) {
                continue;
            }

            MilestoneStatus next = MilestoneStatus::Inactive;
            const bool completed = milestone.completion == MilestoneCompletion::Condition
                                       ? milestone.complete_if.evaluate(facts)
                                       : progress(milestone.id) >= milestone.required_count;
            if (completed) {
                next = MilestoneStatus::Satisfied;
            }
            else if (!prerequisites_satisfied(milestone, *this)) {
                const bool prerequisite_failed =
                    std::ranges::any_of(milestone.prerequisites, [&](const std::string& id) {
                        const MilestoneStatus value = status(id);
                        return value == MilestoneStatus::Missed || value == MilestoneStatus::Impossible;
                    });
                next = prerequisite_failed ? MilestoneStatus::Impossible : MilestoneStatus::Inactive;
            }
            else if (floor > milestone.floor_end) {
                next = MilestoneStatus::Missed;
            }
            else if (floor >= milestone.floor_begin && milestone.active_if.evaluate(facts)) {
                next = MilestoneStatus::Available;
            }
            if (next != previous) {
                set_status(milestone.id, next);
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    // 错过一条里程碑只说明这一层没按计划走，不代表整局作废：真正的失败判据由策略的
    // terminal_rules 与里程碑自己的 on_miss 声明，那里才能区分「这局不值得再打」和
    // 「继续按剩下的目标走」。
    const bool binding_complete = std::ranges::all_of(definitions, [&](const Milestone& milestone) {
        return !milestone.binding_candidate() || status(milestone.id) == MilestoneStatus::Satisfied;
    });
    viability = binding_complete ? MissionViability::Confirmed : MissionViability::Possible;
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
    return rule.candidate_if.evaluate(facts.overlay(candidate_facts));
}

bool milestone_matches_node(const Milestone& milestone, const Node& node) noexcept
{
    return node.floor >= milestone.floor_begin && node.floor <= milestone.floor_end && milestone.selector.matches(node);
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

PolicyDecision PolicyExecutor::choose(
    const ResolvedPolicy& policy,
    const FactStore& facts,
    const MissionState& mission,
    const RunState& run,
    const ResourceRegistry& resources,
    const std::unordered_set<std::string>& binding_milestone_ids,
    const std::vector<PolicyCandidate>& candidates) const
{
    PolicyDecision decision;
    decision.total_candidates = candidates.size();
    const bool minimize_intermediate_interactions =
        std::ranges::find(policy.route_preferences, RoutePreference::MinimizeIntermediateInteractions) !=
        policy.route_preferences.end();
    auto reject = [&](const PolicyCandidate& candidate, std::string category, std::string detail) {
        ++decision.rejection_counts[category];
        decision.rejected.emplace_back(candidate.move.action_id + ": " + std::move(detail));
    };
    auto category_for_tier = [](PolicyTier tier) {
        switch (tier) {
        case PolicyTier::StrategyConstraint:
            return DecisionReasonCategory::StrategyEnd;
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
        else if (!candidate.safe) {
            reject(candidate, "safety", "no safe terminal route");
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

    // 资源预留是硬过滤：一条预留若把候选全部毙光，这一拍就等于无路可走。手上只剩最后一件、
    // 而它正是唯一出路时就会这样。
    //
    // 处置与里程碑的可行性阶梯同一个原则：先证明「加上这条约束仍有候选」，证不出就放弃这条预留。
    // 逐条按声明顺序施加，因此先声明的预留优先保住；被放弃的记进 released_reserve_ids 供诊断。
    const ResourceReserve* decisive_reserve = nullptr;
    for (const auto& reserve : policy.reserves) {
        if (!reserve.active_if.evaluate(facts)) {
            continue;
        }
        std::vector<const PolicyCandidate*> kept;
        std::vector<const PolicyCandidate*> dropped;
        kept.reserve(eligible.size());
        for (const PolicyCandidate* candidate : eligible) {
            const FactStore reserve_facts = facts.overlay(candidate->facts);
            if (reserve.release_if.evaluate(reserve_facts)) {
                kept.emplace_back(candidate);
                continue;
            }
            const auto after = resources.read_after(reserve.resource, run, candidate->move);
            if (!after.has_value() || *after < reserve.minimum) {
                dropped.emplace_back(candidate);
            }
            else {
                kept.emplace_back(candidate);
            }
        }
        if (kept.empty()) {
            decision.released_reserve_ids.emplace_back(reserve.id);
            continue;
        }
        for (const PolicyCandidate* candidate : dropped) {
            reject(*candidate, "resource_reserve", "resource reserve " + reserve.id);
        }
        if (!dropped.empty() && decisive_reserve == nullptr) {
            decisive_reserve = &reserve;
        }
        eligible = std::move(kept);
    }

    decision.eligible_candidates = eligible.size();
    if (eligible.empty()) {
        decision.reason = "no eligible safe candidate";
        return decision;
    }

    const auto is_binding = [&](const Milestone& milestone) {
        return binding_milestone_ids.contains(milestone.id);
    };

    std::vector<const Milestone*> active_milestones;
    for (const auto& milestone : policy.milestones) {
        const MilestoneStatus status = mission.status(milestone.id);
        if (run.floor >= milestone.floor_begin && run.floor <= milestone.floor_end &&
            status != MilestoneStatus::Satisfied && status != MilestoneStatus::Missed &&
            status != MilestoneStatus::Impossible && milestone.active_if.evaluate(facts)) {
            active_milestones.emplace_back(&milestone);
        }
    }
    // 已锁定的目标排在最前，页面意图也按这个顺序取第一条匹配的，因此硬目标的意图优先于顺路目标。
    std::ranges::sort(active_milestones, [&](const Milestone* lhs, const Milestone* rhs) {
        const bool lhs_binding = is_binding(*lhs);
        const bool rhs_binding = is_binding(*rhs);
        if (lhs_binding != rhs_binding) {
            return lhs_binding;
        }
        return std::tie(lhs->kind, lhs->rank, lhs->id) < std::tie(rhs->kind, rhs->rank, rhs->id);
    });

    const auto milestone_value = [&](const PolicyCandidate& current, const Milestone& milestone) {
        const auto planned = current.milestone_progress.find(milestone.id);
        if (planned != current.milestone_progress.end()) {
            return std::min(milestone.required_count, planned->second);
        }
        return std::min(milestone.required_count, mission.progress(milestone.id));
    };

    std::vector<RankedCandidate> ranked;
    for (const PolicyCandidate* candidate : eligible) {
        RankedCandidate entry;
        entry.candidate = candidate;
        auto add_score = [&](int value,
                             DecisionReasonCategory category,
                             std::string id,
                             std::vector<std::string> milestone_ids = {}) {
            entry.score.emplace_back(value);
            entry.origins.emplace_back(ScoreOrigin { category, std::move(id), std::move(milestone_ids) });
        };
        // 已锁定的目标占字典序最高位。可行性阶梯已经证明过“锁定之后仍然有安全解”，
        // 所以这里可以放心地让它压过一切顺路收益。
        auto append_binding_groups = [&] {
            std::vector<const Milestone*> bindings;
            std::ranges::copy_if(active_milestones, std::back_inserter(bindings), [&](const Milestone* milestone) {
                return is_binding(*milestone);
            });
            std::ranges::sort(bindings, [](const Milestone* lhs, const Milestone* rhs) {
                return std::tie(lhs->rank, lhs->id) < std::tie(rhs->rank, rhs->id);
            });
            std::size_t begin = 0;
            while (begin < bindings.size()) {
                const int rank = bindings[begin]->rank;
                std::size_t end = begin;
                int completed = 0;
                int progress_sum = 0;
                while (end < bindings.size() && bindings[end]->rank == rank) {
                    const Milestone& milestone = *bindings[end];
                    const int value = milestone_value(*candidate, milestone);
                    completed += value >= milestone.required_count ? 1 : 0;
                    progress_sum += milestone.weight * value;
                    ++end;
                }
                std::vector<std::string> group_milestone_ids;
                group_milestone_ids.reserve(end - begin);
                for (std::size_t index = begin; index < end; ++index) {
                    group_milestone_ids.emplace_back(bindings[index]->id);
                }
                add_score(-completed, DecisionReasonCategory::StrategyEnd, {}, group_milestone_ids);
                add_score(-progress_sum, DecisionReasonCategory::StrategyEnd, {}, group_milestone_ids);
                begin = end;
            }
        };
        // 软层按 kind 分档。已锁定的目标不参与，降级后的强制目标则按它自己的 kind 回到这里，
        // 于是“证不出可行”只让它从硬约束退成倾向，不会让它整个消失。
        auto append_milestone_groups = [&](MilestoneKind kind, DecisionReasonCategory category) {
            std::size_t begin = 0;
            while (begin < active_milestones.size()) {
                while (begin < active_milestones.size() &&
                       (active_milestones[begin]->kind != kind || is_binding(*active_milestones[begin]))) {
                    ++begin;
                }
                if (begin >= active_milestones.size()) {
                    break;
                }
                const int rank = active_milestones[begin]->rank;
                std::size_t end = begin;
                int progress_sum = 0;
                while (end < active_milestones.size() && active_milestones[end]->kind == kind &&
                       active_milestones[end]->rank == rank) {
                    const Milestone& milestone = *active_milestones[end];
                    progress_sum += milestone.weight * milestone_value(*candidate, milestone);
                    ++end;
                }
                const auto group_reward = [&](const PolicyCandidate& current) {
                    int value = 0;
                    for (std::size_t index = begin; index < end; ++index) {
                        const Milestone& milestone = *active_milestones[index];
                        value += milestone.weight * milestone_value(current, milestone);
                    }
                    return value;
                };
                const int reference = group_reward(*eligible.front());
                const bool varies = std::ranges::any_of(eligible, [&](const PolicyCandidate* other) {
                    return group_reward(*other) != reference;
                });
                if (varies) {
                    std::vector<std::string> group_milestone_ids;
                    group_milestone_ids.reserve(end - begin);
                    for (std::size_t index = begin; index < end; ++index) {
                        group_milestone_ids.emplace_back(active_milestones[index]->id);
                    }
                    add_score(
                        candidate->estimated_duration + candidate->battle_count + candidate->processing_move_count +
                            (minimize_intermediate_interactions ? candidate->intermediate_interaction_count : 0) -
                            progress_sum,
                        category,
                        {},
                        group_milestone_ids);
                    add_score(-progress_sum, category, {}, group_milestone_ids);
                }
                begin = end;
            }
        };

        append_binding_groups();
        for (const PolicyRule* rule : active_rules) {
            if (rule->kind == RuleKind::Prefer && rule->tier == PolicyTier::ResourceReserve) {
                add_score(
                    rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1,
                    DecisionReasonCategory::ResourceReserve,
                    rule->id);
            }
        }
        append_milestone_groups(MilestoneKind::Preferred, DecisionReasonCategory::PreferredGoal);
        append_milestone_groups(MilestoneKind::Opportunistic, DecisionReasonCategory::Development);
        for (const PolicyTier tier : { PolicyTier::Development, PolicyTier::Risk }) {
            for (const PolicyRule* rule : active_rules) {
                if ((rule->kind == RuleKind::Prefer || rule->kind == RuleKind::TieBreak) && rule->tier == tier) {
                    add_score(
                        rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1,
                        category_for_tier(tier),
                        rule->id);
                }
            }
            if (tier == PolicyTier::Development) {
                add_score(candidate->development_score, DecisionReasonCategory::Development, "development_score");
            }
        }
        add_score(candidate->battle_count, DecisionReasonCategory::RiskAvoidance, "battle_count");
        if (minimize_intermediate_interactions) {
            add_score(
                candidate->intermediate_interaction_count,
                DecisionReasonCategory::TieBreak,
                "intermediate_interaction_count");
        }
        add_score(candidate->estimated_duration, DecisionReasonCategory::TieBreak, "estimated_duration");
        add_score(candidate->processing_move_count, DecisionReasonCategory::ResourceReserve, "processing_move_count");
        add_score(candidate->risk_score, DecisionReasonCategory::RiskAvoidance, "risk_score");
        for (const PolicyRule* rule : active_rules) {
            if ((rule->kind == RuleKind::Prefer || rule->kind == RuleKind::TieBreak) &&
                rule->tier == PolicyTier::TieBreak) {
                add_score(
                    rule_matches_candidate(*rule, facts, candidate->facts) ? 0 : 1,
                    DecisionReasonCategory::TieBreak,
                    rule->id);
            }
        }
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

    // 页面意图不在这里决定。地图阶段只知道节点在地图上的身份，隐藏节点要进去才认得出，
    // 提前绑定会把秘境行商这类节点分流到通用页面。意图由 BlackFlowSession 在进入并分类之后解析。
    decision.selected = ranked.front().candidate->move;
    decision.planned_route = ranked.front().candidate->planned_route;
    decision.planned_route_steps = ranked.front().candidate->planned_route_steps;
    decision.planned_milestone_progress = ranked.front().candidate->milestone_progress;
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
            if (!origin.milestone_ids.empty()) {
                for (const std::string& milestone_id : origin.milestone_ids) {
                    const auto milestone = std::ranges::find_if(active_milestones, [&](const Milestone* value) {
                        return value->id == milestone_id;
                    });
                    if (milestone != active_milestones.end() &&
                        milestone_value(*ranked[0].candidate, **milestone) !=
                            milestone_value(*ranked[1].candidate, **milestone)) {
                        decision.decisive_milestone_ids.emplace_back(milestone_id);
                    }
                }
                if (decision.decisive_milestone_ids.size() == 1) {
                    decision.decisive_milestone_id = decision.decisive_milestone_ids.front();
                }
            }
            else {
                decision.decisive_rule_id = origin.id;
            }
            break;
        }
    }
    else if (decisive_reserve != nullptr) {
        decision.reason_category = DecisionReasonCategory::ResourceReserve;
        decision.decisive_rule_id = decisive_reserve->id;
    }
    else if (decisive_hard_rule != nullptr) {
        decision.reason_category = category_for_tier(decisive_hard_rule->tier);
        decision.decisive_rule_id = decisive_hard_rule->id;
    }
    else if (safe_count == 1 && candidates.size() > 1) {
        decision.reason_category = DecisionReasonCategory::SafetyFallback;
        decision.decisive_rule_id = "safety";
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
    case PolicyTier::StrategyConstraint:
        return "strategy_constraint";
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

std::string_view to_string(MilestoneStatus status) noexcept
{
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
}

std::string_view to_string(DecisionReasonCategory category) noexcept
{
    switch (category) {
    case DecisionReasonCategory::StrategyEnd:
        return "strategy_end";
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

