#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "Config/Roguelike/BlackFlow/BlackFlowStrategyConfig.h"
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

#include "BlackFlowTaskPort.h"

namespace asst::blackflow
{
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

class BlackFlowSession;

class BlackFlowSession
{
public:
    bool initialize(std::string profile, std::string* error = nullptr);
    void reset_run();
    bool configure_diagnostics(DiagnosticSettings settings, std::string* error = nullptr);

    bool update(const BlackFlowPerceptionSnapshot& snapshot, std::string* error = nullptr);
    [[nodiscard]] BlackFlowPlan plan(std::string* error = nullptr);
    bool begin_transaction(const MoveCandidate& candidate, std::string* error = nullptr);
    PreviewDisposition accept_preview(MovePreview preview, std::string* error = nullptr);
    bool commit(std::string* error = nullptr);
    bool apply_post_move(const BlackFlowPostMoveSnapshot& snapshot, std::string* error = nullptr);
    bool bind_page_route(std::string* error = nullptr);
    bool apply_task_event(const json::value& callback_details, std::string* error = nullptr);
    void discard_applied_transaction() noexcept;

    void fail(std::string outcome, std::string reason);

    [[nodiscard]] const std::optional<BlackFlowStrategyResult>& result() const noexcept { return m_result; }

    [[nodiscard]] bool terminated() const noexcept { return m_result.has_value(); }

    [[nodiscard]] const ViewportObservation& viewport() const noexcept { return m_viewport; }

    [[nodiscard]] const RunState& run() const noexcept { return m_run; }

    [[nodiscard]] const NormalizedMap& map() const noexcept { return m_map; }

    [[nodiscard]] FactStore facts() const { return m_facts.merged(); }

    [[nodiscard]] const std::string& profile() const noexcept { return m_profile; }

    [[nodiscard]] std::vector<BlackFlowTelemetryEvent> take_telemetry_events();
    [[nodiscard]] std::vector<DiagnosticArtifactRequest> take_diagnostic_requests();

    [[nodiscard]] MoveTransaction* transaction() noexcept
    {
        return m_transaction.has_value() ? &*m_transaction : nullptr;
    }

private:
    bool synchronize_resource_facts(std::string* error);
    void refresh_mission();
    bool apply_run_observation(const RunObservation& observation, std::string* error);
    bool merge_perception(
        const BlackFlowMapObservation& observation,
        const RunObservation& run,
        const FactStore& observed_facts,
        bool post_move,
        std::string* error);
    void queue_map_summary(const PerceptionSummary& summary);
    void queue_warning(std::string code, std::string message, DiagnosticTrigger trigger);
    void queue_decision();
    bool queue_node_resolution(const json::value& details, std::string* error);
    void request_diagnostics(DiagnosticTrigger trigger, json::object snapshot = {});
    bool apply_observed_facts(const FactStore& facts, std::string* error);
    bool set_fact(std::string_view name, FactValue value, std::string* error);
    bool apply_event_effect(const TaskEventEffect& effect, const json::value& callback_details, std::string* error);

    std::string m_profile;
    std::optional<ResolvedPolicy> m_policy;
    FactContext m_facts;
    MissionState m_mission;
    BlackFlowObservationAdapter m_observation_adapter;
    NormalizedMap m_map;
    ViewportObservation m_viewport;
    RunState m_run;
    ResourceRegistry m_resources;
    std::unordered_set<std::string> m_unreachable_actions;
    std::optional<MoveTransaction> m_transaction;
    std::optional<BlackFlowPlan> m_last_plan;
    std::optional<BlackFlowStrategyResult> m_result;
    DiagnosticSettings m_diagnostics;
    std::size_t m_persisted_image_packages = 0;
    std::uint64_t m_run_revision = 0;
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

class BlackFlowTaskPlugin final : public AbstractRoguelikeTaskPlugin
{
public:
    BlackFlowTaskPlugin(
        const AsstCallback& callback,
        Assistant* inst,
        std::string_view task_chain,
        const std::shared_ptr<RoguelikeConfig>& config,
        const std::shared_ptr<RoguelikeControlTaskPlugin>& control,
        std::shared_ptr<BlackFlowSession> session,
        std::shared_ptr<IBlackFlowTaskPort> port);

    virtual bool load_params(const json::value& params) override;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual void reset_in_run_variables() override;

protected:
    virtual bool _run() override;

private:
    enum class PendingWork
    {
        None,
        Routing,
        TaskEvent,
        RecoverMap,
    };

    void report_result();
    void report_telemetry();
    void persist_diagnostics();

    std::shared_ptr<BlackFlowSession> m_session;
    std::shared_ptr<IBlackFlowTaskPort> m_port;
    mutable PendingWork m_pending = PendingWork::None;
    mutable json::value m_pending_details;
    bool m_reported = false;
};
} // namespace asst::blackflow
