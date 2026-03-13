using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services.Features;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class ToolboxModuleO1FeatureTests
{
    [Fact]
    public async Task ExecuteToolAsync_Recruit_ShouldAppendStructuredRecruitTask()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var service = new ToolboxFeatureService(fixture.Bridge, fixture.Runtime.ConnectFeatureService);

        var request = new ToolboxDispatchRequest(
            ToolboxToolKind.Recruit,
            Recruit: new ToolboxRecruitRequest([3, 4, 5], AutoSetTime: true, Level3Time: 540, Level4Time: 530, Level5Time: 520, ServerType: "JP"),
            ParameterSummary: "select=3,4,5");

        var result = await service.DispatchToolAsync(request);

        Assert.True(result.Success);
        var task = Assert.Single(fixture.Bridge.AppendedTasks);
        Assert.Equal("Recruit", task.Type);

        var payload = Assert.IsType<JsonObject>(JsonNode.Parse(task.ParamsJson));
        Assert.Equal(true, payload["set_time"]?.GetValue<bool>());
        Assert.Equal("JP", payload["server"]?.GetValue<string>());
        var levels = Assert.IsType<JsonArray>(payload["select"]);
        Assert.Equal(["3", "4", "5"], levels.Select(item => item?.ToString() ?? string.Empty).ToArray());
    }

    [Fact]
    public async Task ExecuteToolAsync_GachaAndMiniGame_ShouldAppendCustomTasks()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var service = new ToolboxFeatureService(fixture.Bridge, fixture.Runtime.ConnectFeatureService);

        var gachaResult = await service.DispatchToolAsync(
            new ToolboxDispatchRequest(
                ToolboxToolKind.Gacha,
                Gacha: new ToolboxGachaRequest(Once: false),
                ParameterSummary: "drawCount=10"));
        var miniGameResult = await service.DispatchToolAsync(
            new ToolboxDispatchRequest(
                ToolboxToolKind.MiniGame,
                MiniGame: new ToolboxMiniGameRequest("MiniGame@SecretFront@Begin@EndingC@游侠"),
                ParameterSummary: "taskName=MiniGame@SecretFront@Begin@EndingC@游侠"));

        Assert.True(gachaResult.Success);
        Assert.True(miniGameResult.Success);
        Assert.Equal(2, fixture.Bridge.AppendedTasks.Count);

        var gachaPayload = Assert.IsType<JsonObject>(JsonNode.Parse(fixture.Bridge.AppendedTasks[0].ParamsJson));
        var miniGamePayload = Assert.IsType<JsonObject>(JsonNode.Parse(fixture.Bridge.AppendedTasks[1].ParamsJson));
        Assert.Equal("Custom", fixture.Bridge.AppendedTasks[0].Type);
        Assert.Equal("Custom", fixture.Bridge.AppendedTasks[1].Type);
        Assert.Equal("GachaTenTimes", Assert.IsType<JsonArray>(gachaPayload["task_names"])[0]?.ToString());
        Assert.Equal("MiniGame@SecretFront@Begin@EndingC@游侠", Assert.IsType<JsonArray>(miniGamePayload["task_names"])[0]?.ToString());
    }

    [Fact]
    public async Task ExecuteToolAsync_Peep_ShouldFailAsNoCoreTaskIsRequired()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var service = new ToolboxFeatureService(fixture.Bridge, fixture.Runtime.ConnectFeatureService);

        var result = await service.DispatchToolAsync(new ToolboxDispatchRequest(ToolboxToolKind.VideoRecognition));

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolNotSupported, result.Error?.Code);
        Assert.Empty(fixture.Bridge.AppendedTasks);
    }

    [Fact]
    public async Task StartRecruitAsync_WhenAnotherOwnerActive_ShouldFailWithoutAppend()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();
        Assert.True(fixture.Runtime.SessionService.TryBeginRun("TaskQueue", out _));

        await vm.StartRecruitAsync();

        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.Equal(UiErrorCode.ToolboxExecutionFailed, vm.LastExecutionErrorCode);
        Assert.Empty(fixture.Bridge.AppendedTasks);
        fixture.Runtime.SessionService.EndRun("TaskQueue");
    }

    [Fact]
    public async Task StartRecruitAsync_WhenCopilotOwnerActive_ShouldFailWithoutAppend()
    {
        await using var fixture = await ToolboxTestFixture.CreateAsync();
        var vm = new ToolboxPageViewModel(fixture.Runtime, fixture.ConnectionState);
        await vm.InitializeAsync();
        Assert.True(fixture.Runtime.SessionService.TryBeginRun("Copilot", out _));

        await vm.StartRecruitAsync();

        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.Equal(UiErrorCode.ToolboxExecutionFailed, vm.LastExecutionErrorCode);
        Assert.Empty(fixture.Bridge.AppendedTasks);
        fixture.Runtime.SessionService.EndRun("Copilot");
    }
}
