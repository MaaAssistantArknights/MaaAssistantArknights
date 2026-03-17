namespace MAAUnified.Tests;

public sealed class BaselineKeyCoverageClosureTests
{
    [Fact]
    public void RemoteControlIdentityKeys_ShouldHaveSettingsViewModelReadWriteEvidence()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var keySet = baseline.ConfigKeyMappings.Select(mapping => mapping.Key).ToHashSet(StringComparer.Ordinal);
        Assert.Contains("RemoteControl.RemoteControlUserIdentity", keySet);
        Assert.Contains("RemoteControl.RemoteControlDeviceIdentity", keySet);

        var root = BaselineTestSupport.GetMaaUnifiedRoot();
        var vmPath = Path.Combine(root, "App", "ViewModels", "Settings", "SettingsPageViewModel.cs");
        var vmSource = File.ReadAllText(vmPath);

        Assert.Contains("[ConfigurationKeys.RemoteControlUserIdentity] = normalizedUserIdentity", vmSource, StringComparison.Ordinal);
        Assert.Contains("[ConfigurationKeys.RemoteControlDeviceIdentity] = normalizedDeviceIdentity", vmSource, StringComparison.Ordinal);
        Assert.Contains("ReadProfileString(config, ConfigurationKeys.RemoteControlUserIdentity", vmSource, StringComparison.Ordinal);
        Assert.Contains("ReadProfileString(config, ConfigurationKeys.RemoteControlDeviceIdentity", vmSource, StringComparison.Ordinal);
    }

    [Fact]
    public void DingTalkKeys_ShouldHaveProviderMappingValidationAndSendEvidence()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var keySet = baseline.ConfigKeyMappings.Select(mapping => mapping.Key).ToHashSet(StringComparer.Ordinal);
        Assert.Contains("ExternalNotification.DingTalk.AccessToken", keySet);
        Assert.Contains("ExternalNotification.DingTalk.Secret", keySet);

        var root = BaselineTestSupport.GetMaaUnifiedRoot();
        var vmPath = Path.Combine(root, "App", "ViewModels", "Settings", "SettingsPageViewModel.cs");
        var vmSource = File.ReadAllText(vmPath);
        Assert.Contains("[\"DingTalk\"] = new Dictionary<string, string>", vmSource, StringComparison.Ordinal);
        Assert.Contains("[\"accessToken\"] = ConfigurationKeys.ExternalNotificationDingTalkAccessToken", vmSource, StringComparison.Ordinal);
        Assert.Contains("[\"secret\"] = ConfigurationKeys.ExternalNotificationDingTalkSecret", vmSource, StringComparison.Ordinal);

        var servicePath = Path.Combine(root, "Application", "Services", "Features", "FeatureServices.cs");
        var serviceSource = File.ReadAllText(servicePath);
        Assert.Contains("if (provider == \"DingTalk\")", serviceSource, StringComparison.Ordinal);
        Assert.Contains("DingTalk requires `accessToken`.", serviceSource, StringComparison.Ordinal);
        Assert.Contains("return await SendDingTalkAsync(parameters, title, message, cancellationToken);", serviceSource, StringComparison.Ordinal);
    }
}
