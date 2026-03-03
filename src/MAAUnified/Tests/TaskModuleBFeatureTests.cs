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
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class TaskModuleBFeatureTests
{
    [Fact]
    public async Task AddTask_InfrastMallAward_InjectsExpectedDefaults()
    {
        await using var fixture = await TestFixture.CreateAsync("en-us");

        Assert.True((await fixture.TaskQueue.AddTaskAsync("InfrastTask", "infra")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("MallTask", "mall")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("AwardTask", "award")).Success);

        var queueResult = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(queueResult.Success, $"{queueResult.Error?.Code}:{queueResult.Error?.Message}");
        var tasks = Assert.IsAssignableFrom<IReadOnlyList<UnifiedTaskItem>>(queueResult.Value);
        Assert.Equal(3, tasks.Count);

        Assert.Equal(TaskModuleTypes.Infrast, tasks[0].Type);
        Assert.Equal(0, tasks[0].Params["mode"]?.GetValue<int>());
        var threshold = tasks[0].Params["threshold"]?.GetValue<double>() ?? 0;
        Assert.InRange(threshold, 0.299, 0.301);

        Assert.Equal(TaskModuleTypes.Mall, tasks[1].Type);
        var buyFirst = tasks[1].Params["buy_first"] as JsonArray;
        Assert.NotNull(buyFirst);
        Assert.Contains("Recruitment Permit", buyFirst!.Select(item => item?.GetValue<string>() ?? string.Empty));

        Assert.Equal(TaskModuleTypes.Award, tasks[2].Type);
        Assert.True(tasks[2].Params["award"]?.GetValue<bool>());
        Assert.False(tasks[2].Params["mail"]?.GetValue<bool>());
    }

    [Fact]
    public async Task UpdateTaskParams_RoundTripPersistsByTaskIndex()
    {
        await using var fixture = await TestFixture.CreateAsync();
        await fixture.TaskQueue.AddTaskAsync("Mall", "mall");

        var paramsResult = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(paramsResult.Success);
        var parameters = paramsResult.Value ?? new JsonObject();
        parameters["credit_fight"] = true;
        parameters["buy_first"] = new JsonArray("A", "B");

        var updateResult = await fixture.TaskQueue.UpdateTaskParamsAsync(0, parameters, persistImmediately: true);
        Assert.True(updateResult.Success);

        var latest = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(latest.Success);
        Assert.True(latest.Value?["credit_fight"]?.GetValue<bool>());
        var latestBuyFirst = latest.Value?["buy_first"] as JsonArray;
        Assert.NotNull(latestBuyFirst);
        Assert.Equal(2, latestBuyFirst!.Count);
    }

    [Fact]
    public async Task QueueEnabledTasks_BlocksExecutionAndDisablesMallCreditFight_WhenFightStageIsEmpty()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var profile = fixture.Config.CurrentConfig.Profiles["Default"];
        profile.TaskQueue.Add(new UnifiedTaskItem
        {
            Type = "Fight",
            Name = "fight",
            IsEnabled = true,
            Params = new JsonObject
            {
                ["stage"] = string.Empty,
                ["medicine"] = 0,
                ["stone"] = 0,
                ["times"] = 1,
                ["series"] = 1,
            },
        });
        profile.TaskQueue.Add(new UnifiedTaskItem
        {
            Type = "Mall",
            Name = "mall",
            IsEnabled = true,
            Params = new JsonObject
            {
                ["credit_fight"] = true,
            },
        });

        var queueResult = await fixture.TaskQueue.QueueEnabledTasksAsync();
        Assert.False(queueResult.Success);
        Assert.Contains("fight.stage", queueResult.Error?.Message ?? string.Empty, StringComparison.OrdinalIgnoreCase);
        Assert.False(profile.TaskQueue[1].Params["credit_fight"]?.GetValue<bool>());

        var eventLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-events.log"));
        Assert.Contains("Mall credit fight disabled", eventLog, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task Mall_ListNormalization_DedupTrimCaseInsensitive()
    {
        await using var fixture = await TestFixture.CreateAsync("en-us");
        await fixture.TaskQueue.AddTaskAsync("Mall", "mall");

        var vm = new MallModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });
        var taskParams = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(taskParams.Success);
        await vm.BindAsync(0, taskParams.Value!, CancellationToken.None);

        vm.BuyFirstText = "A; a ;B；B;; ";
        vm.BlacklistText = "X;x; y ;Y；";
        var flush = await vm.FlushPendingChangesAsync();
        Assert.True(flush);

        var latest = await fixture.TaskQueue.GetTaskParamsAsync(0);
        Assert.True(latest.Success);
        var buyFirst = latest.Value?["buy_first"] as JsonArray;
        var blacklist = latest.Value?["blacklist"] as JsonArray;
        Assert.NotNull(buyFirst);
        Assert.NotNull(blacklist);
        Assert.Equal(new[] { "A", "B" }, buyFirst!.Select(x => x?.GetValue<string>()).ToArray());
        Assert.Equal(new[] { "X", "y" }, blacklist!.Select(x => x?.GetValue<string>()).ToArray());
    }

    [Fact]
    public async Task PostAction_Load_MigratesLegacyBitmaskToProfileStructuredConfig()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PostActions] = 136; // ExitSelf + Sleep

        var load = await fixture.PostAction.LoadAsync();
        Assert.True(load.Success);
        Assert.NotNull(load.Value);
        Assert.True(load.Value!.ExitSelf);
        Assert.True(load.Value.Sleep);
        Assert.False(load.Value.Shutdown);

        var profile = fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile];
        Assert.True(profile.Values.ContainsKey("TaskQueue.PostAction"));
        Assert.False(profile.Values.ContainsKey(ConfigurationKeys.PostActions));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey("TaskQueue.PostAction"));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey(ConfigurationKeys.PostActions));
    }

    [Fact]
    public async Task PostAction_Save_WritesOnlyStructuredConfigKeyToCurrentProfile()
    {
        await using var fixture = await TestFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PostActions] = 64;

        var save = await fixture.PostAction.SaveAsync(new PostActionConfig
        {
            ExitSelf = true,
            Shutdown = true,
        });

        Assert.True(save.Success);
        var profile = fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile];
        Assert.True(profile.Values.ContainsKey("TaskQueue.PostAction"));
        Assert.False(profile.Values.ContainsKey(ConfigurationKeys.PostActions));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey("TaskQueue.PostAction"));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey(ConfigurationKeys.PostActions));
    }

    [Fact]
    public async Task PostAction_LoadSave_RoundTrip_WithCommands_ProfileScoped()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var save = await fixture.PostAction.SaveAsync(new PostActionConfig
        {
            ExitArknights = true,
            ExitSelf = true,
            Commands = new PostActionCommandConfig
            {
                ExitArknights = "adb shell am force-stop com.hypergryph.arknights",
                ExitSelf = "pkill MAAUnified",
            },
        });

        Assert.True(save.Success);

        var load = await fixture.PostAction.LoadAsync();
        Assert.True(load.Success);
        Assert.NotNull(load.Value);
        Assert.Equal("adb shell am force-stop com.hypergryph.arknights", load.Value!.Commands.ExitArknights);
        Assert.Equal("pkill MAAUnified", load.Value.Commands.ExitSelf);
        Assert.True(load.Value.ExitArknights);
        Assert.True(load.Value.ExitSelf);

        var profile = fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile];
        Assert.True(profile.Values.ContainsKey("TaskQueue.PostAction"));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey("TaskQueue.PostAction"));
    }

    [Fact]
    public async Task PostAction_ValidateSelection_CommandBackedActionsCapability()
    {
        var supported = new PlatformCapabilityStatus(true, "supported", Provider: "test");
        var executor = new TestPostActionExecutorService(new PostActionCapabilityMatrix(
            ExitArknights: supported,
            BackToAndroidHome: supported,
            ExitEmulator: supported,
            ExitSelf: supported,
            Hibernate: supported,
            Shutdown: supported,
            Sleep: supported));
        await using var fixture = await TestFixture.CreateAsync(executor: executor);

        var missingCommandPreview = await fixture.PostAction.ValidateSelectionAsync(new PostActionConfig
        {
            ExitSelf = true,
        });
        Assert.True(missingCommandPreview.Success);
        Assert.Contains(nameof(PostActionType.ExitSelf), missingCommandPreview.Value!.UnsupportedActions);

        var configuredPreview = await fixture.PostAction.ValidateSelectionAsync(new PostActionConfig
        {
            ExitSelf = true,
            Commands = new PostActionCommandConfig
            {
                ExitSelf = "echo close-maa",
            },
        });
        Assert.True(configuredPreview.Success);
        Assert.DoesNotContain(nameof(PostActionType.ExitSelf), configuredPreview.Value!.UnsupportedActions);
    }

    [Fact]
    public async Task PostAction_GetCapabilityPreview_ReportsUnsupportedActionsOnNoOpPlatform()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var preview = await fixture.PostAction.GetCapabilityPreviewAsync(new PostActionConfig
        {
            ExitSelf = true,
            Shutdown = true,
        });

        Assert.True(preview.Success);
        Assert.NotNull(preview.Value);
        Assert.Contains(nameof(PostActionType.ExitSelf), preview.Value!.UnsupportedActions);
        Assert.Contains(nameof(PostActionType.Shutdown), preview.Value.UnsupportedActions);
    }

    [Fact]
    public async Task PostAction_Execute_UsesExecutor_WhenCapabilitySupported()
    {
        var supported = new PlatformCapabilityStatus(true, "supported", Provider: "test");
        var executor = new TestPostActionExecutorService(new PostActionCapabilityMatrix(
            ExitArknights: supported,
            BackToAndroidHome: supported,
            ExitEmulator: supported,
            ExitSelf: supported,
            Hibernate: supported,
            Shutdown: supported,
            Sleep: supported));

        await using var fixture = await TestFixture.CreateAsync(executor: executor);

        var execute = await fixture.PostAction.ExecuteAfterCompletionAsync(
            new PostActionExecutionContext("AllTasksCompleted", true, RunId: "run-b1", TaskIndex: 2),
            new PostActionConfig
            {
                ExitSelf = true,
                Sleep = true,
                Commands = new PostActionCommandConfig
                {
                    ExitSelf = "echo close-maa",
                },
            });

        Assert.True(execute.Success, execute.Message);
        Assert.Contains(PostActionType.ExitSelf, executor.ExecutedActions);
        Assert.Contains(PostActionType.Sleep, executor.ExecutedActions);
        Assert.Contains(executor.Requests, r => r.Action == PostActionType.ExitSelf && r.Request?.CommandLine == "echo close-maa");

        var eventLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-events.log"));
        Assert.Contains("runId=run-b1", eventLog);
        Assert.Contains("action=ExitSelf", eventLog);
        Assert.Contains("action=Sleep", eventLog);
    }

    [Fact]
    public async Task PostAction_Execute_CommandFailure_RecordsError_NoCrash()
    {
        var supported = new PlatformCapabilityStatus(true, "supported", Provider: "test");
        var executor = new TestPostActionExecutorService(
            new PostActionCapabilityMatrix(
                ExitArknights: supported,
                BackToAndroidHome: supported,
                ExitEmulator: supported,
                ExitSelf: supported,
                Hibernate: supported,
                Shutdown: supported,
                Sleep: supported),
            action => action == PostActionType.ExitEmulator
                ? PlatformOperation.Failed("test", "forced failure", PlatformErrorCodes.PostActionExecutionFailed)
                : PlatformOperation.NativeSuccess("test", "ok"));

        await using var fixture = await TestFixture.CreateAsync(executor: executor);
        var execute = await fixture.PostAction.ExecuteAfterCompletionAsync(
            new PostActionExecutionContext("AllTasksCompleted", true, RunId: "run-b3", TaskIndex: 1),
            new PostActionConfig
            {
                ExitEmulator = true,
                Commands = new PostActionCommandConfig
                {
                    ExitEmulator = "echo close-emu",
                },
            });

        Assert.False(execute.Success);
        Assert.Contains(UiErrorCode.PostActionExecutionFailed, execute.Error?.Code ?? string.Empty);

        var errorLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-errors.log"));
        Assert.Contains("runId=run-b3", errorLog);
        Assert.Contains("action=ExitEmulator", errorLog);
    }

    [Fact]
    public async Task PostAction_Execute_UnsupportedSelection_LogsDegradeReason()
    {
        await using var fixture = await TestFixture.CreateAsync();

        var execute = await fixture.PostAction.ExecuteAfterCompletionAsync(
            new PostActionExecutionContext("AllTasksCompleted", true, RunId: "run-b2"),
            new PostActionConfig
            {
                Shutdown = true,
            });

        Assert.True(execute.Success);
        Assert.Contains("Skipped", execute.Message);

        var eventLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-events.log"));
        Assert.Contains("runId=run-b2", eventLog);
        Assert.Contains("action=Shutdown", eventLog);
        Assert.Contains($"errorCode={UiErrorCode.PostActionUnsupported}", eventLog);
    }

    [Fact]
    public async Task PostActionModule_OnceMode_DoesNotPersistActionEdits_ButPersistsCommandConfig()
    {
        var supported = new PlatformCapabilityStatus(true, "supported", Provider: "test");
        var executor = new TestPostActionExecutorService(new PostActionCapabilityMatrix(
            ExitArknights: supported,
            BackToAndroidHome: supported,
            ExitEmulator: supported,
            ExitSelf: supported,
            Hibernate: supported,
            Shutdown: supported,
            Sleep: supported));

        await using var fixture = await TestFixture.CreateAsync(executor: executor);
        await fixture.PostAction.SaveAsync(new PostActionConfig
        {
            Shutdown = true,
            ExitSelf = false,
            Commands = new PostActionCommandConfig
            {
                ExitSelf = "echo persist-old",
            },
        });

        var vm = new PostActionModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });
        await vm.InitializeAsync();
        Assert.True(vm.Shutdown);
        Assert.False(vm.ExitSelf);

        vm.Once = true;
        vm.Shutdown = false;
        vm.ExitSelf = true;
        vm.ExitSelfCommand = "echo persist-new";
        var flush = await vm.FlushPendingChangesAsync();
        Assert.True(flush);

        var persisted = await fixture.PostAction.LoadAsync();
        Assert.True(persisted.Success);
        Assert.True(persisted.Value!.Shutdown);
        Assert.False(persisted.Value.ExitSelf);
        Assert.Equal("echo persist-new", persisted.Value.Commands.ExitSelf);

        await vm.ReloadPersistentConfigAsync();
        Assert.False(vm.Once);
        Assert.True(vm.Shutdown);
        Assert.False(vm.ExitSelf);
        Assert.Equal("echo persist-new", vm.ExitSelfCommand);
    }

    [Fact]
    public async Task TaskQueuePage_CallbackRuntimeMapping_SubTaskStartCompleted()
    {
        await using var fixture = await TestFixture.CreateAsync();
        await fixture.TaskQueue.AddTaskAsync("Fight", "fight");

        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        await vm.ReloadTasksAsync();

        var callbackStart = new CoreCallbackEvent(
            20001,
            "SubTaskStart",
            """{"task_chain":"Fight","sub_task":"Stage","run_id":"run-callback","task_index":0}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callbackStart);

        Assert.NotNull(vm.LastRuntimeStatus);
        Assert.Equal("SubTaskStart", vm.LastRuntimeStatus!.Action);
        Assert.Equal("run-callback", vm.LastRuntimeStatus.RunId);
        Assert.Equal(0, vm.LastRuntimeStatus.TaskIndex);
        Assert.Equal("Fight", vm.LastRuntimeStatus.Module);
        Assert.Equal("Running", vm.LastRuntimeStatus.Status);

        var callbackCompleted = new CoreCallbackEvent(
            20002,
            "SubTaskCompleted",
            """{"task_chain":"Fight","sub_task":"Stage","run_id":"run-callback","task_index":0}""",
            DateTimeOffset.UtcNow);
        await InvokeCallbackAsync(vm, callbackCompleted);

        Assert.NotNull(vm.LastRuntimeStatus);
        Assert.Equal("SubTaskCompleted", vm.LastRuntimeStatus!.Action);
        Assert.Equal("Running", vm.LastRuntimeStatus.Status);
        Assert.Equal("Running", vm.Tasks[0].Status);

        var eventLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-events.log"));
        Assert.Contains("runId=run-callback", eventLog);
        Assert.Contains("taskIndex=0", eventLog);
        Assert.Contains("module=Fight", eventLog);
        Assert.Contains("action=SubTaskCompleted", eventLog);
        Assert.Contains("errorCode=-", eventLog);
    }

    [Fact]
    public async Task Infrast_Parse_SuccessAndRotation()
    {
        await using var fixture = await TestFixture.CreateAsync("en-us");
        var module = new InfrastModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });

        var customPlanPath = Path.Combine(fixture.Root, "config", "infrast.rotation.json");
        await File.WriteAllTextAsync(
            customPlanPath,
            """
            {
              "plans": [
                { "name": "Day", "period": ["00:00-12:00"] },
                { "name": "Night", "period": [] }
              ]
            }
            """);

        module.Mode = 10000;
        module.CustomFilePath = customPlanPath;
        await module.ReloadPlansAsync();

        Assert.Equal(3, module.PlanOptions.Count); // Auto + 2 plans
        Assert.Equal(-1, module.PlanOptions[0].Index);
        Assert.Contains("Loaded", module.StatusMessage, StringComparison.OrdinalIgnoreCase);
        Assert.Equal(string.Empty, module.LastErrorMessage);
    }

    [Fact]
    public async Task Infrast_Parse_FailureAndOutOfRange_LogErrorCode()
    {
        await using var fixture = await TestFixture.CreateAsync("en-us");
        var module = new InfrastModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });

        var invalidPath = Path.Combine(fixture.Root, "config", "infrast.invalid.json");
        await File.WriteAllTextAsync(invalidPath, """{"plans":[{"name":"A"}""");
        module.Mode = 10000;
        module.CustomFilePath = invalidPath;
        module.SelectedPlanIndex = 0;
        await module.ReloadPlansAsync();
        Assert.Contains("parse", module.LastErrorMessage, StringComparison.OrdinalIgnoreCase);

        var validPath = Path.Combine(fixture.Root, "config", "infrast.valid.json");
        await File.WriteAllTextAsync(
            validPath,
            "{\"plans\":[{\"name\":\"PlanA\",\"period\":[\"00:00-12:00\"]}]}");
        module.CustomFilePath = validPath;
        module.SelectedPlanIndex = 5;
        await module.ReloadPlansAsync();
        Assert.Contains("out of range", module.LastErrorMessage, StringComparison.OrdinalIgnoreCase);

        var errorLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-errors.log"));
        Assert.Contains(UiErrorCode.InfrastPlanParseFailed, errorLog);
        Assert.Contains(UiErrorCode.InfrastPlanOutOfRange, errorLog);
    }

    [Fact]
    public async Task Infrast_ReloadPlans_OutOfRange_ReportsErrorAndLogsCode()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var module = new InfrastModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });

        var customPlanPath = Path.Combine(fixture.Root, "config", "infrast.custom.json");
        await File.WriteAllTextAsync(
            customPlanPath,
            "{\"plans\":[{\"name\":\"PlanA\",\"period\":[\"00:00-12:00\"]}]}");

        module.Mode = 10000;
        module.CustomFilePath = customPlanPath;
        module.SelectedPlanIndex = 5;
        await module.ReloadPlansAsync();

        Assert.Contains("out of range", module.LastErrorMessage, StringComparison.OrdinalIgnoreCase);
        var errorLog = await File.ReadAllTextAsync(Path.Combine(fixture.Root, "debug", "avalonia-ui-errors.log"));
        Assert.Contains(UiErrorCode.InfrastPlanOutOfRange, errorLog);
    }

    [Fact]
    public async Task Award_RecruitConfirmation_ConfirmAndCancelBranches_WorkAsExpected()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var module = new AwardModuleViewModel(fixture.Runtime, new LocalizedTextMap { Language = "en-us" });

        module.Recruit = true;
        Assert.True(module.PendingRecruitConfirmation);
        Assert.False(module.Recruit);

        module.CancelRecruitEnable();
        Assert.False(module.PendingRecruitConfirmation);
        Assert.False(module.Recruit);

        module.Recruit = true;
        Assert.True(module.PendingRecruitConfirmation);

        module.ConfirmRecruitEnable();
        Assert.False(module.PendingRecruitConfirmation);
        Assert.True(module.Recruit);
    }

    private static async Task InvokeCallbackAsync(TaskQueuePageViewModel vm, CoreCallbackEvent callback)
    {
        var method = typeof(TaskQueuePageViewModel).GetMethod(
            "HandleCallbackCoreAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        var task = method!.Invoke(vm, new object?[] { callback }) as Task;
        if (task is null)
        {
            throw new InvalidOperationException("HandleCallbackAsync invocation returned null.");
        }

        await task;
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            UnifiedConfigurationService config,
            TaskQueueFeatureService taskQueue,
            PostActionFeatureService postAction,
            MAAUnifiedRuntime runtime,
            FakeBridge bridge)
        {
            Root = root;
            Config = config;
            TaskQueue = taskQueue;
            PostAction = postAction;
            Runtime = runtime;
            Bridge = bridge;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public PostActionFeatureService PostAction { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public FakeBridge Bridge { get; }

        public static async Task<TestFixture> CreateAsync(string language = "zh-cn", IPostActionExecutorService? executor = null)
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
                PostActionExecutorService = executor ?? new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var postAction = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService);
            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = new ConnectFeatureService(session, config),
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

            return new TestFixture(root, config, taskQueue, postAction, runtime, bridge);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            await Bridge.DisposeAsync();
            CleanupRoot();
        }

        private void CleanupRoot()
        {
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // keep temporary folder for inspection when file lock cleanup fails on CI hosts.
            }
        }
    }

    private sealed class TestPostActionExecutorService : IPostActionExecutorService
    {
        private readonly Func<PostActionType, PlatformOperationResult>? _handler;

        public TestPostActionExecutorService(
            PostActionCapabilityMatrix matrix,
            Func<PostActionType, PlatformOperationResult>? handler = null)
        {
            CapabilityMatrix = matrix;
            _handler = handler;
        }

        public PostActionCapabilityMatrix CapabilityMatrix { get; }

        public List<PostActionType> ExecutedActions { get; } = [];

        public List<(PostActionType Action, PostActionExecutorRequest? Request)> Requests { get; } = [];

        public Task<PlatformOperationResult> ExecuteAsync(
            PostActionType action,
            PostActionExecutorRequest? request = null,
            CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            ExecutedActions.Add(action);
            Requests.Add((action, request));
            if (_handler is not null)
            {
                return Task.FromResult(_handler(action));
            }

            return Task.FromResult(PlatformOperation.NativeSuccess(
                CapabilityMatrix.Get(action).Provider,
                $"Executed {action}.",
                $"post-action.{action}"));
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
