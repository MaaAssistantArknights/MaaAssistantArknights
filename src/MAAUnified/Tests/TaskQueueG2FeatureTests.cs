using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class TaskQueueG2FeatureTests
{
    [Fact]
    public async Task StartAsync_ShouldFlushDirtyBoundModulesBeforeQueueAndStart()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.SelectedTask = Assert.Single(vm.Tasks);
        await vm.WaitForPendingBindingAsync();
        vm.StartUpModule.AccountName = "flush-before-start";

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();

        Assert.Equal(1, fixture.Bridge.StartCallCount);
        var appended = Assert.Single(fixture.Bridge.AppendedTasks);
        var appendedParams = Assert.IsType<JsonObject>(JsonNode.Parse(appended.ParamsJson));
        Assert.Equal("flush-before-start", appendedParams["account_name"]?.GetValue<string>());
    }

    [Fact]
    public async Task StopAsync_ShouldFlushDirtyBoundModulesBeforeStop()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.SelectedTask = Assert.Single(vm.Tasks);
        await vm.WaitForPendingBindingAsync();
        vm.StartUpModule.AccountName = "before-start";

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();
        vm.StartUpModule.AccountName = "before-stop";

        await vm.StopAsync();

        Assert.Equal(1, fixture.Bridge.StopCallCount);
        var paramsResult = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(paramsResult.Success);
        Assert.Equal("before-stop", paramsResult.Value?["account_name"]?.GetValue<string>());
    }

    [Fact]
    public async Task RunningState_ShouldBlockQueueMutationsAndEnabledToggle()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();
        Assert.True(vm.IsRunning);

        var beforeCount = vm.Tasks.Count;
        await vm.AddTaskAsync();
        Assert.Equal(beforeCount, vm.Tasks.Count);
        Assert.False(string.IsNullOrWhiteSpace(vm.LastErrorMessage));

        var originalEnabled = vm.Tasks[0].IsEnabled;
        vm.Tasks[0].IsEnabled = !originalEnabled;
        var reverted = await WaitForConditionAsync(() => vm.Tasks[0].IsEnabled == originalEnabled);
        Assert.True(reverted);
    }

    [Fact]
    public async Task Callback_TaskChainStart_WithTaskIndex_ShouldUpdateOnlyTargetTask()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        Assert.Equal(2, vm.Tasks.Count);

        var callback = new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            """{"task_chain":"Fight","task_index":1,"run_id":"run-g2"}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Idle, vm.Tasks[0].Status);
        Assert.Equal(TaskQueueItemStatus.Running, vm.Tasks[1].Status);
    }

    [Fact]
    public async Task Callback_TaskId_ShouldMapToCorrectQueueIndex()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();

        var callback = new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            """{"task_chain":"StartUp","task_id":2,"run_id":"run-g2-task-id"}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Idle, vm.Tasks[0].Status);
        Assert.Equal(TaskQueueItemStatus.Running, vm.Tasks[1].Status);
        Assert.Equal(1, vm.LastRuntimeStatus?.TaskIndex);

        var eventLog = await ReadEventLogAsync(fixture.Root);
        Assert.Contains("resolveSource=task_id_map", eventLog);
    }

    [Fact]
    public async Task Callback_TaskIndexAndTaskIdConflict_ShouldPreferTaskIndex()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();

        var callback = new CoreCallbackEvent(
            10001,
            "TaskChainStart",
            """{"task_chain":"StartUp","task_index":0,"task_id":2,"run_id":"run-g2-conflict"}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Running, vm.Tasks[0].Status);
        Assert.Equal(TaskQueueItemStatus.Idle, vm.Tasks[1].Status);
        Assert.Equal(0, vm.LastRuntimeStatus?.TaskIndex);

        var eventLog = await ReadEventLogAsync(fixture.Root);
        Assert.Contains("resolveSource=task_index", eventLog);
    }

    [Fact]
    public async Task Callback_NoIndexWithDuplicateChain_ShouldUseHeuristicAndLogWarning()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.Tasks[0].Status = TaskQueueItemStatus.Running;
        vm.Tasks[1].Status = TaskQueueItemStatus.Idle;

        var callback = new CoreCallbackEvent(
            20001,
            "SubTaskStart",
            """{"task_chain":"Fight","sub_task":"Stage","run_id":"run-g2-heuristic"}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Running, vm.Tasks[0].Status);
        Assert.Equal(TaskQueueItemStatus.Running, vm.Tasks[1].Status);
        Assert.Equal(1, vm.LastRuntimeStatus?.TaskIndex);

        var eventLog = await ReadEventLogAsync(fixture.Root);
        Assert.Contains("TaskQueue.Callback.ResolveTask", eventLog);
        Assert.Contains("resolveSource=chain_heuristic", eventLog);
    }

    [Fact]
    public async Task Callback_TaskChainError_ShouldSetErrorStatusAndSnapshot()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var callback = new CoreCallbackEvent(
            10000,
            "TaskChainError",
            """{"task_chain":"Fight","task_index":0,"run_id":"run-g2"}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Error, vm.Tasks[0].Status);
        Assert.NotNull(vm.LastRuntimeStatus);
        Assert.Equal("TaskChainError", vm.LastRuntimeStatus!.Action);
        Assert.Equal(TaskQueueItemStatus.Error, vm.LastRuntimeStatus.Status);
    }

    [Fact]
    public async Task Callback_AllTasksCompleted_ShouldExecutePostActionOncePerRunId()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.Tasks[0].Status = TaskQueueItemStatus.Running;

        var callback = new CoreCallbackEvent(
            3,
            "AllTasksCompleted",
            """{"task_chain":"Fight","task_index":0,"run_id":"run-g2"}""",
            DateTimeOffset.UtcNow);

        await InvokeCallbackAsync(vm, callback);
        await InvokeCallbackAsync(vm, callback);

        Assert.Equal(TaskQueueItemStatus.Success, vm.Tasks[0].Status);
        Assert.Equal(1, fixture.PostAction.ExecuteCount);
    }

    private static async Task InvokeCallbackAsync(TaskQueuePageViewModel vm, CoreCallbackEvent callback)
    {
        var method = typeof(TaskQueuePageViewModel).GetMethod(
            "HandleCallbackCoreAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        var task = method!.Invoke(vm, [callback]) as Task;
        if (task is null)
        {
            throw new InvalidOperationException("HandleCallbackCoreAsync invocation returned null.");
        }

        await task;
    }

    private static async Task<string> ReadEventLogAsync(string root)
    {
        var path = Path.Combine(root, "debug", "avalonia-ui-events.log");
        for (var i = 0; i < 20; i++)
        {
            if (File.Exists(path))
            {
                return await File.ReadAllTextAsync(path);
            }

            await Task.Delay(10);
        }

        return string.Empty;
    }

    private static async Task<bool> WaitForConditionAsync(Func<bool> predicate, int retry = 60, int delayMs = 20)
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

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            MAAUnifiedRuntime runtime,
            TaskQueueFeatureService taskQueue,
            CapturingBridge bridge,
            CountingPostActionFeatureService postAction)
        {
            Root = root;
            Runtime = runtime;
            TaskQueue = taskQueue;
            Bridge = bridge;
            PostAction = postAction;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public CapturingBridge Bridge { get; }

        public CountingPostActionFeatureService PostAction { get; }

        public static async Task<TestFixture> CreateAsync(string language = "zh-cn")
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
            config.CurrentConfig.GlobalValues["GUI.Localization"] = language;

            var bridge = new CapturingBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var taskQueue = new TaskQueueFeatureService(session, config);

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
            var postAction = new CountingPostActionFeatureService();

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
                TaskQueueFeatureService = taskQueue,
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = postAction,
            };

            return new TestFixture(root, runtime, taskQueue, bridge, postAction);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            await Bridge.DisposeAsync();
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // keep temporary folder for inspection when cleanup fails.
            }
        }
    }

    private sealed class CapturingBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _channel = Channel.CreateUnbounded<CoreCallbackEvent>();
        private int _taskId;
        private bool _connected;
        private bool _running;

        public List<CoreTaskRequest> AppendedTasks { get; } = [];

        public int StartCallCount { get; private set; }

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
        {
            AppendedTasks.Add(task);
            return Task.FromResult(CoreResult<int>.Ok(Interlocked.Increment(ref _taskId)));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            StartCallCount++;
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
            await foreach (var callback in _channel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _channel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }

    private sealed class CountingPostActionFeatureService : IPostActionFeatureService
    {
        public int ExecuteCount { get; private set; }

        public Task<UiOperationResult<PostActionConfig>> LoadAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult<PostActionConfig>.Ok(new PostActionConfig(), "Loaded."));

        public Task<UiOperationResult> SaveAsync(PostActionConfig config, CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult.Ok("Saved."));

        public Task<UiOperationResult<PostActionPreview>> GetCapabilityPreviewAsync(PostActionConfig config, CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult<PostActionPreview>.Ok(
                new PostActionPreview(false, [], []),
                "Previewed."));

        public Task<UiOperationResult<PostActionPreview>> ValidateSelectionAsync(PostActionConfig config, CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult<PostActionPreview>.Ok(
                new PostActionPreview(false, [], []),
                "Validated."));

        public Task<UiOperationResult> ExecuteAfterCompletionAsync(
            PostActionExecutionContext context,
            PostActionConfig? configOverride = null,
            CancellationToken cancellationToken = default)
        {
            ExecuteCount++;
            return Task.FromResult(UiOperationResult.Ok("Post action executed."));
        }
    }
}
