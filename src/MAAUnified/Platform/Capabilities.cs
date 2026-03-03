namespace MAAUnified.Platform;

public enum PlatformCapabilityId
{
    Tray = 0,
    Notification = 1,
    Hotkey = 2,
    Autostart = 3,
    Overlay = 4,
}

public enum PlatformExecutionMode
{
    Native = 0,
    Fallback = 1,
    Failed = 2,
}

public sealed record PlatformCapabilityStatus(
    bool Supported,
    string Message,
    string Provider = "unknown",
    bool HasFallback = false,
    string? FallbackMode = null);

public sealed record PlatformCapabilitySnapshot(
    PlatformCapabilityStatus Tray,
    PlatformCapabilityStatus Notification,
    PlatformCapabilityStatus Hotkey,
    PlatformCapabilityStatus Autostart,
    PlatformCapabilityStatus Overlay);

public static class PlatformCapabilitySnapshotFactory
{
    public static PlatformCapabilitySnapshot FromBundle(PlatformServiceBundle bundle)
    {
        return new PlatformCapabilitySnapshot(
            bundle.TrayService.Capability,
            bundle.NotificationService.Capability,
            bundle.HotkeyService.Capability,
            bundle.AutostartService.Capability,
            bundle.OverlayService.Capability);
    }
}
