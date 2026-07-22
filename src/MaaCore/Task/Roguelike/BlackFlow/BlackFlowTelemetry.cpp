#include "BlackFlowTelemetry.h"

namespace asst::blackflow
{
bool DiagnosticSettings::validate(std::string* error) const
{
    if (image_package_limit > 100) {
        if (error != nullptr) {
            *error = "blackflow_diagnostic_image_limit must be between 0 and 100";
        }
        return false;
    }
    return true;
}

std::optional<DiagnosticLevel> parse_diagnostic_level(std::string_view value) noexcept
{
    if (value == "normal") {
        return DiagnosticLevel::Normal;
    }
    if (value == "detailed") {
        return DiagnosticLevel::Detailed;
    }
    if (value == "full") {
        return DiagnosticLevel::Full;
    }
    return std::nullopt;
}

bool includes_full_routing_details(DiagnosticLevel level) noexcept
{
    return level == DiagnosticLevel::Full;
}

std::string_view to_string(DiagnosticLevel level) noexcept
{
    switch (level) {
    case DiagnosticLevel::Normal:
        return "normal";
    case DiagnosticLevel::Detailed:
        return "detailed";
    case DiagnosticLevel::Full:
        return "full";
    }
    return "normal";
}

std::string_view to_string(DiagnosticTrigger trigger) noexcept
{
    switch (trigger) {
    case DiagnosticTrigger::RoutineObservation:
        return "routine_observation";
    case DiagnosticTrigger::RebuildConflict:
        return "rebuild_conflict";
    case DiagnosticTrigger::InferredEdgeSelected:
        return "inferred_edge_selected";
    case DiagnosticTrigger::PreviewCostMismatch:
        return "preview_cost_mismatch";
    case DiagnosticTrigger::IdentityConflict:
        return "identity_conflict";
    case DiagnosticTrigger::PostMoveMismatch:
        return "post_move_mismatch";
    case DiagnosticTrigger::MapRebuildFailed:
        return "map_rebuild_failed";
    case DiagnosticTrigger::PageRecoveryFailed:
        return "page_recovery_failed";
    }
    return "rebuild_conflict";
}
} // namespace asst::blackflow
