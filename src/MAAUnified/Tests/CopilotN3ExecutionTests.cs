using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using Avalonia.Threading;
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

public sealed class CopilotN3ExecutionTests
{
    [Fact]
    public async Task CopilotFlow_StrictPump_ImportExecuteStopFeedback_ShouldCompleteByCallbackPump()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);

        var filePath = fixture.CreateCopilotFile();
        fixture.ViewModel.FilePath = filePath;
        await fixture.ViewModel.ImportFromFileAsync();
        Assert.Single(fixture.ViewModel.Items);
        Assert.Equal(filePath, fixture.ViewModel.SelectedItem?.SourcePath);
        Assert.Equal(string.Empty, fixture.ViewModel.LastErrorMessage);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Copilot.ImportFile"));

        await fixture.ViewModel.StartAsync();
        Assert.Equal(1, fixture.Bridge.StartCallCount);
        Assert.Single(fixture.Bridge.AppendedTasks);
        Assert.Equal("Queued", fixture.ViewModel.SelectedItem?.Status);
        Assert.Equal("Copilot", fixture.Runtime.SessionService.CurrentRunOwner);

        var taskId = fixture.Bridge.LastAppendedTaskId;
        fixture.Bridge.Publish(new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.True(await WaitForAsync(() => fixture.ViewModel.SelectedItem?.Status == "Running"));

        await fixture.ViewModel.StopAsync();
        Assert.Equal(1, fixture.Bridge.StopCallCount);
        Assert.Equal("Stopped", fixture.ViewModel.SelectedItem?.Status);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);

        fixture.Bridge.Publish(new CoreCallbackEvent(
            10004,
            "TaskChainStopped",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.True(await WaitForAsync(() => fixture.ViewModel.SelectedItem?.Status == "Stopped"));
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);

        await fixture.ViewModel.SendLikeAsync(true);
        Assert.Contains("点赞", fixture.ViewModel.StatusMessage, StringComparison.Ordinal);
        Assert.Equal(string.Empty, fixture.ViewModel.LastErrorMessage);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Copilot.Feedback"));
    }

    [Fact]
    public async Task CopilotFlow_InternalEntry_ImportExecuteStopFeedback_ShouldCompleteWithoutReflection()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);

        var filePath = fixture.CreateCopilotFile();
        fixture.ViewModel.FilePath = filePath;
        await fixture.ViewModel.ImportFromFileAsync();
        Assert.Equal(string.Empty, fixture.ViewModel.LastErrorMessage);

        await fixture.ViewModel.StartAsync();
        var taskId = fixture.Bridge.LastAppendedTaskId;

        fixture.ViewModel.ApplyRuntimeCallback(new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.Equal("Running", fixture.ViewModel.SelectedItem?.Status);

        await fixture.ViewModel.StopAsync();
        Assert.Equal("Stopped", fixture.ViewModel.SelectedItem?.Status);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);

        fixture.ViewModel.ApplyRuntimeCallback(new CoreCallbackEvent(
            3,
            "AllTasksCompleted",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.Equal("Stopped", fixture.ViewModel.SelectedItem?.Status);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);

        await fixture.ViewModel.SendLikeAsync(false);
        Assert.Contains("点踩", fixture.ViewModel.StatusMessage, StringComparison.Ordinal);
        Assert.Equal(string.Empty, fixture.ViewModel.LastErrorMessage);
    }

    [Fact]
    public async Task CopilotStart_WhenTaskQueueOwnsRun_ShouldBeBlocked()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True(fixture.Runtime.SessionService.TryBeginRun("TaskQueue", out _));

        var filePath = fixture.CreateCopilotFile();
        fixture.ViewModel.FilePath = filePath;
        await fixture.ViewModel.ImportFromFileAsync();
        await fixture.ViewModel.StartAsync();

        Assert.Contains("运行所有者", fixture.ViewModel.LastErrorMessage, StringComparison.Ordinal);
        Assert.Equal(0, fixture.Bridge.StartCallCount);
        Assert.Empty(fixture.Bridge.AppendedTasks);
    }

    [Fact]
    public async Task TaskQueueStart_WhenCopilotOwnsRun_ShouldBeBlockedAndLogged()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.TaskQueueFeatureService.AddTaskAsync("StartUp", "startup-a")).Success);

        var taskQueue = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await taskQueue.InitializeAsync();

        Assert.True(fixture.Runtime.SessionService.TryBeginRun("Copilot", out _));
        await taskQueue.StartAsync();

        Assert.Contains("active run owner", taskQueue.LastErrorMessage, StringComparison.Ordinal);
        Assert.Contains("Copilot", taskQueue.LastErrorMessage, StringComparison.Ordinal);
        Assert.Equal(0, fixture.Bridge.StartCallCount);

        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "[TaskQueue.Start.RunOwner]"));
    }

    [Fact]
    public async Task Copilot_StopWithoutCallback_ShouldRecoverUiAndRunOwner()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
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

    [Fact]
    public async Task Copilot_CallbackPayloadMalformed_ShouldWarnAndContinue()
    {
        await using var fixture = await CopilotN3Fixture.CreateAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);

        var filePath = fixture.CreateCopilotFile();
        fixture.ViewModel.FilePath = filePath;
        await fixture.ViewModel.ImportFromFileAsync();
        await fixture.ViewModel.StartAsync();

        var taskId = fixture.Bridge.LastAppendedTaskId;
        fixture.ViewModel.ApplyRuntimeCallback(new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            "{not-json}",
            DateTimeOffset.UtcNow));

        Assert.Equal("Running", fixture.ViewModel.SelectedItem?.Status);
        Assert.Contains(
            fixture.Runtime.LogService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.OrdinalIgnoreCase)
                && log.Message.Contains("Copilot callback payload parse failed", StringComparison.Ordinal));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Copilot.Callback.Parse"));

        fixture.ViewModel.ApplyRuntimeCallback(new CoreCallbackEvent(
            3,
            "AllTasksCompleted",
            $$"""{"task_chain":"Copilot","task_id":{{taskId}}}""",
            DateTimeOffset.UtcNow));
        Assert.Equal("Success", fixture.ViewModel.SelectedItem?.Status);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);
    }

    private static async Task<bool> WaitForAsync(Func<bool> condition, int retries = 40, int delayMs = 25)
    {
        for (var i = 0; i < retries; i++)
        {
            Dispatcher.UIThread.RunJobs(null);
            if (condition())
            {
                return true;
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private static async Task<bool> WaitForLogContainsAsync(string path, string expected, int retries = 40, int delayMs = 25)
    {
        for (var i = 0; i < retries; i++)
        {
            Dispatcher.UIThread.RunJobs(null);
            if (File.Exists(path))
            {
                var content = await File.ReadAllTextAsync(path);
                if (content.Contains(expected, StringComparison.Ordinal))
                {
                    return true;
                }
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private sealed class CopilotN3Fixture : IAsyncDisposable
    {
        private readonly CancellationTokenSource _pumpCts = new();
        private readonly Task _pumpTask;

        private CopilotN3Fixture(
            string root,
            MAAUnifiedRuntime runtime,
            Bridge bridge)
        {
            Root = root;
            Runtime = runtime;
            Bridge = bridge;
            ViewModel = new CopilotPageViewModel(runtime);
            _pumpTask = runtime.SessionService.StartCallbackPumpAsync(_ => Task.CompletedTask, _pumpCts.Token);
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public Bridge Bridge { get; }

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

        public static async Task<CopilotN3Fixture> CreateAsync()
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

            var bridge = new Bridge();
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

            return new CopilotN3Fixture(root, runtime, bridge);
        }

        public async ValueTask DisposeAsync()
        {
            _pumpCts.Cancel();
            try
            {
                await _pumpTask;
            }
            catch (OperationCanceledException)
            {
                // expected on disposal
            }

            _pumpCts.Dispose();
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

    private sealed class Bridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbacks = Channel.CreateUnbounded<CoreCallbackEvent>();
        private int _nextTaskId = 1;

        public int StartCallCount { get; private set; }

        public int StopCallCount { get; private set; }

        public int LastAppendedTaskId { get; private set; }

        public List<CoreTaskRequest> AppendedTasks { get; } = [];

        public void Publish(CoreCallbackEvent callback)
        {
            _callbacks.Writer.TryWrite(callback);
        }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(
                new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(
            CoreConnectionInfo connectionInfo,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(
            CoreTaskRequest task,
            CancellationToken cancellationToken = default)
        {
            AppendedTasks.Add(task);
            LastAppendedTaskId = _nextTaskId++;
            return Task.FromResult(CoreResult<int>.Ok(LastAppendedTaskId));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            StartCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

        public Task<CoreResult<bool>> AttachWindowAsync(
            CoreAttachWindowRequest request,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync(
            [EnumeratorCancellation] CancellationToken cancellationToken = default)
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
