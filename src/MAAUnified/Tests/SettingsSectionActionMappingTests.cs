using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsSectionActionMappingTests
{
    [Fact]
    public async Task SelectedSection_Gui_ShouldExposeSaveGuiPrimaryAction()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedSection = vm.Sections.Single(section => section.Key == "GUI");

        var primary = Assert.Single(vm.CurrentSectionActions, action => action.IsPrimary);
        Assert.Equal("settings.save-gui", primary.ActionId);
        Assert.Contains("GUI", primary.Label, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task SelectedSection_ExternalNotification_ShouldExposeValidateTestAndSaveActions()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedSection = vm.Sections.Single(section => section.Key == "ExternalNotification");
        var actionIds = vm.CurrentSectionActions.Select(action => action.ActionId).ToArray();

        Assert.Contains("settings.save-notification", actionIds);
        Assert.Contains("settings.validate-notification", actionIds);
        Assert.Contains("settings.test-notification", actionIds);
    }

    [Fact]
    public async Task ExecuteSectionAction_SaveRemote_ShouldRouteToRemoteSaveFlow()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedSection = vm.Sections.Single(section => section.Key == "RemoteControl");
        var saveRemote = vm.CurrentSectionActions.Single(action => action.ActionId == "settings.save-remote");

        await vm.ExecuteSectionActionAsync(saveRemote);

        Assert.Contains("保存成功", vm.RemoteControlStatusMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RemoteControlFields_AutoSave_ShouldPersistWithoutManualSaveAction()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.RemoteGetTaskEndpoint = "https://example.com/get";
        vm.RemoteReportEndpoint = "https://example.com/report";
        vm.RemoteUserIdentity = "user-a";
        vm.RemoteDeviceIdentity = "device-a";
        vm.RemotePollInterval = 2300;

        var persisted = await WaitForConditionAsync(() =>
        {
            var globals = fixture.Runtime.ConfigurationService.CurrentConfig.GlobalValues;
            return ReadConfigValue(globals, ConfigurationKeys.RemoteControlGetTaskEndpointUri) == "https://example.com/get"
                   && ReadConfigValue(globals, ConfigurationKeys.RemoteControlReportStatusUri) == "https://example.com/report"
                   && ReadConfigValue(globals, ConfigurationKeys.RemoteControlUserIdentity) == "user-a"
                   && ReadConfigValue(globals, ConfigurationKeys.RemoteControlDeviceIdentity) == "device-a"
                   && ReadConfigValue(globals, ConfigurationKeys.RemoteControlPollIntervalMs) == "2300";
        });

        Assert.True(persisted);
        var statusUpdated = await WaitForConditionAsync(
            () => vm.RemoteControlStatusMessage.Contains("保存成功", StringComparison.Ordinal));
        Assert.True(statusUpdated);
        Assert.Contains("保存成功", vm.RemoteControlStatusMessage, StringComparison.Ordinal);
    }

    private static string ReadConfigValue(IReadOnlyDictionary<string, JsonNode?> values, string key)
    {
        return values.TryGetValue(key, out var node) && node is not null
            ? node.ToString()
            : string.Empty;
    }

    private static async Task<bool> WaitForConditionAsync(Func<bool> predicate, int retry = 80, int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (predicate())
            {
                return true;
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(string root, MAAUnifiedRuntime runtime, bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public static async Task<RuntimeFixture> CreateAsync(string? root = null, bool cleanupRoot = true)
        {
            root ??= Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));

            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            var bridge = new MaaCoreBridgeStub();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var platform = new PlatformServiceBundle
            {
                TrayService = new NoOpTrayService(),
                NotificationService = new NoOpNotificationService(),
                HotkeyService = new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = new NoOpOverlayCapabilityService(),
                PostActionExecutorService = new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);
            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = connect,
                ShellFeatureService = new ShellFeatureService(connect),
                TaskQueueFeatureService = new TaskQueueFeatureService(session, config),
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                ConfigurationProfileFeatureService = new ConfigurationProfileFeatureService(config),
                VersionUpdateFeatureService = new VersionUpdateFeatureService(config),
                AchievementFeatureService = new AchievementFeatureService(config),
                AnnouncementFeatureService = new AnnouncementFeatureService(config),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime, cleanupRoot);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            if (!_cleanupRoot)
            {
                return;
            }

            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

}
