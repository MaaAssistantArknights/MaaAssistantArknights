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
        NoPendingPackage,
        Succeeded,
        InvalidPackage,
        Failed,
    }

    public bool Succeeded => Status == StatusKind.Succeeded;
}
