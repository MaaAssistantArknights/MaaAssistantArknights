using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services.Features;

public sealed class ConnectFeatureService : IConnectFeatureService
{
    private readonly UnifiedSessionService _sessionService;

    public ConnectFeatureService(UnifiedSessionService sessionService)
    {
        _sessionService = sessionService;
    }

    public Task<CoreResult<bool>> ValidateAndConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.InvalidRequest, "Address cannot be empty.")));
        }

        return _sessionService.ConnectAsync(address, config, adbPath, cancellationToken);
    }
}

public sealed class TaskQueueFeatureService : ITaskQueueFeatureService
{
    private readonly UnifiedSessionService _sessionService;

    public TaskQueueFeatureService(UnifiedSessionService sessionService)
    {
        _sessionService = sessionService;
    }

    public Task<CoreResult<int>> QueueEnabledTasksAsync(CancellationToken cancellationToken = default)
    {
        return _sessionService.AppendTasksFromCurrentProfileAsync(cancellationToken);
    }
}

public sealed class CopilotFeatureService : ICopilotFeatureService
{
    public Task<string> ImportCopilotAsync(string source, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Copilot import queued from {source}");
    }
}

public sealed class ToolboxFeatureService : IToolboxFeatureService
{
    public Task<string> RunToolAsync(string toolName, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Toolbox action dispatched: {toolName}");
    }
}

public sealed class RemoteControlFeatureService : IRemoteControlFeatureService
{
    public Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<bool>.Ok(true));
    }
}

public sealed class OverlayFeatureService : IOverlayFeatureService
{
    private readonly IOverlayCapabilityService _overlay;

    public OverlayFeatureService(IOverlayCapabilityService overlay)
    {
        _overlay = overlay;
    }

    public Task<string> GetOverlayModeAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(_overlay.Capability.Supported ? "full" : "preview-and-log");
    }
}

public sealed class NotificationProviderFeatureService : INotificationProviderFeatureService
{
    public Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new[]
        {
            "Smtp",
            "ServerChan",
            "Bark",
            "Discord",
            "Telegram",
            "Qmsg",
            "Gotify",
            "CustomWebhook",
        });
    }
}

public sealed class DialogFeatureService : IDialogFeatureService
{
    public Task<string> PrepareDialogPayloadAsync(string dialogType, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Dialog payload prepared for {dialogType}");
    }
}
