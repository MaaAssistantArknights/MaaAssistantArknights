#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "BlackFlowPolicy.h"

namespace asst::blackflow
{
using SafetyGoalProgressId = std::uint32_t;
inline constexpr SafetyGoalProgressId InvalidSafetyGoalProgressId = std::numeric_limits<SafetyGoalProgressId>::max();

struct SafetyGoalMilestoneInfo
{
    std::string id;
    MilestoneKind source_kind = MilestoneKind::Mandatory;
    MilestoneCompletion completion = MilestoneCompletion::VisitCount;
    int floor_begin = 0;
    int floor_end = 0;
    int rank = 0;
    int required_count = 1;
    int weight = 1;
    bool mandatory = false;
    bool prerequisite_only = false;
};

struct SafetyGoalPhase
{
    int deadline_floor = 0;
    int rank = 0;
    std::vector<std::size_t> milestone_indices;
};

struct SafetyGoalProgressSnapshot
{
    std::vector<int> progress;
    std::vector<std::uint8_t> satisfied;
    std::vector<std::vector<NodeId>> counted_nodes;

    bool operator==(const SafetyGoalProgressSnapshot&) const noexcept = default;
};

class SafetyGoalProgram
{
public:
    [[nodiscard]] static std::optional<SafetyGoalProgram> compile(
        const ResolvedPolicy& policy,
        const MissionState& mission,
        const FactStore& facts,
        std::string* error = nullptr);

    [[nodiscard]] static std::optional<SafetyGoalProgram> compile(
        const std::vector<Milestone>& milestones,
        const MissionState& mission,
        const FactStore& facts,
        std::string* error = nullptr);

    [[nodiscard]] SafetyGoalProgressId initial_progress_id() const noexcept { return m_initial_progress_id; }

    [[nodiscard]] const std::vector<SafetyGoalMilestoneInfo>& milestones() const noexcept { return m_milestone_info; }

    [[nodiscard]] const std::vector<SafetyGoalPhase>& phases() const noexcept { return m_phases; }

    [[nodiscard]] std::size_t progress_state_count() const noexcept { return m_states.size(); }

    [[nodiscard]] const SafetyGoalProgressSnapshot* progress(SafetyGoalProgressId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> milestone_index(std::string_view id) const noexcept;
    [[nodiscard]] int milestone_progress(SafetyGoalProgressId id, std::string_view milestone_id) const noexcept;
    [[nodiscard]] bool milestone_satisfied(SafetyGoalProgressId id, std::string_view milestone_id) const noexcept;

    [[nodiscard]] std::optional<SafetyGoalProgressId>
        refresh_conditions(SafetyGoalProgressId id, const FactStore& facts, std::string* error = nullptr);

    [[nodiscard]] std::optional<SafetyGoalProgressId> advance_node(
        SafetyGoalProgressId id,
        const Node& node,
        int unknown_nodes_revealed,
        const FactStore& facts,
        std::string* error = nullptr);

    [[nodiscard]] bool mandatory_due_through_floor_satisfied(SafetyGoalProgressId id, int floor) const noexcept;
    [[nodiscard]] bool all_mandatory_satisfied(SafetyGoalProgressId id) const noexcept;

    [[nodiscard]] bool is_floor_terminal_legal(SafetyGoalProgressId id, int floor, bool endpoint_legal) const noexcept;
    [[nodiscard]] bool is_final_terminal_legal(SafetyGoalProgressId id, bool endpoint_legal) const noexcept;

    [[nodiscard]] std::vector<int> mandatory_progress_score(SafetyGoalProgressId id) const;

private:
    struct CompiledMilestone
    {
        Milestone definition;
        bool mandatory = false;
        bool prerequisite_only = false;
        std::vector<std::size_t> prerequisite_indices;
    };

    struct SnapshotHash
    {
        std::size_t operator()(const SafetyGoalProgressSnapshot& value) const noexcept;
    };

    [[nodiscard]] bool valid_id(SafetyGoalProgressId id) const noexcept;
    [[nodiscard]] bool
        route_requirement_satisfied(const SafetyGoalProgressSnapshot& state, std::size_t index) const noexcept;
    [[nodiscard]] bool prerequisites_satisfied(
        const SafetyGoalProgressSnapshot& state,
        const CompiledMilestone& milestone) const noexcept;
    [[nodiscard]] SafetyGoalProgressId intern(SafetyGoalProgressSnapshot state);

    std::vector<CompiledMilestone> m_milestones;
    std::vector<SafetyGoalMilestoneInfo> m_milestone_info;
    std::vector<SafetyGoalPhase> m_phases;
    std::unordered_map<std::string, std::size_t> m_indices;
    std::vector<SafetyGoalProgressSnapshot> m_states;
    std::unordered_map<SafetyGoalProgressSnapshot, SafetyGoalProgressId, SnapshotHash> m_state_ids;
    SafetyGoalProgressId m_initial_progress_id = InvalidSafetyGoalProgressId;
};
} // namespace asst::blackflow
