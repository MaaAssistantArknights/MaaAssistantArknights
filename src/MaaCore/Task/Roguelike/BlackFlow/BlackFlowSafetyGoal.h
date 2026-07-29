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

private:
    struct CompiledMilestone
    {
        Milestone definition;
        bool end = false;
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
    std::unordered_map<std::string, std::size_t> m_indices;
    std::vector<SafetyGoalProgressSnapshot> m_states;
    std::unordered_map<SafetyGoalProgressSnapshot, SafetyGoalProgressId, SnapshotHash> m_state_ids;
    SafetyGoalProgressId m_initial_progress_id = InvalidSafetyGoalProgressId;
};
} // namespace asst::blackflow
