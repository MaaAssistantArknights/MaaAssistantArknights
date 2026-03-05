namespace MAAUnified.Tests;

public sealed class SettingsRemoteControlViewBindingTests
{
    [Fact]
    public void RemoteControlSettingsView_ShouldBindUserAndDeviceIdentity()
    {
        var root = BaselineTestSupport.GetMaaUnifiedRoot();
        var path = Path.Combine(root, "App", "Features", "Settings", "RemoteControlSettingsView.axaml");
        var xaml = File.ReadAllText(path);

        Assert.Contains("Text=\"{Binding RemoteUserIdentity}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Text=\"{Binding RemoteDeviceIdentity}\"", xaml, StringComparison.Ordinal);
    }

    [Fact]
    public void RemoteControlSettingsView_ShouldNotContainLegacyUnboundIdentityPlaceholder()
    {
        var root = BaselineTestSupport.GetMaaUnifiedRoot();
        var path = Path.Combine(root, "App", "Features", "Settings", "RemoteControlSettingsView.axaml");
        var xaml = File.ReadAllText(path);

        Assert.DoesNotContain("userId-deviceId", xaml, StringComparison.Ordinal);
    }
}
