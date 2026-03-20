namespace MAAUnified.Platform;

public sealed record PlatformOperationResult(
    bool Success,
    string Message,
    string? ErrorCode = null,
    bool UsedFallback = false,
    string Provider = "unknown",
    string? OperationId = null,
    PlatformExecutionMode ExecutionMode = PlatformExecutionMode.Native);

public sealed record PlatformOperationResult<T>(
    bool Success,
    T? Value,
    string Message,
    string? ErrorCode = null,
    bool UsedFallback = false,
    string Provider = "unknown",
    string? OperationId = null,
    PlatformExecutionMode ExecutionMode = PlatformExecutionMode.Native);

public static class PlatformOperation
{
    public static PlatformOperationResult NativeSuccess(string provider, string message, string? operationId = null)
        => new(true, message, null, false, provider, operationId, PlatformExecutionMode.Native);

    public static PlatformOperationResult<T> NativeSuccess<T>(string provider, T value, string message, string? operationId = null)
        => new(true, value, message, null, false, provider, operationId, PlatformExecutionMode.Native);

    public static PlatformOperationResult FallbackSuccess(
        string provider,
        string message,
        string? operationId = null,
        string? errorCode = null)
        => new(true, message, errorCode, true, provider, operationId, PlatformExecutionMode.Fallback);

    public static PlatformOperationResult<T> FallbackSuccess<T>(
        string provider,
        T value,
        string message,
        string? operationId = null,
        string? errorCode = null)
        => new(true, value, message, errorCode, true, provider, operationId, PlatformExecutionMode.Fallback);

    public static PlatformOperationResult Failed(
        string provider,
        string message,
        string errorCode,
        string? operationId = null,
        bool usedFallback = false)
        => new(false, message, errorCode, usedFallback, provider, operationId, PlatformExecutionMode.Failed);

    public static PlatformOperationResult<T> Failed<T>(
        string provider,
        string message,
        string errorCode,
        string? operationId = null,
        bool usedFallback = false,
        T? value = default)
        => new(false, value, message, errorCode, usedFallback, provider, operationId, PlatformExecutionMode.Failed);
}

public sealed record TrayMenuState(
    bool StartEnabled,
    bool StopEnabled,
    bool OverlayEnabled,
    bool ForceShowEnabled,
    bool HideTrayEnabled);

public sealed record TrayMenuText(
    string Start,
    string Stop,
    string ForceShow,
    string HideTray,
    string ToggleOverlay,
    string SwitchLanguage,
    string Restart,
    string Exit)
{
    public static TrayMenuText Default { get; } = new(
        Start: "Start",
        Stop: "Stop",
        ForceShow: "Force Show",
        HideTray: "Hide Tray",
        ToggleOverlay: "Toggle Overlay",
        SwitchLanguage: "Switch Language",
        Restart: "Restart",
        Exit: "Exit");
}

public enum TrayCommandId
{
    Start = 0,
    Stop = 1,
    ForceShow = 2,
    HideTray = 3,
    ToggleOverlay = 4,
    Exit = 5,
    SwitchLanguage = 6,
    Restart = 7,
}

public sealed record TrayCommandEvent(
    TrayCommandId Command,
    string Source,
    DateTimeOffset Timestamp);

public sealed record GlobalHotkeyTriggeredEvent(
    string Name,
    string Gesture,
    DateTimeOffset Timestamp);

public enum OverlayRuntimeMode
{
    Hidden = 0,
    Preview = 1,
    Native = 2,
}

public sealed record OverlayStateChangedEvent(
    OverlayRuntimeMode Mode,
    bool Visible,
    string TargetId,
    string Action,
    string Message,
    DateTimeOffset Timestamp,
    string Provider = "unknown",
    bool UsedFallback = false,
    string? ErrorCode = null);

public sealed record OverlayTarget(
    string Id,
    string DisplayName,
    bool IsPrimary,
    long? NativeHandle = null,
    int? ProcessId = null,
    string? ProcessName = null,
    string? WindowTitle = null);

public interface ITrayService
{
    PlatformCapabilityStatus Capability { get; }

    event EventHandler<TrayCommandEvent>? CommandInvoked;

    Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> InitializeAsync(string appTitle, CancellationToken cancellationToken = default)
        => InitializeAsync(appTitle, null, cancellationToken);

    Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default);
}

public interface INotificationService
{
    PlatformCapabilityStatus Capability { get; }

    Task<PlatformOperationResult> NotifyAsync(string title, string message, CancellationToken cancellationToken = default);
}

public interface IGlobalHotkeyService
{
    PlatformCapabilityStatus Capability { get; }

    event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

    Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default);
}

public interface IAutostartService
{
    PlatformCapabilityStatus Capability { get; }

    Task<PlatformOperationResult<bool>> IsEnabledAsync(CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default);
}

public interface IFileDialogService
{
    PlatformCapabilityStatus Capability { get; }
}

public interface IOverlayCapabilityService
{
    PlatformCapabilityStatus Capability { get; }

    event EventHandler<OverlayStateChangedEvent>? OverlayStateChanged;

    Task<PlatformOperationResult> BindHostWindowAsync(nint hostWindowHandle, bool clickThrough, double opacity, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default);

    Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default);
}

public enum PostActionType
{
    ExitArknights = 0,
    BackToAndroidHome = 1,
    ExitEmulator = 2,
    ExitSelf = 3,
    Hibernate = 4,
    Shutdown = 5,
    Sleep = 6,
}

public sealed record PostActionExecutorRequest(
    string? CommandLine = null,
    string? ConnectAddress = null,
    string? ConnectConfig = null,
    string? AdbPath = null,
    string? ClientType = null,
    bool MuMu12ExtrasEnabled = false,
    string? MuMu12EmulatorPath = null,
    bool MuMuBridgeConnection = false,
    string? MuMu12Index = null,
    bool LdPlayerExtrasEnabled = false,
    string? LdPlayerEmulatorPath = null,
    bool LdPlayerManualSetIndex = false,
    string? LdPlayerIndex = null);

public sealed record PostActionCapabilityMatrix(
    PlatformCapabilityStatus ExitArknights,
    PlatformCapabilityStatus BackToAndroidHome,
    PlatformCapabilityStatus ExitEmulator,
    PlatformCapabilityStatus ExitSelf,
    PlatformCapabilityStatus Hibernate,
    PlatformCapabilityStatus Shutdown,
    PlatformCapabilityStatus Sleep)
{
    public PlatformCapabilityStatus Get(PostActionType action)
    {
        return action switch
        {
            PostActionType.ExitArknights => ExitArknights,
            PostActionType.BackToAndroidHome => BackToAndroidHome,
            PostActionType.ExitEmulator => ExitEmulator,
            PostActionType.ExitSelf => ExitSelf,
            PostActionType.Hibernate => Hibernate,
            PostActionType.Shutdown => Shutdown,
            PostActionType.Sleep => Sleep,
            _ => new PlatformCapabilityStatus(false, "Unknown action.", Provider: "unknown"),
        };
    }
}

public interface IPostActionExecutorService
{
    PostActionCapabilityMatrix CapabilityMatrix { get; }

    PostActionCapabilityMatrix GetCapabilityMatrix(PostActionExecutorRequest? request = null)
        => CapabilityMatrix;

    Task<PlatformOperationResult> ExecuteAsync(
        PostActionType action,
        PostActionExecutorRequest? request = null,
        CancellationToken cancellationToken = default);
}
