using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;

namespace MAAUnified.Tests;

public sealed class TaskModuleCFeatureTests
{
    [Fact]
    public async Task AddTask_RoguelikeReclamationCustom_InjectsBaselineDefaults()
    {
        await using var fixture = await TestFixture.CreateAsync();

        Assert.True((await fixture.TaskQueue.AddTaskAsync("RoguelikeTask", "rogue")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("ReclamationTask", "reclamation")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("CustomTask", "custom")).Success);

        var queue = await fixture.TaskQueue.GetCurrentTaskQueueAsync();
        Assert.True(queue.Success);
        var tasks = Assert.IsAssignableFrom<IReadOnlyList<UnifiedTaskItem>>(queue.Value);
        Assert.Equal(3, tasks.Count);

        Assert.Equal(TaskModuleTypes.Roguelike, tasks[0].Type);
        Assert.Equal(0, tasks[0].Params["mode"]?.GetValue<int>());
        Assert.Equal("JieGarden", tasks[0].Params["theme"]?.GetValue<string>());
        Assert.Equal(int.MaxValue, tasks[0].Params["difficulty"]?.GetValue<int>());
        Assert.Equal(999999, tasks[0].Params["starts_count"]?.GetValue<int>());
        Assert.True(tasks[0].Params["investment_enabled"]?.GetValue<bool>());

        Assert.Equal(TaskModuleTypes.Reclamation, tasks[1].Type);
        Assert.Equal("Tales", tasks[1].Params["theme"]?.GetValue<string>());
        Assert.Equal(1, tasks[1].Params["mode"]?.GetValue<int>());
        Assert.Equal(0, tasks[1].Params["increment_mode"]?.GetValue<int>());
        Assert.Equal(16, tasks[1].Params["num_craft_batches"]?.GetValue<int>());
        Assert.False(tasks[1].Params["clear_store"]?.GetValue<bool>());

        Assert.Equal(TaskModuleTypes.Custom, tasks[2].Type);
        var taskNames = Assert.IsType<JsonArray>(tasks[2].Params["task_names"]);
        Assert.Empty(taskNames);
    }

    [Fact]
    public async Task SaveAndGet_RoundTripPreservesModuleCValues()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        var roguelikeSave = await fixture.TaskQueue.SaveRoguelikeParamsAsync(0, new RoguelikeTaskParamsDto
        {
            Mode = 4,
            Theme = "Mizuki",
            Difficulty = 12,
            StartsCount = 128,
            InvestmentEnabled = true,
            InvestmentWithMoreScore = true,
            InvestmentsCount = 77,
            StopWhenInvestmentFull = true,
            Squad = "Leader",
            Roles = "Pioneer",
            CoreChar = "Amiya",
            UseSupport = true,
            UseNonfriendSupport = true,
            RefreshTraderWithDice = true,
            CollectibleModeShopping = true,
            CollectibleModeSquad = "Collectors",
            StartWithEliteTwo = true,
            OnlyStartWithEliteTwo = false,
            CollectibleModeStartList = new RoguelikeCollectibleStartListDto
            {
                HotWater = true,
                Dice = true,
                Ticket = true,
            },
            StartFoldartalList = ["a", "b"],
            StartWithSeed = "seeda,rogue_1,3",
        });
        Assert.True(roguelikeSave.Success, roguelikeSave.Message);

        var reclamationSave = await fixture.TaskQueue.SaveReclamationParamsAsync(1, new ReclamationTaskParamsDto
        {
            Theme = "Fire",
            Mode = 1,
            IncrementMode = 1,
            NumCraftBatches = 9,
            ToolsToCraft = ["tool-a", "tool-b"],
            ClearStore = false,
        });
        Assert.True(reclamationSave.Success, reclamationSave.Message);

        var customSave = await fixture.TaskQueue.SaveCustomParamsAsync(2, new CustomTaskParamsDto
        {
            TaskNames = ["Fight", "Mall", "Award"],
        });
        Assert.True(customSave.Success, customSave.Message);

