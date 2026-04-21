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
        /// Indicates that the pending update package was successfully applied and the application is ready to restart.
        /// </summary>
        Succeeded,

        /// <summary>
        /// Indicates that the pending update package has been handed off to an external updater.
        /// The current process must exit so the updater can finish the replacement.
        /// </summary>
        Delegated,

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

    public bool Delegated => Status == StatusKind.Delegated;
}
