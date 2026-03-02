namespace MAAUnified.Platform;

public sealed class PlatformServiceBundle
{
    public required ITrayService TrayService { get; init; }

    public required INotificationService NotificationService { get; init; }

    public required IGlobalHotkeyService HotkeyService { get; init; }

    public required IAutostartService AutostartService { get; init; }

    public required IFileDialogService FileDialogService { get; init; }

    public required IOverlayCapabilityService OverlayService { get; init; }
}

public static class PlatformServicesFactory
{
    public static PlatformServiceBundle CreateDefaults()
    {
        return new PlatformServiceBundle {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
        };
    }
}