        var rogue = await fixture.TaskQueue.GetRoguelikeParamsAsync(0);
        Assert.True(rogue.Success);
        Assert.NotNull(rogue.Value);
        Assert.Equal(4, rogue.Value!.Mode);
        Assert.Equal("Mizuki", rogue.Value.Theme);
        Assert.True(rogue.Value.RefreshTraderWithDice);
        Assert.True(rogue.Value.CollectibleModeStartList.HotWater);
        Assert.True(rogue.Value.CollectibleModeStartList.Dice);
        Assert.True(rogue.Value.CollectibleModeStartList.Ticket);
        Assert.Equal("seeda,rogue_1,3", rogue.Value.StartWithSeed);

        var reclamation = await fixture.TaskQueue.GetReclamationParamsAsync(1);
        Assert.True(reclamation.Success);
        Assert.NotNull(reclamation.Value);
        Assert.Equal("Fire", reclamation.Value!.Theme);
        Assert.Equal(1, reclamation.Value.Mode);
        Assert.Equal(1, reclamation.Value.IncrementMode);
        Assert.Equal(9, reclamation.Value.NumCraftBatches);
        Assert.Equal(new[] { "tool-a", "tool-b" }, reclamation.Value.ToolsToCraft);
        Assert.False(reclamation.Value.ClearStore);

