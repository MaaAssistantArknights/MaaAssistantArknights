using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services;

public sealed class MAAUnifiedRuntime : IAsyncDisposable
{
    public required IMaaCoreBridge CoreBridge { get; init; }

    public required UnifiedConfigurationService ConfigurationService { get; init; }

    public required ResourceWorkflowService ResourceWorkflowService { get; init; }

    public required UnifiedSessionService SessionService { get; init; }

    public required PlatformServiceBundle Platform { get; init; }

    public required UiLogService LogService { get; init; }

    public required IConnectFeatureService ConnectFeatureService { get; init; }

    public required ITaskQueueFeatureService TaskQueueFeatureService { get; init; }

    public required ICopilotFeatureService CopilotFeatureService { get; init; }

    public required IToolboxFeatureService ToolboxFeatureService { get; init; }

    public required IRemoteControlFeatureService RemoteControlFeatureService { get; init; }

    public required IOverlayFeatureService OverlayFeatureService { get; init; }

    public required INotificationProviderFeatureService NotificationProviderFeatureService { get; init; }

    public required IDialogFeatureService DialogFeatureService { get; init; }

    public ValueTask DisposeAsync()
    {
        return CoreBridge.DisposeAsync();
    }
}

public static class MAAUnifiedRuntimeFactory
{
    public static MAAUnifiedRuntime Create(string baseDirectory)
    {
        var logService = new UiLogService();
        var store = new AvaloniaJsonConfigStore(baseDirectory);
        var configService = new UnifiedConfigurationService(
            store,
            new GuiNewJsonConfigImporter(),
            new GuiJsonConfigImporter(),
            logService,
            baseDirectory);
        var bridge = new MaaCoreBridgeNative();
        var resourceWorkflowService = new ResourceWorkflowService(baseDirectory, bridge, logService);

        var stateMachine = new SessionStateMachine();
        var sessionService = new UnifiedSessionService(bridge, configService, logService, stateMachine);
        var platform = PlatformServicesFactory.CreateDefaults();

        var connectFeatureService = new ConnectFeatureService(sessionService);
        var taskQueueFeatureService = new TaskQueueFeatureService(sessionService);
        var copilotFeatureService = new CopilotFeatureService();
        var toolboxFeatureService = new ToolboxFeatureService();
        var remoteControlFeatureService = new RemoteControlFeatureService();
        var overlayFeatureService = new OverlayFeatureService(platform.OverlayService);
        var notificationProviderFeatureService = new NotificationProviderFeatureService();
        var dialogFeatureService = new DialogFeatureService();

        return new MAAUnifiedRuntime {
            CoreBridge = bridge,
            ConfigurationService = configService,
            ResourceWorkflowService = resourceWorkflowService,
            SessionService = sessionService,
            Platform = platform,
            LogService = logService,
            ConnectFeatureService = connectFeatureService,
            TaskQueueFeatureService = taskQueueFeatureService,
            CopilotFeatureService = copilotFeatureService,
            ToolboxFeatureService = toolboxFeatureService,
            RemoteControlFeatureService = remoteControlFeatureService,
            OverlayFeatureService = overlayFeatureService,
            NotificationProviderFeatureService = notificationProviderFeatureService,
            DialogFeatureService = dialogFeatureService,
        };
    }
}
