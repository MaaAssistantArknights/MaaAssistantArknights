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

public sealed class TaskModuleCJ3ExecutionTests
{
    [Fact]
    public async Task QueueEnabledTasks_ModuleC_AppendParamsMatchValidateCompilation()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);

        var rawParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rawParams["mode"] = 20001;
        rawParams["theme"] = "UnknownTheme";
        rawParams["find_playTime_target"] = 99;
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rawParams)).Success);

        var validate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validate.Success, validate.Message);
        Assert.NotNull(validate.Value);
        Assert.Equal(TaskModuleTypes.Roguelike, validate.Value!.NormalizedType);
        Assert.True(validate.Value.Issues.Count > 0);

        var queue = await fixture.TaskQueue.QueueEnabledTasksAsync();
        Assert.True(queue.Success, queue.Error?.Message);
        Assert.Equal(1, queue.Value);

        var appended = Assert.Single(fixture.Bridge.AppendedTasks);
        var appendedParams = Assert.IsType<JsonObject>(JsonNode.Parse(appended.ParamsJson));
        Assert.Equal(validate.Value.NormalizedType, appended.Type);
        Assert.True(JsonNode.DeepEquals(appendedParams, validate.Value.CompiledParams));
    }

    [Fact]
    public async Task QueueEnabledTasks_ModuleC_BlockingIssue_BlocksAppend()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);

        var rawParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rawParams["mode"] = new JsonObject { ["bad"] = true };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rawParams)).Success);

        var queue = await fixture.TaskQueue.QueueEnabledTasksAsync();
        Assert.False(queue.Success);
        Assert.Contains("TaskFieldTypeInvalid", queue.Error?.Message ?? string.Empty);
        Assert.Empty(fixture.Bridge.AppendedTasks);
    }

    [Fact]
    public async Task TaskQueuePage_BindSelectedTask_UpdatesValidationSummary_ForModuleC()
    {
        await using var fixture = await TestFixture.CreateAsync(language: "en-us");
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.SelectedTask = Assert.Single(vm.Tasks);
        await InvokeBindSelectedTaskAsync(vm);

        Assert.True(vm.SelectedTaskValidationIssueCount > 0);
        Assert.False(vm.SelectedTaskHasBlockingValidationIssues);
        Assert.Contains("warning", vm.SelectedTaskValidationSummary, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task TaskQueuePage_StartAsync_BlockingValidationIssue_PreventsExecution()
    {
        await using var fixture = await TestFixture.CreateAsync(language: "en-us");
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);

        var rawParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rawParams["mode"] = new JsonObject { ["bad"] = true };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rawParams)).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await vm.StartAsync();

        Assert.False(vm.IsRunning);
        Assert.Contains("TaskFieldTypeInvalid", vm.LastErrorMessage, StringComparison.OrdinalIgnoreCase);
        Assert.Empty(fixture.Bridge.AppendedTasks);
    }

    private static async Task InvokeBindSelectedTaskAsync(TaskQueuePageViewModel vm)
    {
        var method = typeof(TaskQueuePageViewModel).GetMethod(
            "BindSelectedTaskAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        var task = method!.Invoke(vm, new object?[] { CancellationToken.None }) as Task;
        if (task is null)
        {
            throw new InvalidOperationException("BindSelectedTaskAsync invocation returned null.");
        }

        await task;
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            TaskQueueFeatureService taskQueue,
            MAAUnifiedRuntime runtime,
            CapturingBridge bridge)
        {
            Root = root;
            TaskQueue = taskQueue;
            Runtime = runtime;
            Bridge = bridge;
        }

        public string Root { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public CapturingBridge Bridge { get; }

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
            var connectFeatureService = new ConnectFeatureService(session, config);
            var postActionFeatureService = new PostActionFeatureService(
                config,
                diagnostics,
                platform.PostActionExecutorService);

            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = connectFeatureService,
                ShellFeatureService = new ShellFeatureService(connectFeatureService),
                TaskQueueFeatureService = taskQueue,
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = postActionFeatureService,
            };

            return new TestFixture(root, taskQueue, runtime, bridge);
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
        private readonly List<CoreTaskRequest> _tasks = [];
        private int _taskId;

        public IReadOnlyList<CoreTaskRequest> AppendedTasks => _tasks;

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            _tasks.Add(task);
            return Task.FromResult(CoreResult<int>.Ok(Interlocked.Increment(ref _taskId)));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

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
}