        var custom = await fixture.TaskQueue.GetCustomParamsAsync(2);
        Assert.True(custom.Success);
        Assert.NotNull(custom.Value);
        Assert.Equal(new[] { "Fight", "Mall", "Award" }, custom.Value!.TaskNames);
    }

    [Fact]
    public async Task ValidateTask_HandlesModuleC_AndReturnsIssueShape()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        var roguelikeParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        roguelikeParams.Remove("theme");
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, roguelikeParams)).Success);

        var validateRogue = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validateRogue.Success);
        Assert.NotNull(validateRogue.Value);
        Assert.Contains(validateRogue.Value!.Issues, issue => issue.Code == "TaskFieldMissing" && issue.Field == "roguelike.theme");

        var validateReclamation = await fixture.TaskQueue.ValidateTaskAsync(1);
        Assert.True(validateReclamation.Success);
        Assert.NotNull(validateReclamation.Value);
        Assert.Empty(validateReclamation.Value!.Issues);

        var validateCustom = await fixture.TaskQueue.ValidateTaskAsync(2);
        Assert.True(validateCustom.Success);
        Assert.NotNull(validateCustom.Value);
        Assert.Contains(validateCustom.Value!.Issues, issue => issue.Code == "CustomTaskNamesEmpty" && !issue.Blocking);
    }

    [Fact]
    public async Task ValidateTaskCompilation_ReturnsCompiledSnapshot_WithoutMutatingTaskParams()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);

        var rawParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rawParams["theme"] = "UnknownTheme";
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rawParams)).Success);

        var validate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validate.Success, validate.Message);
        Assert.NotNull(validate.Value);
        Assert.Equal(0, validate.Value!.TaskIndex);
        Assert.Equal("rogue", validate.Value.TaskName);
        Assert.Equal(TaskModuleTypes.Roguelike, validate.Value.NormalizedType);
        Assert.Equal("JieGarden", validate.Value.CompiledParams["theme"]?.GetValue<string>());
        Assert.Contains(validate.Value.Issues, issue => issue.Code == "RoguelikeThemeUnknown" && !issue.Blocking);

        var storedAfterValidate = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        Assert.Equal("UnknownTheme", storedAfterValidate["theme"]?.GetValue<string>());
    }

    [Fact]
    public async Task ValidateTaskCompilation_BlockingIssue_DoesNotRewriteOriginalTaskParams()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);

        var rawParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rawParams["mode"] = new JsonObject { ["bad"] = true };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rawParams)).Success);
        var beforeValidate = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!.ToJsonString();

        var validate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validate.Success, validate.Message);
        Assert.NotNull(validate.Value);
        Assert.True(validate.Value!.HasBlockingIssues);
        Assert.Contains(validate.Value.Issues, issue => issue.Code == "TaskFieldTypeInvalid" && issue.Blocking);
        Assert.Equal(1, validate.Value.CompiledParams["mode"]?.GetValue<int>());

        var afterValidate = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!.ToJsonString();
        Assert.Equal(beforeValidate, afterValidate);
    }

    [Fact]
    public async Task SaveAndReload_RoundTripKeepsModuleCParams()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        Assert.True((await fixture.TaskQueue.SaveRoguelikeParamsAsync(0, new RoguelikeTaskParamsDto
        {
            Mode = 7,
            Theme = "Sarkaz",
            Difficulty = 5,
            StartsCount = 32,
            InvestmentEnabled = false,
            DeepExplorationAutoIterate = false,
            StartWithSeed = "persist,rogue_2,9",
        })).Success);
        Assert.True((await fixture.TaskQueue.SaveReclamationParamsAsync(1, new ReclamationTaskParamsDto
        {
            Theme = "Tales",
            Mode = 1,
            IncrementMode = 1,
            NumCraftBatches = 4,
            ToolsToCraft = ["alpha", "beta"],
            ClearStore = false,
        })).Success);
        Assert.True((await fixture.TaskQueue.SaveCustomParamsAsync(2, new CustomTaskParamsDto
        {
            TaskNames = ["Roguelike", "Reclamation"],
        })).Success);
        Assert.True((await fixture.TaskQueue.SaveAsync()).Success);

        var log = new UiLogService();
        var config = new UnifiedConfigurationService(
            new AvaloniaJsonConfigStore(fixture.Root),
            new GuiNewJsonConfigImporter(),
            new GuiJsonConfigImporter(),
            log,
            fixture.Root);
        var load = await config.LoadOrBootstrapAsync();
        Assert.True(load.LoadedFromExistingConfig);

        var bridge = new FakeBridge();
        var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
        var queue = new TaskQueueFeatureService(session, config);

        var rogue = await queue.GetRoguelikeParamsAsync(0);
        Assert.True(rogue.Success);
        Assert.Equal(7, rogue.Value!.Mode);
        Assert.Equal("Sarkaz", rogue.Value.Theme);
        Assert.False(rogue.Value.InvestmentEnabled);
        Assert.False(rogue.Value.DeepExplorationAutoIterate);
        Assert.Equal("persist,rogue_2,9", rogue.Value.StartWithSeed);

        var reclamation = await queue.GetReclamationParamsAsync(1);
        Assert.True(reclamation.Success);
        Assert.Equal(1, reclamation.Value!.Mode);
        Assert.Equal(1, reclamation.Value.IncrementMode);
        Assert.Equal(4, reclamation.Value.NumCraftBatches);
        Assert.Equal(new[] { "alpha", "beta" }, reclamation.Value.ToolsToCraft);

        var custom = await queue.GetCustomParamsAsync(2);
        Assert.True(custom.Success);
        Assert.Equal(new[] { "Roguelike", "Reclamation" }, custom.Value!.TaskNames);

        await bridge.DisposeAsync();
    }

    [Fact]
    public async Task SaveRoguelike_InvalidSeed_BlocksAndPreservesOriginalParams()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);

        var before = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!.ToJsonString();
        var save = await fixture.TaskQueue.SaveRoguelikeParamsAsync(0, new RoguelikeTaskParamsDto
        {
            Mode = 0,
            Theme = "JieGarden",
            StartWithSeed = "invalid-seed-format",
        });

        Assert.False(save.Success);
        var after = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!.ToJsonString();
        Assert.Equal(before, after);
    }

    [Fact]
    public async Task SaveReclamation_DegradesRiskyFields_WithWarningsButNoBlocking()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);

        var save = await fixture.TaskQueue.SaveReclamationParamsAsync(0, new ReclamationTaskParamsDto
        {
            Theme = "UnknownTheme",
            Mode = 1,
            IncrementMode = 77,
            NumCraftBatches = 200000,
            ToolsToCraft = ["tool-a", "tool-b"],
            ClearStore = true,
        });
        Assert.True(save.Success, save.Message);

        var parameters = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        Assert.Equal("Tales", parameters["theme"]?.GetValue<string>());
        Assert.Equal(1, parameters["mode"]?.GetValue<int>());
        Assert.Equal(0, parameters["increment_mode"]?.GetValue<int>());
        Assert.Equal(99999, parameters["num_craft_batches"]?.GetValue<int>());
        Assert.False(parameters["clear_store"]?.GetValue<bool>());

        parameters["clear_store"] = JsonValue.Create(true);
        parameters["mode"] = JsonValue.Create(1);
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, parameters)).Success);

        var validate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validate.Success, validate.Message);
        Assert.NotNull(validate.Value);
        Assert.Contains(
            validate.Value!.Issues,
            issue => issue.Code == "ReclamationClearStoreIgnoredInArchive" && !issue.Blocking);
        Assert.False(validate.Value.CompiledParams["clear_store"]?.GetValue<bool>());
    }

    [Fact]
    public async Task SaveReclamation_ZeroCraftBatches_IsPreservedForWpfParity()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);

        var save = await fixture.TaskQueue.SaveReclamationParamsAsync(0, new ReclamationTaskParamsDto
        {
            Theme = "Tales",
            Mode = 1,
            IncrementMode = 0,
            NumCraftBatches = 0,
            ToolsToCraft = ["tool-a"],
            ClearStore = false,
        });
        Assert.True(save.Success, save.Message);

        var parameters = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        Assert.Equal(0, parameters["num_craft_batches"]?.GetValue<int>());

        var validate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(validate.Success, validate.Message);
        Assert.NotNull(validate.Value);
        Assert.DoesNotContain(validate.Value!.Issues, issue => issue.Code == "ReclamationNumCraftBatchesOutOfRange");
    }

    [Fact]
    public async Task SaveCustom_RejectsStructuredNames_AndNormalizesKnownTaskNames()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        var badSave = await fixture.TaskQueue.SaveCustomParamsAsync(0, new CustomTaskParamsDto
        {
            TaskNames = ["[Fight]"],
        });
        Assert.False(badSave.Success);

        var save = await fixture.TaskQueue.SaveCustomParamsAsync(0, new CustomTaskParamsDto
        {
            TaskNames = ["FightTask", "Mall", "Mall", "  FightTask  "],
        });
        Assert.True(save.Success, save.Message);

        var parameters = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        var taskNames = Assert.IsType<JsonArray>(parameters["task_names"]);
        Assert.Equal(new[] { "Fight", "Mall" }, taskNames.Select(node => node?.GetValue<string>()).ToArray());
    }

    [Fact]
    public async Task ValidateTask_ModuleC_CatchesTypeCorruptionAsBlocking()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Roguelike", "rogue")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Reclamation", "reclamation")).Success);
        Assert.True((await fixture.TaskQueue.AddTaskAsync("Custom", "custom")).Success);

        var rogueParams = (await fixture.TaskQueue.GetTaskParamsAsync(0)).Value!;
        rogueParams["collectible_mode_start_list"] = new JsonArray("invalid");
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, rogueParams)).Success);

        var reclamationParams = (await fixture.TaskQueue.GetTaskParamsAsync(1)).Value!;
        reclamationParams["tools_to_craft"] = new JsonObject { ["bad"] = true };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(1, reclamationParams)).Success);

        var customParams = (await fixture.TaskQueue.GetTaskParamsAsync(2)).Value!;
        customParams["task_names"] = new JsonObject { ["bad"] = "Fight" };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(2, customParams)).Success);

        var rogueValidate = await fixture.TaskQueue.ValidateTaskAsync(0);
        Assert.True(rogueValidate.Success);
        Assert.NotNull(rogueValidate.Value);
        Assert.Contains(rogueValidate.Value!.Issues, issue => issue.Code == "TaskFieldTypeInvalid" && issue.Blocking);

        var reclamationValidate = await fixture.TaskQueue.ValidateTaskAsync(1);
        Assert.True(reclamationValidate.Success);
        Assert.NotNull(reclamationValidate.Value);
        Assert.Contains(reclamationValidate.Value!.Issues, issue => issue.Code == "TaskFieldTypeInvalid" && issue.Blocking);

        var customValidate = await fixture.TaskQueue.ValidateTaskAsync(2);
        Assert.True(customValidate.Success);
        Assert.NotNull(customValidate.Value);
        Assert.Contains(customValidate.Value!.Issues, issue => issue.Code == "TaskFieldTypeInvalid" && issue.Blocking);
    }

    [Fact]
    public void TaskModuleCViews_AreBindingBased_NoStaticBusinessDefaults()
    {
        var root = ResolveRepoRoot();
        var roguelikeView = File.ReadAllText(Path.Combine(root, "src", "MAAUnified", "App", "Features", "TaskQueue", "RoguelikeSettingsView.axaml"));
        var reclamationView = File.ReadAllText(Path.Combine(root, "src", "MAAUnified", "App", "Features", "TaskQueue", "ReclamationSettingsView.axaml"));
        var customView = File.ReadAllText(Path.Combine(root, "src", "MAAUnified", "App", "Features", "TaskQueue", "CustomSettingsView.axaml"));

        Assert.Contains("{Binding SelectedThemeOption}", roguelikeView);
        Assert.Contains("{Binding SelectedModeOption}", roguelikeView);
        Assert.Contains("{Binding DelayAbortUntilCombatComplete}", roguelikeView);
        Assert.Contains("{Binding StatusMessage}", roguelikeView);
        Assert.DoesNotContain("傀影与猩红孤钻", roguelikeView);
        Assert.DoesNotContain("SelectedIndex=\"0\"", roguelikeView);

        Assert.Contains("{Binding SelectedThemeOption}", reclamationView);
        Assert.Contains("{Binding NumCraftBatches}", reclamationView);
        Assert.Contains("{Binding StatusMessage}", reclamationView);
        Assert.DoesNotContain("萨尔贡沙洲遗闻", reclamationView);

        Assert.Contains("{Binding TaskNamesText", customView);
        Assert.Contains("{Binding TaskNamesPreview}", customView);
        Assert.Contains("{Binding StatusMessage}", customView);
        Assert.DoesNotContain("自定义任务名规则", customView);
    }

    [Fact]
    public void Localization_HasModuleCKeys_ForAllSupportedLanguages()
    {
        var text = new LocalizedTextMap();
        var languages = new[] { "zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr", "pallas" };
        var requiredKeys = new[]
        {
            "Roguelike.Title",
            "Roguelike.Theme",
            "Roguelike.Mode",
            "Roguelike.InvestmentEnabled",
            "Roguelike.Collectible.Dice",
            "Roguelike.DelayAbortUntilCombatComplete",
            "RoguelikeThemePhantom",
            "Reclamation.Title",
            "Reclamation.ToolsToCraft",
            "Reclamation.Option.Mode.Archive",
            "Custom.Title",
            "Custom.TaskNamesPreview",
            "Common.LoadingTaskHint",
            "Common.ValidationIssues",
            "TaskQueue.Validation.Clean",
            "TaskQueue.Validation.BlockingCount",
            "TaskQueue.Error.BlockingValidation",
            "Issue.TaskFieldTypeInvalid",
            "Issue.DelimitedInputParseFailed",
            "Issue.RoguelikeModeInvalid",
            "Issue.CustomTaskNameInvalid",
            "Issue.ReclamationClearStoreIgnoredInArchive",
        };

        foreach (var language in languages)
        {
            text.Language = language;
            foreach (var key in requiredKeys)
            {
                var value = text[key];
                Assert.False(string.Equals(value, key, StringComparison.Ordinal), $"Missing `{key}` for `{language}`");
            }
        }
    }

    private static string ResolveRepoRoot()
    {
        var current = AppContext.BaseDirectory;
        for (var i = 0; i < 12; i++)
        {
            if (Directory.Exists(Path.Combine(current, "src", "MAAUnified")))
            {
                return current;
            }

            var parent = Directory.GetParent(current);
            if (parent is null)
            {
                break;
            }

            current = parent.FullName;
        }

        throw new DirectoryNotFoundException("Failed to locate repo root containing src/MAAUnified.");
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            UnifiedConfigurationService config,
            TaskQueueFeatureService taskQueue,
            FakeBridge bridge)
        {
            Root = root;
            Config = config;
            TaskQueue = taskQueue;
            Bridge = bridge;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public FakeBridge Bridge { get; }

        public static async Task<TestFixture> CreateAsync()
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));

            var log = new UiLogService();
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            var bridge = new FakeBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var taskQueue = new TaskQueueFeatureService(session, config);
            return new TestFixture(root, config, taskQueue, bridge);
        }

        public async ValueTask DisposeAsync()
        {
            await Bridge.DisposeAsync();
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary folder.
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
