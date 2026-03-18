using MAAUnified.CoreBridge;

namespace MAAUnified.Tests;

public sealed class MaaCoreBridgePostActionTests
{
    [Fact]
    public async Task MaaCoreBridgeStub_BackToHome_ShouldRequireConnectedSession()
    {
        await using var bridge = new MaaCoreBridgeStub();

        var beforeConnect = await bridge.BackToHomeAsync();
        Assert.False(beforeConnect.Success);
        Assert.Equal(CoreErrorCode.NotInitialized, beforeConnect.Error?.Code);

        var init = await bridge.InitializeAsync(new CoreInitializeRequest(Path.GetTempPath(), "Official"));
        Assert.True(init.Success);
        var connect = await bridge.ConnectAsync(new CoreConnectionInfo("127.0.0.1:5555", "General", "adb"));
        Assert.True(connect.Success);

        var afterConnect = await bridge.BackToHomeAsync();
        Assert.True(afterConnect.Success);
    }

    [Fact]
    public async Task MaaCoreBridgeStub_StartCloseDown_ShouldStartRuntimeAfterAppend()
    {
        await using var bridge = new MaaCoreBridgeStub();
        Assert.True((await bridge.InitializeAsync(new CoreInitializeRequest(Path.GetTempPath(), "Official"))).Success);
        Assert.True((await bridge.ConnectAsync(new CoreConnectionInfo("127.0.0.1:5555", "General", "adb"))).Success);

        var result = await bridge.StartCloseDownAsync("YoStarEN");

        Assert.True(result.Success);
        var status = await bridge.GetRuntimeStatusAsync();
        Assert.True(status.Success);
        Assert.True(status.Value!.Running);
    }
}
