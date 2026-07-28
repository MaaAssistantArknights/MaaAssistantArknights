#include "BlackFlowSafetyGoal.h"

#include <algorithm>
#include <functional>
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
        // 已经错过或不可能的里程碑不再是目标，连同它的前置一起退出安全目标图。
        // MissionState::refresh 会沿 requires 链把它的后继一并标成 Impossible，
        // 所以这样跳过不会留下仍在追求、却依赖已死目标的里程碑。
        const MilestoneStatus status = mission.status(id);
        if (status == MilestoneStatus::Missed || status == MilestoneStatus::Impossible) {
            return true;
        }
        const auto definition = definitions.find(id);
        if (definition == definitions.end()) {
            if (status == MilestoneStatus::Satisfied) {
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
    for (const Milestone* milestone : selected) {
        const std::size_t index = program.m_milestones.size();
        program.m_indices.emplace(milestone->id, index);
        const bool mandatory = milestone->kind == MilestoneKind::Mandatory;
        program.m_milestones.emplace_back(CompiledMilestone { *milestone, mandatory, {} });
    }

    for (CompiledMilestone& milestone : program.m_milestones) {
        for (const std::string& prerequisite : milestone.definition.prerequisites) {
            const auto found = program.m_indices.find(prerequisite);
            if (found != program.m_indices.end()) {
                milestone.prerequisite_indices.emplace_back(found->second);
            }
            // 前置没有被编进来，只有两种合法情形：它已经完成，或者它已经失效而被跳过。
            else if (const MilestoneStatus status = mission.status(prerequisite);
                     status != MilestoneStatus::Satisfied && status != MilestoneStatus::Missed &&
                     status != MilestoneStatus::Impossible) {
                set_error(error, "mandatory safety goal cannot track prerequisite: " + prerequisite);
                return std::nullopt;
            }
        }
        std::ranges::sort(milestone.prerequisite_indices);
    }

    SafetyGoalProgressSnapshot initial;
    initial.progress.resize(program.m_milestones.size(), 0);
    initial.satisfied.resize(program.m_milestones.size(), 0);
    initial.counted_nodes.resize(program.m_milestones.size());
    for (std::size_t index = 0; index < program.m_milestones.size(); ++index) {
        const CompiledMilestone& milestone = program.m_milestones[index];
        const MilestoneStatus status = mission.status(milestone.definition.id);
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
        // 失效的里程碑也算作不再追求，安全求解不必再为它保留可达性。
        // 正常情况下它已被 include_with_prerequisites 跳过，这里只是兜住未刷新的 MissionState。
        initial.satisfied[index] = static_cast<std::uint8_t>(
            status == MilestoneStatus::Satisfied || status == MilestoneStatus::Missed ||
            status == MilestoneStatus::Impossible || completed_by_condition || completed_by_progress);
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

bool SafetyGoalProgram::is_floor_terminal_legal(SafetyGoalProgressId id, int floor, bool endpoint_legal) const noexcept
{
    return endpoint_legal && mandatory_due_through_floor_satisfied(id, floor);
}
} // namespace asst::blackflow
