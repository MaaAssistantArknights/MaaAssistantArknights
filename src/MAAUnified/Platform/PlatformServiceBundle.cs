namespace MAAUnified.Platform;

public sealed class PlatformServiceBundle
{
    public required ITrayService TrayService { get; init; }

    public required INotificationService NotificationService { get; init; }

    public required IGlobalHotkeyService HotkeyService { get; init; }

    public required IAutostartService AutostartService { get; init; }

    public required IFileDialogService FileDialogService { get; init; }

    public required IOverlayCapabilityService OverlayService { get; init; }

    public required IPostActionExecutorService PostActionExecutorService { get; init; }
}

public static class PlatformServicesFactory
{
    public static PlatformServiceBundle CreateDefaults()
    {
        var forceFallback = string.Equals(
            Environment.GetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK"),
            "1",
            StringComparison.OrdinalIgnoreCase);

        ITrayService trayService;
        INotificationService notificationService;
        IGlobalHotkeyService hotkeyService;
        IAutostartService autostartService;
        IOverlayCapabilityService overlayService;
        IPostActionExecutorService postActionExecutorService;

        try
        {
            if (!forceFallback && OperatingSystem.IsWindows() && WindowsNotifyIconTrayService.TryCreate(out var nativeWinTray))
            {
                trayService = nativeWinTray;
            }
            else if (!forceFallback
                     && (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS())
                     && AvaloniaTrayIconTrayService.TryCreate(out var nativeAvaloniaTray))
            {
                trayService = nativeAvaloniaTray;
            }
            else
            {
                trayService = new WindowMenuTrayService();
            }
        }
        catch
        {
            trayService = new NoOpTrayService();
        }

        try
        {
            if (!forceFallback && DesktopNotificationService.TryCreate(out var nativeNotification))
            {
                notificationService = nativeNotification;
            }
            else
            {
                notificationService = new CommandNotificationService();
            }
        }
        catch
        {
            notificationService = new NoOpNotificationService();
        }

        try
        {
            if (!forceFallback && SharpHookGlobalHotkeyService.TryCreate(out var nativeHotkey))
            {
                hotkeyService = nativeHotkey;
            }
            else
            {
                hotkeyService = new WindowScopedHotkeyService();
            }
        }
        catch
        {
            hotkeyService = new NoOpGlobalHotkeyService();
        }

        try
        {
            autostartService = new CrossPlatformAutostartService();
        }
        catch
        {
            autostartService = new NoOpAutostartService();
        }

        try
        {
            overlayService = !forceFallback && OperatingSystem.IsWindows()
                ? new WindowsOverlayCapabilityService()
                : new NoOpOverlayCapabilityService();
        }
        catch
        {
            overlayService = new NoOpOverlayCapabilityService();
        }

        try
        {
            postActionExecutorService = new CommandPostActionExecutorService();
        }
        catch
        {
            postActionExecutorService = new NoOpPostActionExecutorService();
        }

        return new PlatformServiceBundle {
            TrayService = trayService,
            NotificationService = notificationService,
            HotkeyService = hotkeyService,
            AutostartService = autostartService,
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = overlayService,
            PostActionExecutorService = postActionExecutorService,
        };
    }
}
