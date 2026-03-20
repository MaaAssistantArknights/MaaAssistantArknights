using System.Reflection;
using System.Text.Json;
using MAAUnified.CoreBridge;

namespace MAAUnified.Tests;

public sealed class MaaCoreBridgeNativeClientTypeTests
{
    [Theory]
    [InlineData(null, "")]
    [InlineData("", "")]
    [InlineData(" official ", "Official")]
    [InlineData("BILIBILI", "Bilibili")]
    [InlineData("txwy", "txwy")]
    [InlineData("Txwy", "txwy")]
    [InlineData("yostaren", "YoStarEN")]
    [InlineData("YOSTARJP", "YoStarJP")]
    [InlineData("YoStarKR", "YoStarKR")]
    [InlineData("CustomClient", "CustomClient")]
    public void NormalizeClientType_ShouldCanonicalizeKnownValues(string? rawClientType, string expected)
    {
        var method = typeof(MaaCoreBridgeNative).GetMethod(
            "NormalizeClientType",
            BindingFlags.NonPublic | BindingFlags.Static);
        Assert.NotNull(method);

        var normalized = method!.Invoke(null, [rawClientType]) as string;
        Assert.Equal(expected, normalized);
    }

    [Fact]
    public void TryResolveClientResourcePath_ShouldMatchDirectoryName_CaseInsensitive()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-core-bridge-client-type", Guid.NewGuid().ToString("N"));
        try
        {
            Directory.CreateDirectory(Path.Combine(root, "resource", "global", "YoStarEN", "resource"));
            var method = typeof(MaaCoreBridgeNative).GetMethod(
                "TryResolveClientResourcePath",
                BindingFlags.NonPublic | BindingFlags.Static);
            Assert.NotNull(method);

            object?[] args =
            [
                root,
                "yostaren",
                string.Empty,
                string.Empty,
            ];

            var found = method!.Invoke(null, args);
            Assert.IsType<bool>(found);
            Assert.True((bool)found!);
            Assert.Equal("YoStarEN", Assert.IsType<string>(args[2]));
            Assert.EndsWith(
                Path.Combine("resource", "global", "YoStarEN", "resource"),
                Assert.IsType<string>(args[3]),
                StringComparison.Ordinal);
        }
        finally
        {
            try
            {
                Directory.Delete(root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    [Fact]
    public void TryResolveClientResourcePath_WhenClientResourceMissing_ShouldReturnFalse()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-core-bridge-client-type", Guid.NewGuid().ToString("N"));
        try
        {
            Directory.CreateDirectory(Path.Combine(root, "resource", "global", "YoStarEN"));
            var method = typeof(MaaCoreBridgeNative).GetMethod(
                "TryResolveClientResourcePath",
                BindingFlags.NonPublic | BindingFlags.Static);
            Assert.NotNull(method);

            object?[] args =
            [
                root,
                "YoStarEN",
                string.Empty,
                string.Empty,
            ];

            var found = method!.Invoke(null, args);
            Assert.IsType<bool>(found);
            Assert.False((bool)found!);
        }
        finally
        {
            try
            {
                Directory.Delete(root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    [Fact]
    public void BuildConnectionFailureMessage_ShouldExposeRawCommandOutput()
    {
        var method = typeof(MaaCoreBridgeNative).GetMethod(
            "BuildConnectionFailureMessage",
            BindingFlags.NonPublic | BindingFlags.Static);
        Assert.NotNull(method);

        using var doc = JsonDocument.Parse(
            """
            {
              "what": "ConnectFailed",
              "why": "Connection command failed to exec",
              "details": {
                "raw_output": "由于找不到 AdbWinApi.dll，无法继续执行代码。\r\n重新安装程序可能会解决此问题。"
              }
            }
            """);

        var message = method!.Invoke(null, [doc.RootElement.Clone(), "ConnectFailed"]) as string;

        Assert.Equal(
            "由于找不到 AdbWinApi.dll，无法继续执行代码。\n重新安装程序可能会解决此问题。",
            message);
    }

    [Fact]
    public void BuildConnectionFailureMessage_ShouldReturnWhy_WhenRawOutputMissing()
    {
        var method = typeof(MaaCoreBridgeNative).GetMethod(
            "BuildConnectionFailureMessage",
            BindingFlags.NonPublic | BindingFlags.Static);
        Assert.NotNull(method);

        using var doc = JsonDocument.Parse(
            """
            {
              "what": "ConnectFailed",
              "why": "ConfigNotFound",
              "details": {}
            }
            """);

        var message = method!.Invoke(null, [doc.RootElement.Clone(), "ConnectFailed"]) as string;

        Assert.Equal("ConfigNotFound", message);
    }
}
