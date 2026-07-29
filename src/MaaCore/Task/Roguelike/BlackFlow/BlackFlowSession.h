#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "BlackFlowNodeExecutionTypes.h"
#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"

#include "BlackFlowTaskPort.h"

namespace asst::blackflow
{
enum class FailureDisposition
{
    RestartRun,
    StopTask,
};

struct BlackFlowStrategyResult
{
    std::string profile;
    std::string outcome;
    std::string termination_reason;
    int cultivated_animals = 0;
    bool succeeded = false;
    std::string next_action = "stop_run";

    [[nodiscard]] json::object to_json() const;
};

enum class PageExecutionStage
{
    None,
    PendingDispatch,
    Running,
    Resolved,
};

struct VerifiedMoveArc
{
    std::string action_id;
    NodeId source = InvalidNodeId;
    NodeId target = InvalidNodeId;
    NodeId landing = InvalidNodeId;
    MovementKind movement = MovementKind::Walk;
    int exact_action_point_cost = 0;
    int required_action_points_after = UnreachableActionPointRequirement;
    std::optional<std::size_t> proof_depth;
    std::uint64_t map_revision = 0;
    std::uint64_t cost_revision = 0;
    std::uint64_t resources_revision = 0;
    std::uint64_t viewport_revision = 0;
};

struct PendingMoveCandidate
{
    MoveCandidate candidate;
    std::uint64_t run_revision = 0;
    std::uint64_t map_revision = 0;
    std::uint64_t cost_revision = 0;
    std::uint64_t resources_revision = 0;
    std::uint64_t viewport_revision = 0;
};

struct PageIdentityResolution
{
    NodeType type = NodeType::Unknown;
    std::string name;
};

[[nodiscard]] PageIdentityResolution resolve_page_identity(
    NodeType map_type,
    std::string map_name,
    const MovePreview* preview,
    const EnteredPageObservation& entered_page);

struct PageExecutionContext
{
    std::uint64_t run_revision = 0;
    std::uint64_t page_revision = 0;
    std::string decision_id;
    std::string transaction_id;
    int floor = 0;
    NodeId node = InvalidNodeId;
    NodeType node_type = NodeType::Unknown;
    std::string node_name;
    std::string page_intent = "default";
    std::vector<std::string> entry_markers;
    PageExecutionStage stage = PageExecutionStage::None;
    std::optional<NodeStateUpdate> result;
    bool resolution_reported = false;
};

class BlackFlowSession
{
public:
    bool initialize(std::string profile, std::string* error = nullptr);
    void reset_run();
    bool configure_diagnostics(DiagnosticSettings settings, std::string* error = nullptr);

    bool update(const BlackFlowPerceptionSnapshot& snapshot, std::string* error = nullptr);
    bool apply_movement_panel_observation(
        MovementPanelObservation panel,
        std::optional<MovementKind> active_movement,
        std::string* error = nullptr);
    bool apply_movement_inventory_observation(
        const std::unordered_set<MovementKind>& visible_movements,
        std::string* error = nullptr);
    bool report_movement_unavailable(MovementKind target, std::string* error = nullptr);
    [[nodiscard]] BlackFlowPlan plan(std::string* error = nullptr);
    bool save_pending_candidate(const MoveCandidate& candidate, std::string* error = nullptr);
    [[nodiscard]] bool validate_pending_candidate(std::string* error = nullptr) const;
    bool begin_pending_transaction(std::string* error = nullptr);

    void clear_pending_candidate() noexcept { m_pending_candidate.reset(); }

    bool begin_transaction(const MoveCandidate& candidate, std::string* error = nullptr);
    PreviewDisposition accept_preview(MovePreview preview, std::string* error = nullptr);
    [[nodiscard]] bool validate_commit(std::string* error = nullptr) const;
    bool commit(EnteredPageObservation entered_page = {}, std::string* error = nullptr);
    void cancel_transaction();
    bool set_current_floor(int floor, std::string* error = nullptr);

    void clear_current_floor() noexcept { m_current_floor.reset(); }

    [[nodiscard]] std::optional<int> current_floor() const noexcept { return m_current_floor; }

    [[nodiscard]] bool completed_page_changes_floor() const noexcept;

    [[nodiscard]] bool movement_inventory_refresh_required() const noexcept
    {
        return m_movement_inventory_refresh_required;
    }

    // 开局干员、分队、职业组整局不变，策略条件靠它们判断这一局能不能拿到结构性原理。
    // 必须在 initialize() 之前设置：事实是在 initialize() 末尾写入的，reset_run() 也会再写一次。
    void set_start_loadout(std::string core_char, std::string squad, std::string roles);

