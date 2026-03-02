namespace MAAUnified.Platform;

public sealed class NoOpTrayService : ITrayService
{
    public PlatformCapabilityStatus Capability => new(false, "System tray is not supported in this environment");

    public Task ShowAsync(string title, string message, CancellationToken cancellationToken = default) => Task.CompletedTask;
}

public sealed class NoOpNotificationService : INotificationService
{
    public PlatformCapabilityStatus Capability => new(false, "System notification is unavailable, fallback to in-app notifications");

    public Task NotifyAsync(string title, string message, CancellationToken cancellationToken = default) => Task.CompletedTask;
}

public sealed class NoOpGlobalHotkeyService : IGlobalHotkeyService
{
    public PlatformCapabilityStatus Capability => new(false, "Global hotkey is unavailable, fallback to window-scoped hotkeys");
}

public sealed class NoOpAutostartService : IAutostartService
{
    public PlatformCapabilityStatus Capability => new(false, "Autostart is unavailable on current platform runtime");

    public Task<bool> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default) => Task.FromResult(false);
}

public sealed class NoOpFileDialogService : IFileDialogService
{
    public PlatformCapabilityStatus Capability => new(true, "Basic file dialog is supported by Avalonia");
}

public sealed class NoOpOverlayCapabilityService : IOverlayCapabilityService
{
    public PlatformCapabilityStatus Capability => new(false, "Overlay attachment is unsupported, fallback to preview and logs only");
}
