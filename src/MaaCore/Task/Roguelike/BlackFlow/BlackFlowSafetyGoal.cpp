#include "BlackFlowSafetyGoal.h"

#include <algorithm>
#include <functional>
#include <map>
#include <ranges>
#include <tuple>
#include <unordered_set>

namespace asst::blackflow
{
namespace
{
void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}

std::size_t combine_hash(std::size_t seed, std::size_t value) noexcept
{
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}
} // namespace

std::optional<SafetyGoalProgram> SafetyGoalProgram::compile(
    const ResolvedPolicy& policy,
    const MissionState& mission,
    const FactStore& facts,
    std::string* error)
{
    return compile(policy.milestones, mission, facts, error);
}

std::optional<SafetyGoalProgram> SafetyGoalProgram::compile(
    const std::vector<Milestone>& milestones,
    const MissionState& mission,
    const FactStore& facts,
    std::string* error)
{
    if (error != nullptr) {
        error->clear();
    }

    std::unordered_map<std::string, const Milestone*> definitions;
    definitions.reserve(milestones.size());
    for (const Milestone& milestone : milestones) {
        if (milestone.id.empty()) {
            set_error(error, "mandatory safety goal contains an empty milestone id");
            return std::nullopt;
        }
        if (milestone.floor_begin > milestone.floor_end) {
            set_error(error, "mandatory safety goal has an invalid floor interval: " + milestone.id);
            return std::nullopt;
        }
        if (milestone.required_count < 0) {
            set_error(error, "mandatory safety goal has a negative required count: " + milestone.id);
            return std::nullopt;
        }
        if (!definitions.emplace(milestone.id, &milestone).second) {
            set_error(error, "mandatory safety goal contains duplicate milestone id: " + milestone.id);
            return std::nullopt;
        }
    }

    enum class VisitMark : std::uint8_t
    {
        Unseen,
        Visiting,
        Complete,
    };
    std::unordered_map<std::string, VisitMark> marks;
    std::unordered_set<std::string> selected_ids;
    std::function<bool(const std::string&)> include_with_prerequisites;
    include_with_prerequisites = [&](const std::string& id) {
        const auto definition = definitions.find(id);
        if (definition == definitions.end()) {
            if (mission.status(id) == MilestoneStatus::Satisfied) {
                return true;
            }
            set_error(error, "mandatory safety goal has an unresolved prerequisite: " + id);
            return false;
        }
        const VisitMark mark = marks[id];
        if (mark == VisitMark::Visiting) {
            set_error(error, "mandatory safety goal contains a prerequisite cycle at: " + id);
            return false;
        }
        if (mark == VisitMark::Complete) {
            return true;
        }
        marks[id] = VisitMark::Visiting;
        for (const std::string& prerequisite : definition->second->prerequisites) {
            if (!include_with_prerequisites(prerequisite)) {
                return false;
            }
        }
        marks[id] = VisitMark::Complete;
        selected_ids.emplace(id);
        return true;
    };

    for (const Milestone& milestone : milestones) {
        if (milestone.kind != MilestoneKind::Mandatory) {
            continue;
        }
        const MilestoneStatus status = mission.status(milestone.id);
        if (status == MilestoneStatus::Missed || status == MilestoneStatus::Impossible) {
            set_error(error, "mandatory safety goal is already failed: " + milestone.id);
            return std::nullopt;
        }
        if (!include_with_prerequisites(milestone.id)) {
            return std::nullopt;
        }
    }

    std::vector<const Milestone*> selected;
    selected.reserve(selected_ids.size());
    for (const std::string& id : selected_ids) {
        selected.emplace_back(definitions.at(id));
    }
    std::ranges::sort(selected, [](const Milestone* lhs, const Milestone* rhs) {
        return std::tie(lhs->kind, lhs->rank, lhs->id) < std::tie(rhs->kind, rhs->rank, rhs->id);
    });

    SafetyGoalProgram program;
    program.m_milestones.reserve(selected.size());
    program.m_milestone_info.reserve(selected.size());
    for (const Milestone* milestone : selected) {
        const std::size_t index = program.m_milestones.size();
        program.m_indices.emplace(milestone->id, index);
        const bool mandatory = milestone->kind == MilestoneKind::Mandatory;
        program.m_milestones.emplace_back(CompiledMilestone { *milestone, mandatory, !mandatory, {} });
        program.m_milestone_info.emplace_back(
            SafetyGoalMilestoneInfo {
                milestone->id,
                milestone->kind,
                milestone->completion,
                milestone->floor_begin,
                milestone->floor_end,
                milestone->rank,
                milestone->required_count,
                milestone->weight,
                mandatory,
                !mandatory,
            });
    }

    for (CompiledMilestone& milestone : program.m_milestones) {
        for (const std::string& prerequisite : milestone.definition.prerequisites) {
            const auto found = program.m_indices.find(prerequisite);
            if (found != program.m_indices.end()) {
                milestone.prerequisite_indices.emplace_back(found->second);
            }
            else if (mission.status(prerequisite) != MilestoneStatus::Satisfied) {
                set_error(error, "mandatory safety goal cannot track prerequisite: " + prerequisite);
                return std::nullopt;
            }
        }
        std::ranges::sort(milestone.prerequisite_indices);
    }

    std::map<std::pair<int, int>, std::vector<std::size_t>> phase_indices;
    for (std::size_t index = 0; index < program.m_milestones.size(); ++index) {
        const CompiledMilestone& milestone = program.m_milestones[index];
        if (milestone.mandatory) {
            phase_indices[{ milestone.definition.floor_end, milestone.definition.rank }].emplace_back(index);
        }
    }
    for (auto& [key, indices] : phase_indices) {
        program.m_phases.emplace_back(SafetyGoalPhase { key.first, key.second, std::move(indices) });
    }

    SafetyGoalProgressSnapshot initial;
    initial.progress.resize(program.m_milestones.size(), 0);
    initial.satisfied.resize(program.m_milestones.size(), 0);
    initial.counted_nodes.resize(program.m_milestones.size());
    for (std::size_t index = 0; index < program.m_milestones.size(); ++index) {
        const CompiledMilestone& milestone = program.m_milestones[index];
        const MilestoneStatus status = mission.status(milestone.definition.id);
        if (status == MilestoneStatus::Missed || status == MilestoneStatus::Impossible) {
            set_error(error, "mandatory safety prerequisite is already failed: " + milestone.definition.id);
            return std::nullopt;
        }
        initial.progress[index] =
            std::min(std::max(mission.progress(milestone.definition.id), 0), milestone.definition.required_count);
        const auto counted = mission.milestone_nodes.find(milestone.definition.id);
        if (counted != mission.milestone_nodes.end()) {
            initial.counted_nodes[index].assign(counted->second.begin(), counted->second.end());
            std::ranges::sort(initial.counted_nodes[index]);
            initial.counted_nodes[index].erase(
                std::unique(initial.counted_nodes[index].begin(), initial.counted_nodes[index].end()),
                initial.counted_nodes[index].end());
        }
        const bool completed_by_condition = milestone.definition.completion == MilestoneCompletion::Condition &&
                                            milestone.definition.complete_if.evaluate(facts);
        const bool completed_by_progress = milestone.definition.completion == MilestoneCompletion::VisitCount &&
                                           initial.progress[index] >= milestone.definition.required_count;
        initial.satisfied[index] = static_cast<std::uint8_t>(
            status == MilestoneStatus::Satisfied || completed_by_condition || completed_by_progress);
    }

    program.m_initial_progress_id = program.intern(std::move(initial));
    return program;
}

