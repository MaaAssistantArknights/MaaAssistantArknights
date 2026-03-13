using MAAUnified.Compat.Constants;

namespace MAAUnified.Application.Models;

public sealed record VersionUpdatePolicy(
    string Proxy,
    string ProxyType,
    string VersionType,
    string ResourceUpdateSource,
    bool ForceGithubGlobalSource,
    string MirrorChyanCdk,
    string MirrorChyanCdkExpired,
    bool StartupUpdateCheck,
    bool ScheduledUpdateCheck,
    string ResourceApi,
    bool AllowNightlyUpdates,
    bool HasAcknowledgedNightlyWarning,
    bool UseAria2,
    bool AutoDownloadUpdatePackage,
    bool AutoInstallUpdatePackage,
    string VersionName,
    string VersionBody,
    bool IsFirstBoot,
    string VersionPackage,
    bool DoNotShowUpdate)
{
    public static VersionUpdatePolicy Default { get; } = new(
        Proxy: string.Empty,
        ProxyType: "http",
        VersionType: "Stable",
        ResourceUpdateSource: "Github",
        ForceGithubGlobalSource: false,
        MirrorChyanCdk: string.Empty,
        MirrorChyanCdkExpired: string.Empty,
        StartupUpdateCheck: true,
        ScheduledUpdateCheck: false,
        ResourceApi: string.Empty,
        AllowNightlyUpdates: false,
        HasAcknowledgedNightlyWarning: false,
        UseAria2: false,
        AutoDownloadUpdatePackage: true,
        AutoInstallUpdatePackage: false,
        VersionName: string.Empty,
        VersionBody: string.Empty,
        IsFirstBoot: false,
        VersionPackage: string.Empty,
        DoNotShowUpdate: false);

    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.UpdateProxy] = Proxy,
            [ConfigurationKeys.ProxyType] = ProxyType,
            [ConfigurationKeys.VersionType] = VersionType,
            [ConfigurationKeys.UpdateSource] = ResourceUpdateSource,
            [ConfigurationKeys.ForceGithubGlobalSource] = ForceGithubGlobalSource.ToString(),
            [ConfigurationKeys.MirrorChyanCdk] = MirrorChyanCdk,
            [ConfigurationKeys.MirrorChyanCdkExpiredTime] = MirrorChyanCdkExpired,
            [ConfigurationKeys.StartupUpdateCheck] = StartupUpdateCheck.ToString(),
            [ConfigurationKeys.UpdateAutoCheck] = ScheduledUpdateCheck.ToString(),
            [ConfigurationKeys.ResourceApi] = ResourceApi,
            [ConfigurationKeys.AllowNightlyUpdates] = AllowNightlyUpdates.ToString(),
            [ConfigurationKeys.HasAcknowledgedNightlyWarning] = HasAcknowledgedNightlyWarning.ToString(),
            [ConfigurationKeys.UseAria2] = UseAria2.ToString(),
            [ConfigurationKeys.AutoDownloadUpdatePackage] = AutoDownloadUpdatePackage.ToString(),
            [ConfigurationKeys.AutoInstallUpdatePackage] = AutoInstallUpdatePackage.ToString(),
            [ConfigurationKeys.VersionName] = VersionName,
            [ConfigurationKeys.VersionUpdateBody] = VersionBody,
            [ConfigurationKeys.VersionUpdateIsFirstBoot] = IsFirstBoot.ToString(),
            [ConfigurationKeys.VersionUpdatePackage] = VersionPackage,
            [ConfigurationKeys.VersionUpdateDoNotShowUpdate] = DoNotShowUpdate.ToString(),
        };
    }

    public IReadOnlyDictionary<string, string> ToChannelSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.VersionType] = VersionType,
            [ConfigurationKeys.UpdateSource] = ResourceUpdateSource,
            [ConfigurationKeys.ForceGithubGlobalSource] = ForceGithubGlobalSource.ToString(),
            [ConfigurationKeys.MirrorChyanCdk] = MirrorChyanCdk,
            [ConfigurationKeys.MirrorChyanCdkExpiredTime] = MirrorChyanCdkExpired,
            [ConfigurationKeys.StartupUpdateCheck] = StartupUpdateCheck.ToString(),
            [ConfigurationKeys.UpdateAutoCheck] = ScheduledUpdateCheck.ToString(),
            [ConfigurationKeys.AllowNightlyUpdates] = AllowNightlyUpdates.ToString(),
            [ConfigurationKeys.HasAcknowledgedNightlyWarning] = HasAcknowledgedNightlyWarning.ToString(),
            [ConfigurationKeys.UseAria2] = UseAria2.ToString(),
            [ConfigurationKeys.AutoDownloadUpdatePackage] = AutoDownloadUpdatePackage.ToString(),
            [ConfigurationKeys.AutoInstallUpdatePackage] = AutoInstallUpdatePackage.ToString(),
            [ConfigurationKeys.VersionName] = VersionName,
            [ConfigurationKeys.VersionUpdateBody] = VersionBody,
            [ConfigurationKeys.VersionUpdateIsFirstBoot] = IsFirstBoot.ToString(),
            [ConfigurationKeys.VersionUpdatePackage] = VersionPackage,
            [ConfigurationKeys.VersionUpdateDoNotShowUpdate] = DoNotShowUpdate.ToString(),
        };
    }

    public IReadOnlyDictionary<string, string> ToProxySettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.UpdateProxy] = Proxy,
            [ConfigurationKeys.ProxyType] = ProxyType,
            [ConfigurationKeys.ResourceApi] = ResourceApi,
        };
    }
}
