using MAAUnified.CoreBridge;

namespace MAAUnified.Application.Services.Features;

public interface IConnectFeatureService
{
    Task<CoreResult<bool>> ValidateAndConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default);
}

public interface ITaskQueueFeatureService
{
    Task<CoreResult<int>> QueueEnabledTasksAsync(CancellationToken cancellationToken = default);
}

public interface ICopilotFeatureService
{
    Task<string> ImportCopilotAsync(string source, CancellationToken cancellationToken = default);
}

public interface IToolboxFeatureService
{
    Task<string> RunToolAsync(string toolName, CancellationToken cancellationToken = default);
}

public interface IRemoteControlFeatureService
{
    Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default);
}

public interface IOverlayFeatureService
{
    Task<string> GetOverlayModeAsync(CancellationToken cancellationToken = default);
}

public interface INotificationProviderFeatureService
{
    Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default);
}

public interface IDialogFeatureService
{
    Task<string> PrepareDialogPayloadAsync(string dialogType, CancellationToken cancellationToken = default);
}
