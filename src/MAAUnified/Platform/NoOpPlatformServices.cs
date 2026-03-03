namespace MAAUnified.Platform;

public sealed class NoOpTrayService : ITrayService
{
    public event EventHandler<TrayCommandEvent>? CommandInvoked;

    public PlatformCapabilityStatus Capability => new(
        false,
        "System tray is not supported in this environment",
        Provider: "no-op",
        HasFallback: true,
        FallbackMode: "window-menu");

    public Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "tray.initialize", PlatformErrorCodes.TrayUnsupported));

    public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "tray.shutdown", PlatformErrorCodes.TrayUnsupported));

    public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "tray.show", PlatformErrorCodes.TrayUnsupported));

    public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "tray.setMenuState", PlatformErrorCodes.TrayUnsupported));

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "tray.setVisible", PlatformErrorCodes.TrayUnsupported));
}

public sealed class NoOpNotificationService : INotificationService
{
    public PlatformCapabilityStatus Capability => new(
        false,
        "System notification is unavailable, fallback to in-app notifications",
        Provider: "no-op",
        HasFallback: true,
        FallbackMode: "in-app");

    public Task<PlatformOperationResult> NotifyAsync(string title, string message, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "notification.notify", PlatformErrorCodes.NotificationUnsupported));
}

public sealed class NoOpGlobalHotkeyService : IGlobalHotkeyService
{
    public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

    public PlatformCapabilityStatus Capability => new(
        false,
        "Global hotkey is unavailable, fallback to window-scoped hotkeys",
        Provider: "no-op",
        HasFallback: true,
        FallbackMode: "window-scoped");

    public Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "hotkey.register", PlatformErrorCodes.HotkeyUnsupported));

    public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "hotkey.unregister", PlatformErrorCodes.HotkeyUnsupported));
}

public sealed class NoOpAutostartService : IAutostartService
{
    public PlatformCapabilityStatus Capability => new(
        false,
        "Autostart is unavailable on current platform runtime",
        Provider: "no-op",
        HasFallback: false,
        FallbackMode: null);

    public Task<PlatformOperationResult<bool>> IsEnabledAsync(CancellationToken cancellationToken = default)
        => Task.FromResult(new PlatformOperationResult<bool>(
            false,
            false,
            Capability.Message,
            PlatformErrorCodes.AutostartUnsupported,
            false,
            Capability.Provider,
            "autostart.query",
            PlatformExecutionMode.Failed));

    public Task<PlatformOperationResult> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.Failed(Capability.Provider, Capability.Message, PlatformErrorCodes.AutostartUnsupported, "autostart.set"));
}

public sealed class NoOpFileDialogService : IFileDialogService
{
    public PlatformCapabilityStatus Capability => new(true, "Basic file dialog is supported by Avalonia", Provider: "avalonia");
}

public sealed class NoOpOverlayCapabilityService : IOverlayCapabilityService
{
    public PlatformCapabilityStatus Capability => new(
        false,
        "Overlay attachment is unsupported, fallback to preview and logs only",
        Provider: "no-op",
        HasFallback: true,
        FallbackMode: "preview-and-log");

    public Task<PlatformOperationResult> BindHostWindowAsync(
        nint hostWindowHandle,
        bool clickThrough,
        double opacity,
        CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            Capability.Message,
            "overlay.bindHost",
            PlatformErrorCodes.OverlayUnsupported));

    public Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default)
    {
        IReadOnlyList<OverlayTarget> targets = new List<OverlayTarget>
        {
            new OverlayTarget("preview", "Preview + Logs", true),
        };
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            targets,
            Capability.Message,
            "overlay.query-targets",
            PlatformErrorCodes.OverlayUnsupported));
    }

    public Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "overlay.selectTarget", PlatformErrorCodes.OverlayUnsupported));

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
        => Task.FromResult(PlatformOperation.FallbackSuccess(Capability.Provider, Capability.Message, "overlay.setVisible", PlatformErrorCodes.OverlayUnsupported));
}

public sealed class NoOpPostActionExecutorService : IPostActionExecutorService
{
    private static readonly PlatformCapabilityStatus Unsupported = new(
        false,
        "Post action is unsupported in current environment and will fallback to logs.",
        Provider: "no-op",
        HasFallback: true,
        FallbackMode: "log-only");

    public PostActionCapabilityMatrix CapabilityMatrix { get; } = new(
        Unsupported,
        Unsupported,
        Unsupported,
        Unsupported,
        Unsupported,
        Unsupported,
        Unsupported);

    public Task<PlatformOperationResult> ExecuteAsync(
        PostActionType action,
        PostActionExecutorRequest? request = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            CapabilityMatrix.Get(action).Provider,
            CapabilityMatrix.Get(action).Message,
            operationId: $"post-action.{action}",
            errorCode: PlatformErrorCodes.PostActionUnsupported));
    }
}
