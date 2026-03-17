using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Models;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class ToolboxModuleO2FeatureTests
{
    [Fact]
    public async Task InitializeAsync_ShouldLoadBridgeSettingsAndPersistedToolData()
    {
        var globalSeeds = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [LegacyConfigurationKeys.ChooseLevel3] = "false",
            [LegacyConfigurationKeys.ChooseLevel4] = "true",
            [LegacyConfigurationKeys.ChooseLevel5] = "true",
            [LegacyConfigurationKeys.ChooseLevel6] = "false",
            [LegacyConfigurationKeys.ToolBoxChooseLevel3Time] = "510",
            [LegacyConfigurationKeys.ToolBoxChooseLevel4Time] = "520",
            [LegacyConfigurationKeys.ToolBoxChooseLevel5Time] = "530",
            [LegacyConfigurationKeys.AutoSetTime] = "false",
            [LegacyConfigurationKeys.RecruitmentShowPotential] = "false",
            [LegacyConfigurationKeys.GachaShowDisclaimerNoMore] = "true",
            [LegacyConfigurationKeys.PeepTargetFps] = "37",
            [LegacyConfigurationKeys.MiniGameTaskName] = "MiniGame@SecretFront",
            [LegacyConfigurationKeys.MiniGameSecretFrontEnding] = "D",
            [LegacyConfigurationKeys.MiniGameSecretFrontEvent] = "游侠",
            [LegacyConfigurationKeys.OperBoxData] = "[{\"id\":\"char_003_kalts\",\"name\":\"凯尔希\",\"rarity\":6,\"elite\":2,\"level\":90,\"own\":true,\"potential\":6}]",
            [LegacyConfigurationKeys.DepotResult] = "{\"done\":true,\"data\":\"{\\\"2001\\\":123}\",\"syncTime\":\"2026-03-12T08:00:00.0000000+00:00\"}",
        };

        await using var fixture = await ToolboxTestFixture.CreateAsync(globalSeeds);
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);

        await vm.InitializeAsync();

        Assert.False(vm.ChooseLevel3);
        Assert.True(vm.ChooseLevel4);
        Assert.True(vm.ChooseLevel5);
        Assert.False(vm.ChooseLevel6);
        Assert.Equal(510, vm.RecruitLevel3Time);
        Assert.Equal(520, vm.RecruitLevel4Time);
        Assert.Equal(530, vm.RecruitLevel5Time);
        Assert.False(vm.RecruitAutoSetTime);
        Assert.False(vm.RecruitmentShowPotential);
        Assert.True(vm.GachaShowDisclaimerNoMore);
        Assert.False(vm.GachaShowDisclaimer);
        Assert.Equal(37, vm.PeepTargetFps);
        Assert.Equal("MiniGame@SecretFront", vm.MiniGameTaskName);
        Assert.Equal("D", vm.MiniGameSecretFrontEnding);
        Assert.Equal("游侠", vm.MiniGameSecretFrontEvent);
        Assert.Equal("MiniGame@SecretFront@Begin@EndingD@游侠", vm.GetMiniGameTask());
        Assert.Single(vm.OperBoxHaveList);
        Assert.Single(vm.DepotResult);
        Assert.Contains("char_003_kalts", vm.OperBoxExportText, StringComparison.Ordinal);
        Assert.Contains("@penguin-statistics/depot", vm.ArkPlannerResult, StringComparison.Ordinal);
        Assert.Contains("2001", vm.LoliconResult, StringComparison.Ordinal);
    }

    [Fact]
    public async Task InitializeAsync_ShouldResolveToolboxImagesAndDefaultMiniGameTips()
    {
        var globalSeeds = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [LegacyConfigurationKeys.OperBoxData] = "[{\"id\":\"char_003_kalts\",\"name\":\"凯尔希\",\"rarity\":6,\"elite\":2,\"level\":90,\"own\":true,\"potential\":6}]",
            [LegacyConfigurationKeys.DepotResult] = "{\"done\":true,\"data\":\"{\\\"2001\\\":123}\"}",
        };

        await using var fixture = await ToolboxTestFixture.CreateAsync(globalSeeds);
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);

        await vm.InitializeAsync();

        Assert.NotNull(ToolboxAssetCatalog.ResolveOperatorEliteAssetPath(vm.OperBoxHaveList[0].Elite));
        Assert.NotNull(ToolboxAssetCatalog.ResolveOperatorPotentialAssetPath(vm.OperBoxHaveList[0].Potential));
        Assert.NotNull(ToolboxAssetCatalog.ResolveItemImagePath(vm.DepotResult[0].Id));

        vm.MiniGameTaskName = "SS@Store@Begin";
        Assert.Equal("请在活动商店页面开始。\n不买无限池。", vm.MiniGameTip);
        vm.MiniGameTaskName = "MiniGame@SecretFront";
        Assert.Equal("在选小队界面开始，如有存档须手动删除。\n第一次打自己看完把教程关了。\n推荐勾选游戏内「继承上一支队伍发回的数据」", vm.MiniGameTip);
    }

    [Fact]
    public async Task StartRecruitAsync_ShouldConnectAppendStartAndPersistSettings()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync(
            profileSeeds: new Dictionary<string, JsonNode?>
            {
                ["ServerType"] = JsonValue.Create("KR"),
            });
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();
        vm.ChooseLevel3 = true;
        vm.ChooseLevel4 = false;
        vm.ChooseLevel5 = true;
        vm.ChooseLevel6 = true;
        vm.RecruitLevel3Time = 500;
        vm.RecruitLevel4Time = 510;
        vm.RecruitLevel5Time = 520;

        await vm.StartRecruitAsync();

        Assert.Equal(ToolboxExecutionState.Executing, vm.ExecutionState);
        Assert.Equal(1, fixture.Bridge.ConnectCallCount);
        Assert.Equal(1, fixture.Bridge.StartCallCount);
        var task = Assert.Single(fixture.Bridge.AppendedTasks);
        Assert.Equal("Recruit", task.Type);
        var payload = Assert.IsType<JsonObject>(JsonNode.Parse(task.ParamsJson));
        Assert.Equal("KR", payload["server"]?.GetValue<string>());
        Assert.Equal("500", fixture.Config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.ToolBoxChooseLevel3Time]?.ToString());
        Assert.Equal("520", fixture.Config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.ToolBoxChooseLevel5Time]?.ToString());
        Assert.Equal("Toolbox", fixture.Runtime.SessionService.CurrentRunOwner);
    }

    [Fact]
    public async Task StartGachaAsync_ShouldStartCustomTaskAndAutoPeep()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();
        vm.AgreeGachaDisclaimer();

        await vm.StartGachaAsync(once: false);

        Assert.Equal(ToolboxExecutionState.Executing, vm.ExecutionState);
        Assert.True(vm.IsGachaInProgress);
        Assert.True(vm.Peeping);
        Assert.Equal(1, fixture.Bridge.StartCallCount);
        Assert.Single(fixture.Bridge.AppendedTasks);
        Assert.Contains("GachaTenTimes", fixture.Bridge.AppendedTasks[0].ParamsJson, StringComparison.Ordinal);
    }

    [Fact]
    public async Task TogglePeepAsync_ShouldAcquireOwnerAndReleaseOnSecondToggle()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        await vm.TogglePeepAsync();
        Assert.True(vm.Peeping);
        Assert.Equal("Toolbox", fixture.Runtime.SessionService.CurrentRunOwner);

        await vm.TogglePeepAsync();
        Assert.False(vm.Peeping);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);
    }

    [Fact]
    public async Task StopActiveToolAsync_WhenRecruitRunning_ShouldStopViaToolboxService()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        await vm.StartRecruitAsync();
        await vm.StopActiveToolAsync();

        Assert.Equal(1, fixture.Bridge.StopCallCount);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);
        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.Equal(UiErrorCode.ToolboxExecutionCancelled, vm.LastExecutionErrorCode);
    }

    [Fact]
    public async Task InitializeAsync_ShouldLoadMiniGameEntriesFromStageActivityOverride()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        Directory.CreateDirectory(Path.Combine(fixture.Root, "gui"));
        await File.WriteAllTextAsync(
            Path.Combine(fixture.Root, "gui", "StageActivityV2.json"),
            """
            {
              "Official": {
                "miniGame": [
                  {
                    "Display": "测试小游戏",
                    "Value": "MiniGame@Test@Begin",
                    "Tip": "测试提示"
                  }
                ]
              }
            }
            """);

        using var _ = ToolboxAssetCatalog.PushTestBaseDirectoriesForTests(fixture.Root);
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);

        await vm.InitializeAsync();

        var entry = Assert.Single(vm.MiniGameTaskList, item => item.Value == "MiniGame@Test@Begin");
        Assert.Equal("测试小游戏", entry.Display);
        Assert.Equal("测试提示", entry.Tip);
    }
}
