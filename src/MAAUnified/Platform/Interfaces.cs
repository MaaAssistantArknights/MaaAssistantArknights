namespace MAAUnified.Platform;

public interface ITrayService
{
    PlatformCapabilityStatus Capability { get; }
    Task ShowAsync(string title, string message, CancellationToken cancellationToken = default);
}

public interface INotificationService
{
    PlatformCapabilityStatus Capability { get; }
    Task NotifyAsync(string title, string message, CancellationToken cancellationToken = default);
}

public interface IGlobalHotkeyService
{
    PlatformCapabilityStatus Capability { get; }
}

public interface IAutostartService
{
    PlatformCapabilityStatus Capability { get; }
    Task<bool> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default);
}

public interface IFileDialogService
{
    PlatformCapabilityStatus Capability { get; }
}

public interface IOverlayCapabilityService
{
    PlatformCapabilityStatus Capability { get; }
}
