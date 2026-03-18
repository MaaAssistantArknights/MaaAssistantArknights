using System.Text.Json.Nodes;
using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Advanced;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class OverlayParityTests
{
    [Fact]
    public void OverlayTargetPersistence_SerializeAndLoad_ShouldRoundTripSelection()
    {
        var target = new OverlayTarget(
            "hwnd:1A2B",
            "Game Window",
            false,
            NativeHandle: 0x1A2B,
            ProcessId: 2048,
            ProcessName: "emulator",
            WindowTitle: "Arknights");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create(OverlayTargetPersistence.Serialize(target)),
        };

        var persisted = OverlayTargetPersistence.Load(globals);

        Assert.NotNull(persisted);
        Assert.Equal(target.Id, persisted!.TargetId);
        Assert.Equal(target.NativeHandle, persisted.NativeHandle);
        Assert.Equal(target.ProcessId, persisted.ProcessId);
        Assert.Equal(target.ProcessName, persisted.ProcessName);
        Assert.Equal(target.WindowTitle, persisted.WindowTitle);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldRestoreByExactId()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var target = new OverlayTarget("hwnd:100", "Game", false, NativeHandle: 0x100, ProcessId: 1, ProcessName: "emu", WindowTitle: "Title");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"hwnd:100","NativeHandle":9999,"ProcessId":7,"ProcessName":"other","WindowTitle":"other"}"""),
        };

        var resolved = OverlayTargetPersistence.ResolveSelection([preview, target], globals);

        Assert.Equal(target, resolved);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldRestoreByLegacyHandleAndMetadata()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var byHandle = new OverlayTarget("hwnd:200", "Handle Match", false, NativeHandle: 0x200, ProcessId: 2, ProcessName: "emu-a", WindowTitle: "A");
        var byMetadata = new OverlayTarget("hwnd:300", "Metadata Match", false, NativeHandle: 0x300, ProcessId: 3, ProcessName: "emu-b", WindowTitle: "Arknights - MuMu");

        var globalsByHandle = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","Hwnd":512,"ProcessId":99,"ProcessName":"other","Title":"other"}"""),
        };
        var globalsByMetadata = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","Hwnd":9999,"ProcessId":3,"ProcessName":"emu-b","Title":"Arknights"}"""),
        };

        var resolvedByHandle = OverlayTargetPersistence.ResolveSelection([preview, byHandle, byMetadata], globalsByHandle);
        var resolvedByMetadata = OverlayTargetPersistence.ResolveSelection([preview, byHandle, byMetadata], globalsByMetadata);

        Assert.Equal(byHandle, resolvedByHandle);
        Assert.Equal(byMetadata, resolvedByMetadata);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldFallbackToPrimaryTarget()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var target = new OverlayTarget("hwnd:400", "Game", false, NativeHandle: 0x400, ProcessId: 4, ProcessName: "emu", WindowTitle: "Title");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","NativeHandle":9999,"ProcessId":77,"ProcessName":"other","WindowTitle":"other"}"""),
        };

        var resolved = OverlayTargetPersistence.ResolveSelection([preview, target], globals);

        Assert.Equal(preview, resolved);
    }

    [Fact]
    public async Task TaskQueuePage_AppendSystemLog_ShouldMirrorAndTrimOverlayLogs()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var vm = fixture.Shell.TaskQueuePage;

        for (var index = 0; index < 205; index++)
        {
            vm.AppendSystemLog($"entry-{index}");
        }

        Assert.Equal(200, vm.OverlayLogs.Count);
        Assert.Contains("entry-5", vm.OverlayLogs[0].Content, StringComparison.Ordinal);
        Assert.Contains("entry-204", vm.OverlayLogs[^1].Content, StringComparison.Ordinal);
        Assert.NotEmpty(vm.LogCards);
        Assert.Contains("entry-204", vm.LogCards[^1].Items[^1].Content, StringComparison.Ordinal);
    }

    [Fact]
    public async Task OverlaySharedState_ShouldSyncVisibilityBetweenTaskQueueAndAdvancedPage()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var taskQueue = fixture.Shell.TaskQueuePage;
        var advanced = new OverlayAdvancedPageViewModel(fixture.Runtime);

        Assert.False(taskQueue.OverlayVisible);
        Assert.False(advanced.Visible);

        await taskQueue.ToggleOverlayAsync();

        Assert.True(taskQueue.OverlayVisible);
        Assert.True(advanced.Visible);
    }

    [Fact]
    public async Task OverlayPresentation_ShouldPreferRunOwnerOverIdlePreference()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var presentation = fixture.Shell.OverlayPresentation;

        Assert.Equal(OverlayLogSource.TaskQueue, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.TaskQueuePage.OverlayLogs, presentation.CurrentLogs);

        presentation.PreferCopilot();
        Assert.Equal(OverlayLogSource.Copilot, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.CopilotPage.Logs, presentation.CurrentLogs);

        Assert.True(fixture.Runtime.SessionService.TryBeginRun("TaskQueue", out _));
        presentation.RefreshResolvedSource();

        Assert.Equal(OverlayLogSource.TaskQueue, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.TaskQueuePage.OverlayLogs, presentation.CurrentLogs);

        fixture.Runtime.SessionService.EndRun("TaskQueue");
        presentation.RefreshResolvedSource();

        Assert.Equal(OverlayLogSource.Copilot, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.CopilotPage.Logs, presentation.CurrentLogs);
    }

    [Fact]
    public async Task MainShellOverlayContextMethods_ShouldUpdateIdlePreference()
    {
        await using var fixture = await OverlayFixture.CreateAsync();

        await fixture.Shell.ToggleOverlayFromCopilotAsync();
        Assert.Equal(OverlayLogSource.Copilot, fixture.Shell.OverlayPresentation.PreferredSource);

        await fixture.Shell.ToggleOverlayFromTaskQueueAsync();
        Assert.Equal(OverlayLogSource.TaskQueue, fixture.Shell.OverlayPresentation.PreferredSource);
    }

    private sealed class OverlayFixture : IAsyncDisposable
    {
        private OverlayFixture(string root, MAAUnifiedRuntime runtime, MainShellViewModel shell)
        {
            Root = root;
            Runtime = runtime;
            Shell = shell;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public MainShellViewModel Shell { get; }

        public static async Task<OverlayFixture> CreateAsync()
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-overlay-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);

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
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
                AppLifecycleService = new NoOpAppLifecycleService(),
            };

            return new OverlayFixture(root, runtime, new MainShellViewModel(runtime, NoOpAppDialogService.Instance));
        }

        public async ValueTask DisposeAsync()
        {
            TestShellCleanup.StopTimerScheduler(Shell);
            await Runtime.DisposeAsync();

            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // Ignore cleanup failures for temp directories.
            }
        }
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbackChannel = Channel.CreateUnbounded<CoreCallbackEvent>();

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(
                new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));
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

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await foreach (var callback in _callbackChannel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _callbackChannel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