std::size_t SafetyGoalProgram::SnapshotHash::operator()(const SafetyGoalProgressSnapshot& value) const noexcept
{
    std::size_t seed = 0;
    for (const int progress : value.progress) {
        seed = combine_hash(seed, std::hash<int> {}(progress));
    }
    for (const std::uint8_t satisfied : value.satisfied) {
        seed = combine_hash(seed, std::hash<std::uint8_t> {}(satisfied));
    }
    for (const auto& counted : value.counted_nodes) {
        seed = combine_hash(seed, counted.size());
        for (const NodeId node : counted) {
            seed = combine_hash(seed, std::hash<NodeId> {}(node));
        }
    }
    return seed;
}

bool SafetyGoalProgram::valid_id(SafetyGoalProgressId id) const noexcept
{
    return id != InvalidSafetyGoalProgressId && static_cast<std::size_t>(id) < m_states.size();
}

bool SafetyGoalProgram::route_requirement_satisfied(const SafetyGoalProgressSnapshot& state, std::size_t index)
    const noexcept
{
    if (index >= m_milestones.size() || index >= state.progress.size() || index >= state.satisfied.size()) {
        return false;
    }
    const Milestone& milestone = m_milestones[index].definition;
    return state.satisfied[index] != 0 || state.progress[index] >= milestone.required_count;
}

