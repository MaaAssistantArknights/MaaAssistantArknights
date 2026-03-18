using System.Reflection;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class PostActionExecutorSupportTests
{
    [Fact]
    public void CommandPostActionExecutorService_GetCapabilityMatrix_ShouldReportSupportedEmulatorExitForRecognizedLocalContext()
    {
        var service = new CommandPostActionExecutorService();
        var request = OperatingSystem.IsWindows()
            ? new PostActionExecutorRequest(ConnectAddress: "127.0.0.1:5555", ConnectConfig: "LDPlayer")
            : new PostActionExecutorRequest(ConnectAddress: "127.0.0.1:5555", ConnectConfig: "General", AdbPath: "adb");

        var capability = service.GetCapabilityMatrix(request).Get(PostActionType.ExitEmulator);

        Assert.True(capability.Supported, capability.Message);
    }

    [Fact]
    public void WindowsEmulatorExitHelper_TryResolveMuMuIndex_ShouldPreferBridgeIndexAndPortRules()
    {
        var helperType = typeof(CommandPostActionExecutorService).Assembly.GetType("MAAUnified.Platform.WindowsEmulatorExitHelper");
        Assert.NotNull(helperType);

        var method = helperType!.GetMethod(
            "TryResolveMuMuIndex",
            BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
        Assert.NotNull(method);

        var byBridge = new PostActionExecutorRequest(
            ConnectAddress: "127.0.0.1:16416",
            ConnectConfig: "MuMuEmulator12",
            MuMuBridgeConnection: true,
            MuMu12Index: "3");
        var byPort = new PostActionExecutorRequest(
            ConnectAddress: "127.0.0.1:16448",
            ConnectConfig: "MuMuEmulator12");

        object?[] bridgeArgs = [byBridge, 0];
        object?[] portArgs = [byPort, 0];
        Assert.True((bool)method!.Invoke(null, bridgeArgs)!);
        Assert.Equal(3, Assert.IsType<int>(bridgeArgs[1]));
        Assert.True((bool)method.Invoke(null, portArgs)!);
        Assert.Equal(2, Assert.IsType<int>(portArgs[1]));
    }

    [Fact]
    public void WindowsEmulatorExitHelper_TryResolveLdPlayerIndex_ShouldSupportManualAndSerialFormats()
    {
        var helperType = typeof(CommandPostActionExecutorService).Assembly.GetType("MAAUnified.Platform.WindowsEmulatorExitHelper");
        Assert.NotNull(helperType);

        var method = helperType!.GetMethod(
            "TryResolveLdPlayerIndex",
            BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
        Assert.NotNull(method);

        var manual = new PostActionExecutorRequest(
            ConnectAddress: "127.0.0.1:5555",
            ConnectConfig: "LDPlayer",
            LdPlayerManualSetIndex: true,
            LdPlayerIndex: "4");
        var serial = new PostActionExecutorRequest(
            ConnectAddress: "emulator-5558",
            ConnectConfig: "LDPlayer");

        object?[] manualArgs = [manual, 0];
        object?[] serialArgs = [serial, 0];
        Assert.True((bool)method!.Invoke(null, manualArgs)!);
        Assert.Equal(4, Assert.IsType<int>(manualArgs[1]));
        Assert.True((bool)method.Invoke(null, serialArgs)!);
        Assert.Equal(2, Assert.IsType<int>(serialArgs[1]));
    }

    [Fact]
    public void AndroidEmulatorAdbHelper_TryBuildKillCommand_ShouldTranslateLoopbackAddressToSerial()
    {
        var helperType = typeof(CommandPostActionExecutorService).Assembly.GetType("MAAUnified.Platform.AndroidEmulatorAdbHelper");
        Assert.NotNull(helperType);

        var method = helperType!.GetMethod(
            "TryBuildKillCommand",
            BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
        Assert.NotNull(method);

        var request = new PostActionExecutorRequest(
            ConnectAddress: "127.0.0.1:5555",
            ConnectConfig: "General",
            AdbPath: "/tmp/fake-adb");
        object?[] args = [request, string.Empty, (string.Empty, string.Empty)];

        var built = method!.Invoke(null, args);
        Assert.IsType<bool>(built);
        Assert.True((bool)built!);
        Assert.Equal("emulator-5554", Assert.IsType<string>(args[1]));
        Assert.Equal(("/tmp/fake-adb", "-s emulator-5554 emu kill"), Assert.IsType<(string, string)>(args[2]));
    }
}
