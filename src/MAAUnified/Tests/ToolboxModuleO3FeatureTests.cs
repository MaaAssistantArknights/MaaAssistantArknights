using System.Text.Json.Nodes;
using Avalonia.Media;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.CoreBridge;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class ToolboxModuleO3FeatureTests
{
    [Fact]
    public async Task ApplyRuntimeCallback_RecruitCallbacks_ShouldProjectResultsAndCompleteRun()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        await vm.StartRecruitAsync();

        vm.ApplyRuntimeCallback(CreateCallback(
            "SubTaskExtraInfo",
            new JsonObject
            {
                ["taskchain"] = "Recruit",
                ["what"] = "RecruitTagsDetected",
                ["details"] = new JsonObject
                {
                    ["tags"] = new JsonArray("先锋干员", "费用回复"),
                },
            }));
        vm.ApplyRuntimeCallback(CreateCallback(
            "SubTaskExtraInfo",
            new JsonObject
            {
                ["taskchain"] = "Recruit",
                ["what"] = "RecruitResult",
                ["details"] = new JsonObject
                {
                    ["result"] = new JsonArray
                    {
                        new JsonObject
                        {
                            ["level"] = 4,
                            ["tags"] = new JsonArray("先锋干员", "费用回复"),
                            ["opers"] = new JsonArray
                            {
                                new JsonObject
                                {
                                    ["level"] = 5,
                                    ["id"] = "char_102_texas",
                                    ["name"] = "德克萨斯",
                                },
                                new JsonObject
                                {
                                    ["level"] = 3,
                                    ["id"] = "char_123_fang",
                                    ["name"] = "芬",
                                },
                            },
                        },
                    },
                },
            }));
        vm.ApplyRuntimeCallback(CreateCallback("TaskChainCompleted"));

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.Null(fixture.Runtime.SessionService.CurrentRunOwner);
        Assert.Contains("先锋干员", vm.RecruitInfo, StringComparison.Ordinal);
        Assert.True(vm.RecruitResultLines.Count >= 2);

        var operatorLine = Assert.Single(
            vm.RecruitResultLines,
            line => line.Text.Contains("德克萨斯", StringComparison.Ordinal));
        Assert.Equal(2, operatorLine.Segments.Count);
        var texasSegment = Assert.Single(
            operatorLine.Segments,
            segment => segment.Text.Contains("德克萨斯", StringComparison.Ordinal));
        var texasBrush = Assert.IsAssignableFrom<ISolidColorBrush>(texasSegment.Foreground);
        Assert.Equal(Colors.Orange, texasBrush.Color);

        var history = Assert.Single(vm.ExecutionHistory);
        Assert.True(history.Success);
        Assert.Equal("招募识别", history.ToolName);
        await WaitForSettingAsync(fixture, "Toolbox.ExecutionHistory");
    }

    [Fact]
    public async Task ApplyRuntimeCallback_OperBoxDone_ShouldPersistLegacyBoxData()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        await vm.StartOperBoxAsync();

        vm.ApplyRuntimeCallback(CreateCallback(
            "SubTaskExtraInfo",
            new JsonObject
            {
                ["taskchain"] = "OperBox",
                ["what"] = "OperBoxResult",
                ["details"] = new JsonObject
                {
                    ["own_opers"] = new JsonArray
                    {
                        new JsonObject
                        {
                            ["id"] = "char_003_kalts",
                            ["name"] = "凯尔希",
                            ["rarity"] = 6,
                            ["elite"] = 2,
                            ["level"] = 90,
                            ["potential"] = 6,
                        },
                    },
                    ["done"] = true,
                },
            }));
        vm.ApplyRuntimeCallback(CreateCallback("TaskChainCompleted"));

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.Single(vm.OperBoxHaveList);
        Assert.Equal("char_003_kalts", vm.OperBoxHaveList[0].Id);

        await WaitForSettingAsync(fixture, LegacyConfigurationKeys.OperBoxData, expectedSubstring: "char_003_kalts");
    }

    [Fact]
    public async Task ApplyRuntimeCallback_StageDrops_ShouldUpdateDepotAndPersistLegacyResult()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        vm.ApplyRuntimeCallback(CreateCallback(
            "SubTaskExtraInfo",
            new JsonObject
            {
                ["taskchain"] = "Fight",
                ["what"] = "StageDrops",
                ["details"] = new JsonObject
                {
                    ["stats"] = new JsonArray
                    {
                        new JsonObject
                        {
                            ["itemId"] = "2001",
                            ["itemName"] = "源岩",
                            ["addQuantity"] = 7,
                        },
                    },
                },
            }));

        var depotItem = Assert.Single(vm.DepotResult);
        Assert.Equal("2001", depotItem.Id);
        Assert.Equal(7, depotItem.Count);
        await WaitForSettingAsync(fixture, LegacyConfigurationKeys.DepotResult);
        Assert.Equal(7, ReadPersistedDepotCount(fixture, "2001"));
    }

    [Fact]
    public async Task ApplyRuntimeCallback_StageDrops_ShouldRemainCompatibleWithFightTaskHints()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();

        vm.ApplyRuntimeCallback(CreateCallback(
            "SubTaskExtraInfo",
            new JsonObject
            {
                ["taskchain"] = "Fight",
                ["what"] = "StageDrops",
                ["details"] = new JsonObject
                {
                    ["stats"] = new JsonArray
                    {
                        new JsonObject
                        {
                            ["itemId"] = "3231",
                            ["itemName"] = "重装芯片",
                            ["addQuantity"] = 7,
                        },
                    },
                },
            }));

        await WaitForSettingAsync(fixture, LegacyConfigurationKeys.DepotResult);
        var hint = FightTaskModuleViewModel.BuildDailyResourceHint(
            "zh-cn",
            "Official",
            fixture.Config.CurrentConfig,
            new DateTime(2026, 3, 15, 0, 0, 0, DateTimeKind.Utc));

        Assert.Contains("PR-A-1/2", hint, StringComparison.Ordinal);
        Assert.DoesNotContain("(库存", hint, StringComparison.Ordinal);
        Assert.DoesNotContain("-1", hint, StringComparison.Ordinal);
    }

    [Fact]
    public void ToolboxView_ShouldExposeAllToolActions_AndRemoveLegacyDemoActions()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Features", "Advanced", "ToolboxView.axaml"));

        Assert.Contains("Header=\"招募识别\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Header=\"干员识别\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Header=\"仓库识别\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Classes=\"toolbox-nav\"", xaml, StringComparison.Ordinal);
        Assert.Contains("ItemsSource=\"{Binding Segments}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Header=\"抽卡\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Header=\"窥屏\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Header=\"小游戏\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"开始识别\"", xaml, StringComparison.Ordinal);
        Assert.Contains("复制到剪切板", xaml, StringComparison.Ordinal);
        Assert.Contains("导出至企鹅物流刷图规划", xaml, StringComparison.Ordinal);
        Assert.Contains("导出至明日方舟工具箱", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"寻访一次\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"寻访十次\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"知道了\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"{Binding PeepCommandText}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"{Binding MiniGameCommandText}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Source=\"{Binding EliteIconImage}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Source=\"{Binding PotentialIconImage}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Source=\"{Binding ItemImage}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("x:Name=\"GachaDisclaimerEmphasisText\"", xaml, StringComparison.Ordinal);
        Assert.Contains("SpreadMethod=\"Repeat\"", xaml, StringComparison.Ordinal);
        Assert.Contains("DropShadowDirectionEffect", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("执行成功示例", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("执行失败示例", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("CurrentToolParameters", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("Text=\"状态\"", xaml, StringComparison.Ordinal);
    }

    private static CoreCallbackEvent CreateCallback(string msgName, JsonObject? payload = null)
    {
        return new CoreCallbackEvent(0, msgName, (payload ?? new JsonObject()).ToJsonString(), DateTimeOffset.Now);
    }

    private static async Task WaitForSettingAsync(ToolboxTestFixture fixture, string key, string? expectedSubstring = null)
    {
        for (var attempt = 0; attempt < 50; attempt++)
        {
            var value = ReadGlobalString(fixture, key);
            if (!string.IsNullOrWhiteSpace(value)
                && (string.IsNullOrWhiteSpace(expectedSubstring) || value.Contains(expectedSubstring, StringComparison.Ordinal)))
            {
                return;
            }

            await Task.Delay(20);
        }

        var current = ReadGlobalString(fixture, key);
        Assert.True(
            !string.IsNullOrWhiteSpace(current)
            && (string.IsNullOrWhiteSpace(expectedSubstring) || current.Contains(expectedSubstring, StringComparison.Ordinal)),
            $"Expected setting `{key}` to contain `{expectedSubstring}`, but got `{current}`.");
    }

    private static string ReadGlobalString(ToolboxTestFixture fixture, string key)
    {
        if (fixture.Config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return node.ToString();
        }

        return string.Empty;
    }

    private static int ReadPersistedDepotCount(ToolboxTestFixture fixture, string itemId)
    {
        var payload = ReadGlobalString(fixture, LegacyConfigurationKeys.DepotResult);
        var root = JsonNode.Parse(payload) as JsonObject;
        var dataPayload = root?["data"]?.ToString();
        var data = string.IsNullOrWhiteSpace(dataPayload) ? null : JsonNode.Parse(dataPayload) as JsonObject;
        var countText = data?[itemId]?.ToString();
        return int.TryParse(countText, out var count) ? count : 0;
    }

    private static string GetMaaUnifiedRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "App");
            var testsDir = Path.Combine(current.FullName, "Tests");
            if (Directory.Exists(appDir) && Directory.Exists(testsDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Could not locate src/MAAUnified root.");
    }
}
