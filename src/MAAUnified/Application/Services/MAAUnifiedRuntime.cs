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

    public required UiDiagnosticsService DiagnosticsService { get; init; }

    public required IConnectFeatureService ConnectFeatureService { get; init; }

    public required IShellFeatureService ShellFeatureService { get; init; }

    public required ITaskQueueFeatureService TaskQueueFeatureService { get; init; }

    public required ICopilotFeatureService CopilotFeatureService { get; init; }

    public required IToolboxFeatureService ToolboxFeatureService { get; init; }

    public required IRemoteControlFeatureService RemoteControlFeatureService { get; init; }

    public required IPlatformCapabilityService PlatformCapabilityService { get; init; }

    public required IOverlayFeatureService OverlayFeatureService { get; init; }

    public required INotificationProviderFeatureService NotificationProviderFeatureService { get; init; }

    public required ISettingsFeatureService SettingsFeatureService { get; init; }

    public IConfigurationProfileFeatureService ConfigurationProfileFeatureService { get; init; } = new ConfigurationProfileFeatureService();

    public IVersionUpdateFeatureService VersionUpdateFeatureService { get; init; } = new VersionUpdateFeatureService();

    public IAchievementFeatureService AchievementFeatureService { get; init; } = new AchievementFeatureService();

    public IAnnouncementFeatureService AnnouncementFeatureService { get; init; } = new AnnouncementFeatureService();

    public IStageManagerFeatureService StageManagerFeatureService { get; init; } = new StageManagerFeatureService();

    public IWebApiFeatureService WebApiFeatureService { get; init; } = new WebApiFeatureService();

    public required IDialogFeatureService DialogFeatureService { get; init; }

    public required IPostActionFeatureService PostActionFeatureService { get; init; }

    public IAppLifecycleService AppLifecycleService { get; init; } = new NoOpAppLifecycleService();

    public ValueTask DisposeAsync()
    {
        try
        {
            if (Platform.TrayService is IDisposable trayDisposable)
            {
                trayDisposable.Dispose();
            }

            if (Platform.HotkeyService is IDisposable hotkeyDisposable)
            {
                hotkeyDisposable.Dispose();
            }

            if (Platform.OverlayService is IDisposable overlayDisposable)
            {
                overlayDisposable.Dispose();
            }
        }
        catch
        {
            // Best-effort disposal.
        }

        return CoreBridge.DisposeAsync();
    }
}

public static class MAAUnifiedRuntimeFactory
{
    public static MAAUnifiedRuntime Create(string baseDirectory)
    {
        var logService = new UiLogService();
        var diagnosticsService = new UiDiagnosticsService(baseDirectory, logService);
        var store = new AvaloniaJsonConfigStore(baseDirectory);
        var configService = new UnifiedConfigurationService(
            store,
            new GuiNewJsonConfigImporter(),
            new GuiJsonConfigImporter(),
            logService,
            baseDirectory);
        var bridge = new MaaCoreBridgeNative();
        var stateMachine = new SessionStateMachine();
        var sessionService = new UnifiedSessionService(bridge, configService, logService, stateMachine);
        var platform = PlatformServicesFactory.CreateDefaults();
        var resourceWorkflowService = new ResourceWorkflowService(
            baseDirectory,
            bridge,
            logService,
            platform.GpuCapabilityService);

        var connectFeatureService = new ConnectFeatureService(sessionService, configService);
        var shellFeatureService = new ShellFeatureService(connectFeatureService);
        var taskQueueFeatureService = new TaskQueueFeatureService(sessionService, configService);
        var copilotFeatureService = new CopilotFeatureService();
        var toolboxFeatureService = new ToolboxFeatureService(bridge, connectFeatureService);
        var remoteControlFeatureService = new RemoteControlFeatureService();
        var platformCapabilityService = new PlatformCapabilityFeatureService(platform, diagnosticsService);
        var overlayFeatureService = new OverlayFeatureService(platformCapabilityService);
        var notificationProviderFeatureService = new NotificationProviderFeatureService();
        var settingsFeatureService = new SettingsFeatureService(configService, platformCapabilityService, diagnosticsService);
        var configurationProfileFeatureService = new ConfigurationProfileFeatureService(configService);
        var versionUpdateFeatureService = new VersionUpdateFeatureService(configService);
        var achievementFeatureService = new AchievementFeatureService(configService);
        var announcementFeatureService = new AnnouncementFeatureService(configService);
        var stageManagerFeatureService = new StageManagerFeatureService(configService);
        var webApiFeatureService = new WebApiFeatureService(configService);
        var dialogFeatureService = new DialogFeatureService(diagnosticsService);
        var postActionFeatureService = new PostActionFeatureService(
            configService,
            diagnosticsService,
            platform.PostActionExecutorService);
        var appLifecycleService = new ProcessAppLifecycleService();

        return new MAAUnifiedRuntime {
            CoreBridge = bridge,
            ConfigurationService = configService,
            ResourceWorkflowService = resourceWorkflowService,
            SessionService = sessionService,
            Platform = platform,
            LogService = logService,
            DiagnosticsService = diagnosticsService,
            ConnectFeatureService = connectFeatureService,
            ShellFeatureService = shellFeatureService,
            TaskQueueFeatureService = taskQueueFeatureService,
            CopilotFeatureService = copilotFeatureService,
            ToolboxFeatureService = toolboxFeatureService,
            RemoteControlFeatureService = remoteControlFeatureService,
            PlatformCapabilityService = platformCapabilityService,
            OverlayFeatureService = overlayFeatureService,
            NotificationProviderFeatureService = notificationProviderFeatureService,
            SettingsFeatureService = settingsFeatureService,
            ConfigurationProfileFeatureService = configurationProfileFeatureService,
            VersionUpdateFeatureService = versionUpdateFeatureService,
            AchievementFeatureService = achievementFeatureService,
            AnnouncementFeatureService = announcementFeatureService,
            StageManagerFeatureService = stageManagerFeatureService,
            WebApiFeatureService = webApiFeatureService,
            DialogFeatureService = dialogFeatureService,
            PostActionFeatureService = postActionFeatureService,
            AppLifecycleService = appLifecycleService,
        };
    }
}
