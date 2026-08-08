#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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

// 安全求解的成功条件由两部分合取而成：一是物理上走到了没有后继的端点，二是本轮锁定的
// 强制目标都已经满足。这里负责后半部分——把锁定目标的完成进度编成一个可以进状态键的小整数。
//
// 只编入调用方给出的 binding_ids 及其前置。前置本身不进合取，它们只是让 prerequisites
// 判定有据可依。binding_ids 为空时程序退化成恒真，终局条件就只剩物理端点。
class SafetyGoalProgram
{
public:
    [[nodiscard]] static std::optional<SafetyGoalProgram> compile(
        const ResolvedPolicy& policy,
        const MissionState& mission,
        const FactStore& facts,
        const std::unordered_set<std::string>& binding_ids,
        std::string* error = nullptr);

    [[nodiscard]] static std::optional<SafetyGoalProgram> compile(
        const std::vector<Milestone>& milestones,
        const MissionState& mission,
        const FactStore& facts,
        const std::unordered_set<std::string>& binding_ids,
        std::string* error = nullptr);

    [[nodiscard]] SafetyGoalProgressId initial_progress_id() const noexcept { return m_initial_progress_id; }

    // 本轮锁定的强制目标是否已经全部满足。终局判定的合取项。
    [[nodiscard]] bool binding_goals_satisfied(SafetyGoalProgressId id) const noexcept;

    [[nodiscard]] bool has_binding_goals() const noexcept { return m_binding_count > 0; }

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
        bool binding = false;
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
    std::size_t m_binding_count = 0;
};
} // namespace asst::blackflow