const SafetyGoalProgressSnapshot* SafetyGoalProgram::progress(SafetyGoalProgressId id) const noexcept
{
    return valid_id(id) ? &m_states[id] : nullptr;
}

std::optional<std::size_t> SafetyGoalProgram::milestone_index(std::string_view id) const noexcept
{
    const auto found = m_indices.find(std::string(id));
    return found == m_indices.end() ? std::nullopt : std::optional<std::size_t>(found->second);
}

int SafetyGoalProgram::milestone_progress(SafetyGoalProgressId id, std::string_view milestone_id) const noexcept
{
    const auto index = milestone_index(milestone_id);
    return valid_id(id) && index.has_value() ? m_states[id].progress[*index] : 0;
}

bool SafetyGoalProgram::milestone_satisfied(SafetyGoalProgressId id, std::string_view milestone_id) const noexcept
{
    const auto index = milestone_index(milestone_id);
    return valid_id(id) && index.has_value() && m_states[id].satisfied[*index] != 0;
}

bool SafetyGoalProgram::prerequisites_satisfied(
    const SafetyGoalProgressSnapshot& state,
    const CompiledMilestone& milestone) const noexcept
{
    return std::ranges::all_of(milestone.prerequisite_indices, [&](const std::size_t index) {
        return route_requirement_satisfied(state, index);
    });
}

SafetyGoalProgressId SafetyGoalProgram::intern(SafetyGoalProgressSnapshot state)
{
    const auto found = m_state_ids.find(state);
    if (found != m_state_ids.end()) {
        return found->second;
    }
    const auto id = static_cast<SafetyGoalProgressId>(m_states.size());
    m_states.emplace_back(std::move(state));
    m_state_ids.emplace(m_states.back(), id);
    return id;
}

std::optional<SafetyGoalProgressId>
    SafetyGoalProgram::refresh_conditions(SafetyGoalProgressId id, const FactStore& facts, std::string* error)
{
    if (!valid_id(id)) {
        set_error(error, "mandatory safety goal references an invalid progress id");
        return std::nullopt;
    }
    SafetyGoalProgressSnapshot next = m_states[id];
    for (std::size_t index = 0; index < m_milestones.size(); ++index) {
        const Milestone& milestone = m_milestones[index].definition;
        if (next.satisfied[index] == 0 && milestone.completion == MilestoneCompletion::Condition &&
            milestone.complete_if.evaluate(facts)) {
            next.satisfied[index] = 1;
        }
    }
    return intern(std::move(next));
}

