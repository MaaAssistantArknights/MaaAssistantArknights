namespace MAAUnified.Tests;

public sealed class SettingsRemoteControlViewBindingTests
{
    [Fact]
    public void RemoteControlSettingsView_ShouldBindUserAndDeviceIdentity()
    {
        var root = BaselineTestSupport.GetMaaUnifiedRoot();
        var path = Path.Combine(root, "App", "Features", "Settings", "RemoteControlSettingsView.axaml");
        var xaml = File.ReadAllText(path);

        Assert.Matches(
            "Text=\"\\{Binding RemoteUserIdentity(?:,\\s*UpdateSourceTrigger=LostFocus)?\\}\"",
            xaml);
        Assert.Matches(
            "Text=\"\\{Binding RemoteDeviceIdentity(?:,\\s*UpdateSourceTrigger=LostFocus)?\\}\"",
            xaml);
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
