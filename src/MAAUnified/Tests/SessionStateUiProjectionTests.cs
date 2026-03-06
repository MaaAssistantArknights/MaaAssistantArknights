using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SessionStateUiProjectionTests
{
    [Fact]
    public async Task TaskQueuePage_ShouldProjectSessionState_ToRunControls()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.Equal(SessionState.Idle, vm.CurrentSessionState);
        Assert.False(vm.IsRunning);
        Assert.False(vm.CanToggleRun);
        Assert.Equal("LinkStart", vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal("LinkStart", vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        Assert.True(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal("Stop", vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StopAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal("LinkStart", vm.RunButtonText);
    }

    [Fact]
    public async Task CopilotPage_ShouldProjectSessionState_ToRunControls()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new CopilotPageViewModel(fixture.Runtime);

        Assert.Equal(SessionState.Idle, vm.CurrentSessionState);
        Assert.False(vm.CanStart);
        Assert.False(vm.CanStop);
        Assert.False(vm.IsRunning);

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.True(vm.CanStart);
        Assert.False(vm.CanStop);
        Assert.False(vm.IsRunning);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        Assert.False(vm.CanStart);
        Assert.True(vm.CanStop);
        Assert.True(vm.IsRunning);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_ShouldDisableControlsAndRecoverWhenSessionStops()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        var waitTask = vm.WaitAndStopAsync();
        await WaitUntilAsync(() => vm.IsWaitingForStop);

        Assert.False(vm.CanToggleRun);
        Assert.False(vm.CanWaitAndStop);
        Assert.Equal("Waiting...", vm.WaitAndStopButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StopAsync()).Success);
        await waitTask;
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsWaitingForStop);
        Assert.True(vm.CanToggleRun);
        Assert.False(vm.CanWaitAndStop);
        Assert.Equal("WaitAndStop", vm.WaitAndStopButtonText);
        Assert.Equal(1, fixture.Bridge.StopCallCount);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_Canceled_ShouldRecoverPendingState()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        using var cts = new CancellationTokenSource();
        var waitTask = vm.WaitAndStopAsync(cts.Token);
        await WaitUntilAsync(() => vm.IsWaitingForStop);
        cts.Cancel();

        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await waitTask);

        Assert.False(vm.IsWaitingForStop);
        Assert.True(vm.CanToggleRun);
        Assert.True(vm.CanWaitAndStop);
        Assert.Equal("WaitAndStop", vm.WaitAndStopButtonText);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_ShouldDisableControlsDuringWait_AndRecoverAfterCancel()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        using var waitCts = new CancellationTokenSource();
        var waitTask = vm.WaitAndStopAsync(waitCts.Token);

        Assert.False(vm.CanWaitAndStop);
        Assert.False(vm.CanToggleRun);

        waitCts.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await waitTask);

        Assert.True(vm.CanWaitAndStop);
        Assert.True(vm.CanToggleRun);
    }

    private static async Task WaitUntilAsync(Func<bool> condition, int retry = 80, int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (condition())
            {
                return;
            }

            await Task.Delay(delayMs);
        }

        throw new TimeoutException("Condition not reached in expected time.");
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(string root, MAAUnifiedRuntime runtime, FakeBridge bridge)
        {
            Root = root;
            Runtime = runtime;
            Bridge = bridge;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public FakeBridge Bridge { get; }

        public static async Task<TestFixture> CreateAsync()
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
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
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new TestFixture(root, runtime, bridge);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
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
        private bool _connected;
        private bool _running;

        public int StopCallCount { get; private set; }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            _connected = !string.IsNullOrWhiteSpace(connectionInfo.Address);
            return Task.FromResult(_connected
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "connect failed")));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            if (!_connected)
            {
                return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "not connected")));
            }

            _running = true;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
            var wasRunning = _running;
            _running = false;
            return Task.FromResult(wasRunning
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "not running")));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, _connected, _running)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync() => ValueTask.CompletedTask;
    }
}
