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
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class TaskQueueG1FeatureTests
{
    [Fact]
    public async Task TaskQueuePage_InitializeOnEmptyQueue_ShouldSeedWpfDefaultTasks()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var expected = new[]
        {
            TaskModuleTypes.StartUp,
            TaskModuleTypes.Fight,
            TaskModuleTypes.Infrast,
            TaskModuleTypes.Recruit,
            TaskModuleTypes.Mall,
            TaskModuleTypes.Award,
            TaskModuleTypes.Roguelike,
            TaskModuleTypes.Reclamation,
        };

        Assert.Equal(expected.Length, vm.Tasks.Count);
        Assert.Equal(expected, vm.Tasks.Select(task => task.Type).ToArray());
    }

    [Fact]
    public async Task SelectedTask_ShouldProjectTaskConfigVisibilityFlags()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync(TaskModuleTypes.StartUp, "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync(TaskModuleTypes.Fight, "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedTask = vm.Tasks[0];
        Assert.True(vm.IsStartUpTaskSelected);
        Assert.False(vm.IsFightTaskSelected);

        vm.SelectedTask = vm.Tasks[1];
        Assert.False(vm.IsStartUpTaskSelected);
        Assert.True(vm.IsFightTaskSelected);
        Assert.False(vm.IsNoTaskSelected);
    }

    [Fact]
    public async Task SelectedTask_SwitchingTasks_ShouldResetSettingsModeToGeneral()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync(TaskModuleTypes.Fight, "fight-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync(TaskModuleTypes.Recruit, "recruit-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();
        Assert.True(vm.CanUseAdvancedSettings);
        Assert.True(vm.ShowSettingsModeSwitch);
        Assert.True(vm.IsGeneralSettingsSelected);

        vm.SelectAdvancedSettingsMode();
        Assert.True(vm.IsAdvancedSettingsSelected);
        Assert.False(vm.IsGeneralSettingsSelected);
        Assert.True(vm.FightModule.IsAdvancedMode);
        Assert.False(vm.FightModule.IsGeneralMode);

        vm.SelectedTask = vm.Tasks[1];
        await vm.WaitForPendingBindingAsync();
        Assert.True(vm.IsGeneralSettingsSelected);
        Assert.False(vm.IsAdvancedSettingsSelected);
        Assert.False(vm.FightModule.IsAdvancedMode);
        Assert.True(vm.FightModule.IsGeneralMode);
    }

    [Fact]
    public async Task SelectedTask_SelectAdvancedSettingsMode_ShouldRaiseModeSelectionPropertyChanges()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync(TaskModuleTypes.Fight, "fight-a")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.SelectedTask = vm.Tasks[0];
        await vm.WaitForPendingBindingAsync();

        var changedProperties = new List<string>();
        vm.PropertyChanged += (_, e) =>
        {
            if (!string.IsNullOrWhiteSpace(e.PropertyName))
            {
                changedProperties.Add(e.PropertyName);
            }
        };

        vm.SelectAdvancedSettingsMode();

        Assert.Contains(nameof(TaskQueuePageViewModel.IsAdvancedSettingsSelected), changedProperties);
        Assert.Contains(nameof(TaskQueuePageViewModel.IsGeneralSettingsSelected), changedProperties);
    }

    [Fact]
    public async Task SelectedTask_ModuleType_ShouldExposeAdvancedSettingsAvailabilityMatrix()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var moduleTypes = new[]
        {
            TaskModuleTypes.StartUp,
            TaskModuleTypes.Fight,
            TaskModuleTypes.Recruit,
            TaskModuleTypes.Infrast,
            TaskModuleTypes.Mall,
            TaskModuleTypes.Award,
            TaskModuleTypes.Roguelike,
            TaskModuleTypes.Reclamation,
            TaskModuleTypes.Custom,
            TaskModuleTypes.PostAction,
        };

        for (var i = 0; i < moduleTypes.Length; i++)
        {
            Assert.True((await fixture.TaskQueue.AddTaskAsync(moduleTypes[i], $"task-{i}")).Success);
        }

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var expected = new Dictionary<string, (bool canAdvanced, bool showSwitch)>(StringComparer.OrdinalIgnoreCase)
        {
            [TaskModuleTypes.StartUp] = (false, false),
            [TaskModuleTypes.Fight] = (true, true),
            [TaskModuleTypes.Recruit] = (true, true),
            [TaskModuleTypes.Infrast] = (true, true),
            [TaskModuleTypes.Mall] = (true, true),
            [TaskModuleTypes.Award] = (false, false),
            [TaskModuleTypes.Roguelike] = (true, true),
            [TaskModuleTypes.Reclamation] = (true, true),
            [TaskModuleTypes.Custom] = (false, true),
            [TaskModuleTypes.PostAction] = (false, false),
        };

        foreach (var task in vm.Tasks)
        {
            vm.SelectedTask = task;
            await vm.WaitForPendingBindingAsync();

            var moduleType = TaskModuleTypes.Normalize(task.Type);
            Assert.True(expected.TryGetValue(moduleType, out var expectedState), $"Missing matrix expectation: {moduleType}");
            Assert.Equal(expectedState.canAdvanced, vm.CanUseAdvancedSettings);
            Assert.Equal(expectedState.showSwitch, vm.ShowSettingsModeSwitch);
            Assert.True(vm.IsGeneralSettingsSelected);
            Assert.False(vm.IsAdvancedSettingsSelected);

            vm.SelectAdvancedSettingsMode();
            Assert.Equal(expectedState.canAdvanced, vm.IsAdvancedSettingsSelected);
            Assert.Equal(!expectedState.canAdvanced, vm.IsGeneralSettingsSelected);
        }
    }

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
    public async Task TaskQueuePage_BatchAction_InverseMode_ShouldInvertWholeQueue()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.InverseClearMode] = JsonValue.Create("ClearInverse");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(true);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a", enabled: true)).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b", enabled: false)).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True(vm.ShowBatchModeToggle);
        Assert.Equal(SelectionBatchMode.Inverse, vm.SelectionBatchMode);
        Assert.Equal("反选", vm.BatchActionText);

        await vm.ExecuteBatchActionAsync();
        var queue = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(queue.Success);
        Assert.NotNull(queue.Value);
        Assert.False(queue.Value![0].IsEnabled);
        Assert.True(queue.Value[1].IsEnabled);
    }

    [Fact]
    public async Task TaskQueuePage_BatchAction_ClearMode_ShouldDisableWholeQueue()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.InverseClearMode] = JsonValue.Create("ClearInverse");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(false);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a", enabled: true)).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b", enabled: true)).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Equal(SelectionBatchMode.Clear, vm.SelectionBatchMode);
        Assert.Equal("清空", vm.BatchActionText);
        await vm.ExecuteBatchActionAsync();

        var queue = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(queue.Success);
        Assert.NotNull(queue.Value);
        Assert.All(queue.Value!, task => Assert.False(task.IsEnabled));
    }

    [Fact]
    public async Task TaskQueuePage_ToggleSelectionBatchMode_ShouldPersistLegacyKeys()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.InverseClearMode] = JsonValue.Create("ClearInverse");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(false);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        Assert.Equal(SelectionBatchMode.Clear, vm.SelectionBatchMode);

        await vm.ToggleSelectionBatchModeAsync();

        Assert.Equal(SelectionBatchMode.Inverse, vm.SelectionBatchMode);
        Assert.True(vm.ShowBatchModeToggle);
        Assert.Equal("反选", vm.BatchActionText);
        var profile = fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile];
        Assert.Equal("ClearInverse", profile.Values[ConfigurationKeys.InverseClearMode]?.GetValue<string>());
        Assert.True(profile.Values[ConfigurationKeys.MainFunctionInverseMode]?.GetValue<bool>());
    }

    [Fact]
    public async Task TaskQueuePage_InverseClearModeDisabled_ShouldFallbackToClearAndHideToggle()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.InverseClearMode] = JsonValue.Create("Clear");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(true);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.False(vm.ShowBatchModeToggle);
        Assert.Equal(SelectionBatchMode.Clear, vm.SelectionBatchMode);
        Assert.Equal("清空", vm.BatchActionText);
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
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup-a")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Fight", "fight-b")).Success);

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

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
