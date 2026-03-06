using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class CopilotStateFallbackTests
{
    [Fact]
    public async Task StopWithoutCallback_ShouldRecoverUiAndRunOwner()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);

        var filePath = fixture.CreateCopilotFile();
        fixture.ViewModel.FilePath = filePath;
        await fixture.ViewModel.ImportFromFileAsync();
        await fixture.ViewModel.StartAsync();

        var taskId = fixture.Bridge.LastAppendedTaskId;
        fixture.ViewModel.ApplyRuntimeCallback(new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.Equal("Running", fixture.ViewModel.SelectedItem?.Status);
        Assert.Equal("Copilot", fixture.Runtime.SessionService.CurrentRunOwner);

        await fixture.ViewModel.StopAsync();

        Assert.Equal(SessionState.Connected, fixture.Runtime.SessionService.CurrentState);
        Assert.Equal("Stopped", fixture.ViewModel.SelectedItem?.Status);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(string root, MAAUnifiedRuntime runtime, TestBridge bridge, CopilotPageViewModel viewModel)
        {
            Root = root;
            Runtime = runtime;
            Bridge = bridge;
            ViewModel = viewModel;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public TestBridge Bridge { get; }

        public CopilotPageViewModel ViewModel { get; }

        public string CreateCopilotFile()
        {
            var path = Path.Combine(Root, "copilot.json");
            var payload = new JsonObject
            {
                ["stage_name"] = "1-7",
                ["minimum_required"] = "v4.0",
                ["actions"] = new JsonArray(new JsonObject()),
            };
            File.WriteAllText(path, payload.ToJsonString());
            return path;
        }

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

            var bridge = new TestBridge();
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
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            return new TestFixture(root, runtime, bridge, new CopilotPageViewModel(runtime));
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
                // ignore temp cleanup failures
            }
        }
    }

    private sealed class TestBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbacks = Channel.CreateUnbounded<CoreCallbackEvent>();
        private bool _connected;
        private bool _running;
        private int _nextTaskId;

        public int LastAppendedTaskId { get; private set; }

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
        {
            LastAppendedTaskId = Interlocked.Increment(ref _nextTaskId);
            return Task.FromResult(CoreResult<int>.Ok(LastAppendedTaskId));
        }

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
            await foreach (var callback in _callbacks.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _callbacks.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