std::optional<SafetyGoalProgressId> SafetyGoalProgram::advance_node(
    SafetyGoalProgressId id,
    const Node& node,
    int unknown_nodes_revealed,
    const FactStore& facts,
    std::string* error)
{
    const auto refreshed = refresh_conditions(id, facts, error);
    if (!refreshed.has_value()) {
        return std::nullopt;
    }
    SafetyGoalProgressSnapshot next = m_states[*refreshed];
    const SafetyGoalProgressSnapshot before = next;
    for (std::size_t index = 0; index < m_milestones.size(); ++index) {
        const CompiledMilestone& compiled = m_milestones[index];
        const Milestone& milestone = compiled.definition;
        if (route_requirement_satisfied(before, index) || node.floor < milestone.floor_begin ||
            node.floor > milestone.floor_end || !milestone.active_if.evaluate(facts) ||
            !prerequisites_satisfied(before, compiled) || !milestone.selector.matches(node) ||
            milestone.minimum_unknown_nodes_revealed > unknown_nodes_revealed) {
            continue;
        }
        auto& counted = next.counted_nodes[index];
        const auto insertion = std::ranges::lower_bound(counted, node.id);
        if (insertion != counted.end() && *insertion == node.id) {
            continue;
        }
        counted.insert(insertion, node.id);
        next.progress[index] = std::min(milestone.required_count, before.progress[index] + 1);
        if (milestone.completion == MilestoneCompletion::VisitCount &&
            next.progress[index] >= milestone.required_count) {
            next.satisfied[index] = 1;
        }
    }
    return intern(std::move(next));
}

bool SafetyGoalProgram::mandatory_due_through_floor_satisfied(SafetyGoalProgressId id, int floor) const noexcept
{
    if (!valid_id(id)) {
        return false;
    }
    const SafetyGoalProgressSnapshot& state = m_states[id];
    for (std::size_t index = 0; index < m_milestones.size(); ++index) {
        if (m_milestones[index].mandatory && m_milestones[index].definition.floor_end <= floor &&
            !route_requirement_satisfied(state, index)) {
            return false;
        }
    }
    return true;
}

bool SafetyGoalProgram::all_mandatory_satisfied(SafetyGoalProgressId id) const noexcept
{
    if (!valid_id(id)) {
        return false;
    }
    const SafetyGoalProgressSnapshot& state = m_states[id];
    for (std::size_t index = 0; index < m_milestones.size(); ++index) {
        if (m_milestones[index].mandatory && !route_requirement_satisfied(state, index)) {
            return false;
        }
    }
    return true;
}

bool SafetyGoalProgram::is_floor_terminal_legal(SafetyGoalProgressId id, int floor, bool endpoint_legal) const noexcept
{
    return endpoint_legal && mandatory_due_through_floor_satisfied(id, floor);
}

bool SafetyGoalProgram::is_final_terminal_legal(SafetyGoalProgressId id, bool endpoint_legal) const noexcept
{
    return endpoint_legal && all_mandatory_satisfied(id);
}

std::vector<int> SafetyGoalProgram::mandatory_progress_score(SafetyGoalProgressId id) const
{
    std::vector<int> score;
    if (!valid_id(id)) {
        return score;
    }
    const SafetyGoalProgressSnapshot& state = m_states[id];
    std::size_t begin = 0;
    while (begin < m_milestones.size()) {
        while (begin < m_milestones.size() && !m_milestones[begin].mandatory) {
            ++begin;
        }
        if (begin == m_milestones.size()) {
            break;
        }
        const int rank = m_milestones[begin].definition.rank;
        int completed = 0;
        int weighted_progress = 0;
        std::size_t end = begin;
        while (end < m_milestones.size() && m_milestones[end].mandatory && m_milestones[end].definition.rank == rank) {
            completed += state.satisfied[end] != 0 ? 1 : 0;
            weighted_progress += m_milestones[end].definition.weight *
                                 std::min(state.progress[end], m_milestones[end].definition.required_count);
            ++end;
        }
        score.emplace_back(completed);
        score.emplace_back(weighted_progress);
        begin = end;
    }
    return score;
}
} // namespace asst::blackflow
