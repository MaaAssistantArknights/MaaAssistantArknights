#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <meojson/json.hpp>

#include "BlackFlowPolicy.h"

namespace asst::blackflow
{
enum class DiagnosticLevel
{
    Normal,
    Detailed,
    Full,
};

enum class DiagnosticTrigger
{
    RoutineObservation,
    RebuildConflict,
    InferredEdgeSelected,
    PreviewCostMismatch,
    IdentityConflict,
    PostMoveMismatch,
    MapRebuildFailed,
    PageRecoveryFailed,
};

struct DiagnosticSettings
{
    DiagnosticLevel level = DiagnosticLevel::Normal;
    std::size_t image_package_limit = 3;

    [[nodiscard]] bool validate(std::string* error = nullptr) const;
};

struct DiagnosticArtifactRequest
{
    DiagnosticTrigger trigger = DiagnosticTrigger::RebuildConflict;
    std::string artifact_set_id;
    std::string observation_id;
    std::string decision_id;
    std::string transaction_id;
    bool include_images = false;
    json::object snapshot;
};

struct BlackFlowTelemetryEvent
{
    std::string what;
    json::object details;
};

[[nodiscard]] std::optional<DiagnosticLevel> parse_diagnostic_level(std::string_view value) noexcept;
[[nodiscard]] bool includes_full_routing_details(DiagnosticLevel level) noexcept;
[[nodiscard]] std::string_view to_string(DiagnosticLevel level) noexcept;
[[nodiscard]] std::string_view to_string(DiagnosticTrigger trigger) noexcept;
} // namespace asst::blackflow
