using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsModuleAK3FeatureTests
{
    [Fact]
    public async Task Timer_SaveAndReload_RoundTrip8Slots()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false))
            {
                first.Config.CurrentConfig.Profiles["Alpha"] = new UnifiedProfile();
                first.Config.CurrentConfig.Profiles["Beta"] = new UnifiedProfile();
                await first.Config.SaveAsync();

                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();

                vm.ForceScheduledStart = true;
                vm.ShowWindowBeforeForceScheduledStart = false;
                vm.CustomTimerConfig = true;
                for (var i = 0; i < vm.Timers.Count; i++)
                {
                    var slot = vm.Timers[i];
                    slot.Enabled = i % 2 == 0;
                    slot.Time = FormattableString.Invariant($"{i + 1:00}:{(i * 7) % 60:00}");
                    slot.Profile = i % 2 == 0 ? "Alpha" : "Beta";
                }

                await vm.SaveTimerSettingsAsync();

                Assert.False(vm.HasPendingTimerChanges);
                Assert.Equal("True", ReadGlobalString(first.Config, LegacyConfigurationKeys.ForceScheduledStart));
                Assert.Equal("False", ReadGlobalString(first.Config, LegacyConfigurationKeys.ShowWindowBeforeForceScheduledStart));
                Assert.Equal("True", ReadGlobalString(first.Config, LegacyConfigurationKeys.CustomConfig));

                for (var index = 1; index <= 8; index++)
                {
                    Assert.Equal((index % 2 == 1).ToString(), ReadGlobalString(first.Config, TimerEnabledKey(index)));
                    Assert.Equal((index).ToString(), ReadGlobalString(first.Config, TimerHourKey(index)));
                    Assert.Equal((((index - 1) * 7) % 60).ToString(), ReadGlobalString(first.Config, TimerMinuteKey(index)));
                    Assert.Equal(index % 2 == 1 ? "Alpha" : "Beta", ReadGlobalString(first.Config, TimerProfileKey(index)));
                }
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false);
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.True(reloaded.ForceScheduledStart);
            Assert.False(reloaded.ShowWindowBeforeForceScheduledStart);
            Assert.True(reloaded.CustomTimerConfig);
            for (var i = 0; i < reloaded.Timers.Count; i++)
            {
                var slot = reloaded.Timers[i];
                Assert.Equal(i % 2 == 0, slot.Enabled);
                Assert.Equal(FormattableString.Invariant($"{i + 1:00}:{(i * 7) % 60:00}"), slot.Time);
                Assert.Equal(i % 2 == 0 ? "Alpha" : "Beta", slot.Profile);
            }
        }
        finally
        {
            try
            {
                Directory.Delete(root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    [Fact]
    public async Task Timer_SaveInvalidTime_BlocksAndKeepsOldConfig()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.Timers[0].Enabled = true;
        vm.Timers[0].Time = "08:30";
        vm.Timers[0].Profile = "Default";
        await vm.SaveTimerSettingsAsync();

        var oldHour = ReadGlobalString(fixture.Config, TimerHourKey(1));
        var oldMinute = ReadGlobalString(fixture.Config, TimerMinuteKey(1));

        vm.Timers[0].Time = "8:30";
        await vm.SaveTimerSettingsAsync();

        Assert.Equal(oldHour, ReadGlobalString(fixture.Config, TimerHourKey(1)));
        Assert.Equal(oldMinute, ReadGlobalString(fixture.Config, TimerMinuteKey(1)));
        Assert.True(vm.HasPendingTimerChanges);
        Assert.Contains("HH:mm", vm.TimerValidationMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task Timer_SaveInvalidProfileWhenCustomConfig_Blocks()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.CustomTimerConfig = true;
        vm.Timers[0].Enabled = true;
        vm.Timers[0].Time = "09:45";
        vm.Timers[0].Profile = "Default";
        await vm.SaveTimerSettingsAsync();
        var oldProfile = ReadGlobalString(fixture.Config, TimerProfileKey(1));

        vm.Timers[0].Profile = "MissingProfile";
        await vm.SaveTimerSettingsAsync();

        Assert.Equal(oldProfile, ReadGlobalString(fixture.Config, TimerProfileKey(1)));
        Assert.True(vm.HasPendingTimerChanges);
        Assert.Contains("does not exist", vm.TimerValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task Timer_LoadLegacyKeys_NormalizesAndWarns()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.ForceScheduledStart] = JsonValue.Create(1);
        fixture.Config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.ShowWindowBeforeForceScheduledStart] = JsonValue.Create("0");
        fixture.Config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.CustomConfig] = JsonValue.Create("1");
        fixture.Config.CurrentConfig.GlobalValues[TimerEnabledKey(1)] = JsonValue.Create("true");
        fixture.Config.CurrentConfig.GlobalValues[TimerHourKey(1)] = JsonValue.Create("99");
        fixture.Config.CurrentConfig.GlobalValues[TimerMinuteKey(1)] = JsonValue.Create("-3");
        fixture.Config.CurrentConfig.GlobalValues[TimerProfileKey(1)] = JsonValue.Create("UnknownProfile");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True(vm.ForceScheduledStart);
        Assert.False(vm.ShowWindowBeforeForceScheduledStart);
        Assert.True(vm.CustomTimerConfig);
        Assert.True(vm.Timers[0].Enabled);
        Assert.Equal("23:00", vm.Timers[0].Time);
        Assert.Equal("Default", vm.Timers[0].Profile);
        Assert.Contains("clamped", vm.TimerValidationMessage, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("fell back", vm.TimerValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    private static string TimerEnabledKey(int index) => $"Timer.Timer{index}";

    private static string TimerHourKey(int index) => $"Timer.Timer{index}Hour";

    private static string TimerMinuteKey(int index) => $"Timer.Timer{index}Min";

    private static string TimerProfileKey(int index) => $"Timer.Timer{index}.Config";

    private static string ReadGlobalString(UnifiedConfigurationService config, string key)
    {
        if (!config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return string.Empty;
        }

        if (node is JsonValue value && value.TryGetValue(out string? text) && text is not null)
        {
            return text;
        }

        return node.ToString();
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            UnifiedConfigurationService config,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            Config = config;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public UnifiedConfigurationService Config { get; }

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

            var bridge = new FakeBridge();
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
            var shell = new ShellFeatureService(connect);
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
                ShellFeatureService = shell,
                TaskQueueFeatureService = new TaskQueueFeatureService(session, config),
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime, config, cleanupRoot);
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

    private sealed class FakeBridge : IMaaCoreBridge
    {
        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));
        }

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<int>.Ok(1));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));
        }

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));
        }

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));
        }

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([System.Runtime.CompilerServices.EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync()
        {
            return ValueTask.CompletedTask;
        }
    }
}
