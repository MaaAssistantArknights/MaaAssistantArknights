using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class TaskQueueG1FeatureTests
{
    [Fact]
    public async Task TaskQueueFeatureService_SetAllAndInvertEnabled_ShouldUpdateWholeQueue()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a", enabled: true)).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b", enabled: false)).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Recruit", "recruit-c", enabled: true)).Success);

        var disableAll = await fixture.TaskQueue.SetAllTasksEnabledAsync(false);
        Assert.True(disableAll.Success);

        var allDisabled = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(allDisabled.Success);
        Assert.NotNull(allDisabled.Value);
        Assert.All(allDisabled.Value!, task => Assert.False(task.IsEnabled));

        var invert = await fixture.TaskQueue.InvertTasksEnabledAsync();
        Assert.True(invert.Success);

        var allEnabled = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(allEnabled.Success);
        Assert.NotNull(allEnabled.Value);
        Assert.All(allEnabled.Value!, task => Assert.True(task.IsEnabled));
    }

    [Fact]
    public async Task TaskQueuePage_StopAsync_ShouldMarkRunningTasksAsSkipped()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        Assert.Single(vm.Tasks);
        vm.Tasks[0].Status = TaskQueueItemStatus.Running;

        await vm.StopAsync();

        Assert.Equal(TaskQueueItemStatus.Skipped, vm.Tasks[0].Status);
    }

    [Fact]
    public async Task TaskQueuePage_SelectedTaskSwitch_ShouldFlushDirtyStartUpData()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();

        vm.StartUpModule.AccountName = "dirty-account";
        Assert.True(vm.StartUpModule.IsDirty);

        vm.SelectedTask = vm.Tasks[1];
        await vm.WaitForPendingBindingAsync();

        var saved = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(saved.Success);
        Assert.Equal("dirty-account", saved.Value?["account_name"]?.GetValue<string>());
    }

    [Fact]
    public async Task TaskQueuePage_MoveSelectedTask_ShouldFlushDirtyDataBeforeMove()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();

        vm.StartUpModule.AccountName = "dirty-before-move";
        Assert.True(vm.StartUpModule.IsDirty);

        await vm.MoveSelectedTaskAsync(1);
        await vm.WaitForPendingBindingAsync();

        var queue = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(queue.Success);
        Assert.NotNull(queue.Value);
        Assert.Equal("startup-a", queue.Value![1].Name);
        Assert.Equal("dirty-before-move", queue.Value[1].Params["account_name"]?.GetValue<string>());
    }

    [Fact]
    public async Task TaskQueuePage_TaskEnabledToggle_ShouldPersistToService()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        await vm.WaitForPendingBindingAsync();

        vm.Tasks[0].IsEnabled = false;

        var synced = await WaitForConditionAsync(async () =>
        {
            var queue = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
            return queue.Success && queue.Value is not null && !queue.Value[0].IsEnabled;
        });
        Assert.True(synced);
    }

    [Fact]
    public async Task QueueMutations_SaveAsync_ShouldPersistToDisk()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedTaskModule = "StartUp";
        vm.NewTaskName = "startup-a";
        await vm.AddTaskAsync();

        vm.SelectedTaskModule = "Fight";
        vm.NewTaskName = "fight-b";
        await vm.AddTaskAsync();

        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();
        vm.RenameTargetName = "startup-renamed";
        await vm.RenameSelectedTaskAsync();
        await vm.MoveSelectedTaskAsync(1);
        await vm.SelectAllAsync(false);
        await vm.InverseSelectionAsync();
        await vm.SaveAsync();

        var log = new UiLogService();
        var reloaded = new UnifiedConfigurationService(
            new AvaloniaJsonConfigStore(fixture.Root),
            new GuiNewJsonConfigImporter(),
            new GuiJsonConfigImporter(),
            log,
            fixture.Root);
        await reloaded.LoadOrBootstrapAsync();

        var profile = reloaded.CurrentConfig.Profiles[reloaded.CurrentConfig.CurrentProfile];
        Assert.Equal(2, profile.TaskQueue.Count);
        Assert.Equal("fight-b", profile.TaskQueue[0].Name);
        Assert.Equal("startup-renamed", profile.TaskQueue[1].Name);
        Assert.All(profile.TaskQueue, task => Assert.True(task.IsEnabled));
    }

    [Fact]
    public async Task SelectedTask_RapidSwitch_ShouldNotLoseDirtyData()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();
        vm.StartUpModule.AccountName = "rapid-a";

        vm.SelectedTask = vm.Tasks[1];
        vm.SelectedTask = vm.Tasks[0];
        vm.SelectedTask = vm.Tasks[1];
        await vm.WaitForPendingBindingAsync();
        vm.StartUpModule.AccountName = "rapid-b";

        vm.SelectedTask = vm.Tasks[0];
        vm.SelectedTask = vm.Tasks[1];
        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();

        var firstParams = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(firstParams.Success);
        Assert.Equal("rapid-a", firstParams.Value?["account_name"]?.GetValue<string>());

        var secondParams = await fixture.TaskQueue.GetTaskParamsAsync(1);
        Assert.True(secondParams.Success);
        Assert.Equal("rapid-b", secondParams.Value?["account_name"]?.GetValue<string>());
    }

    private static async Task<bool> WaitForConditionAsync(Func<Task<bool>> predicate, int retry = 50, int delayMs = 20)
    {
        for (var i = 0; i < retry; i++)
        {
            if (await predicate())
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
            UnifiedConfigurationService config,
            TaskQueueFeatureService taskQueue,
            MAAUnifiedRuntime runtime,
            FakeBridge bridge)
        {
            Root = root;
            Config = config;
            TaskQueue = taskQueue;
            Runtime = runtime;
            Bridge = bridge;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public FakeBridge Bridge { get; }

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

            var bridge = new FakeBridge();
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

            return new TestFixture(root, config, taskQueue, runtime, bridge);
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

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _channel = Channel.CreateUnbounded<CoreCallbackEvent>();
        private int _taskId;

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(Interlocked.Increment(ref _taskId)));

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