    bool mark_page_running(std::string* error = nullptr);
    bool apply_node_task_result(
        const NodeTaskResult& result,
        const json::value& callback_details,
        std::string* error = nullptr);

    void fail(std::string outcome, std::string reason, FailureDisposition disposition = FailureDisposition::StopTask);

    [[nodiscard]] const std::optional<BlackFlowStrategyResult>& result() const noexcept { return m_result; }

    [[nodiscard]] bool terminated() const noexcept { return m_result.has_value(); }

    bool claim_result_report() noexcept;

    [[nodiscard]] const ViewportObservation& viewport() const noexcept { return m_viewport; }

    [[nodiscard]] const RunState& run() const noexcept { return m_run; }

    [[nodiscard]] const NormalizedMap& map() const noexcept { return m_map; }

    [[nodiscard]] FactStore facts() const { return m_facts.merged(); }

    [[nodiscard]] const std::string& profile() const noexcept { return m_profile; }

    [[nodiscard]] const std::optional<PageExecutionContext>& page_context() const noexcept { return m_page_context; }

    [[nodiscard]] bool page_dispatch_required() const noexcept { return m_page_context.has_value(); }

    [[nodiscard]] const std::optional<PendingMoveCandidate>& pending_candidate() const noexcept
    {
        return m_pending_candidate;
    }

    [[nodiscard]] std::vector<BlackFlowTelemetryEvent> take_telemetry_events();
    [[nodiscard]] std::vector<DiagnosticArtifactRequest> take_diagnostic_requests();

    [[nodiscard]] MoveTransaction* transaction() noexcept
    {
        return m_transaction.has_value() ? &*m_transaction : nullptr;
    }

private:
    bool update_in_place(const BlackFlowPerceptionSnapshot& snapshot, std::string* error);
    bool synchronize_resource_facts(std::string* error);
    void refresh_mission();
    void evaluate_terminal_rules();
    bool apply_run_observation(const RunObservation& observation, std::string* error);
    bool merge_perception(
        const BlackFlowMapObservation& observation,
        const RunObservation& run,
        const FactStore& observed_facts,
        bool reconcile_move,
        std::string* error);
    bool reconcile_committed_move(const BlackFlowPerceptionSnapshot& snapshot, std::string* error);
    void finalize_entered_node(const PageExecutionContext& context, bool page_completed);
    void queue_map_summary(const PerceptionSummary& summary);
    void queue_warning(std::string code, std::string message, DiagnosticTrigger trigger);
    void queue_decision();
    void queue_node_resolution(const PageExecutionContext& context);
    void request_diagnostics(DiagnosticTrigger trigger, json::object snapshot = {});
    bool apply_observed_facts(const FactStore& facts, std::string* error);
    bool set_fact(std::string_view name, FactValue value, std::string* error);
    bool apply_node_signal(const NodeStrategySignal& signal, const json::value& callback_details, std::string* error);

    std::string m_profile;
    std::string m_start_core_char;
    std::string m_start_squad;
    std::string m_start_roles;
    std::optional<ResolvedPolicy> m_policy;
    FactContext m_facts;
    MissionState m_mission;
    BlackFlowObservationAdapter m_observation_adapter;
    NormalizedMap m_map;
    ViewportObservation m_viewport;
    RunState m_run;
    std::optional<int> m_current_floor;
    ResourceRegistry m_resources;
    std::unordered_set<std::string> m_unreachable_actions;
    std::optional<NodeId> m_pending_probe_target;
    std::optional<VerifiedMoveArc> m_verified_move_arc;
    std::optional<PendingMoveCandidate> m_pending_candidate;
    std::optional<MoveTransaction> m_transaction;
    std::optional<BlackFlowPlan> m_last_plan;
    std::optional<PageExecutionContext> m_page_context;
    std::optional<BlackFlowStrategyResult> m_result;
    bool m_result_reported = false;
    bool m_movement_inventory_refresh_required = true;
    DiagnosticSettings m_diagnostics;
    std::size_t m_persisted_image_packages = 0;
    std::uint64_t m_run_revision = 0;
    std::uint64_t m_page_revision = 0;
    std::uint64_t m_decision_sequence = 0;
    std::uint64_t m_transaction_sequence = 0;
    std::uint64_t m_artifact_sequence = 0;
    NodeId m_observed_current_node = InvalidNodeId;
    std::string m_observation_id;
    std::string m_decision_id;
    std::string m_transaction_id;
    std::vector<BlackFlowTelemetryEvent> m_telemetry_events;
    std::vector<DiagnosticArtifactRequest> m_diagnostic_requests;
};
} // namespace asst::blackflow
