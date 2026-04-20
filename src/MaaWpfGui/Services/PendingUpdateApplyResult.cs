#pragma warning disable SA1633

#nullable enable

namespace MaaWpfGui.Services;

internal sealed record PendingUpdateApplyResult(
    PendingUpdateApplyResult.StatusKind Status,
    bool RequiresManualRecovery = false,
    string? FailureReason = null)
{
    internal enum StatusKind
    {
        /// <summary>
        /// Indicates that there is no pending update package to apply.
        /// </summary>
        NoPendingPackage,

        /// <summary>
        /// Indicates that the pending update package is not supported on the current system configuration.
        /// </summary>
        Succeeded,

        /// <summary>
        /// Indicates that the pending update package is invalid or corrupted.
        /// </summary>
        InvalidPackage,

        /// <summary>
        /// Indicates that the pending update package failed to apply due to an unexpected error.
        /// Requires manual recovery to restore the application to a stable state.
        Failed,
    }

    public bool Succeeded => Status == StatusKind.Succeeded;
}
