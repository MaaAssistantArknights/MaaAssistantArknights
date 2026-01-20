// <copyright file="VersionUpdateSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Properties;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 软件更新设置
/// </summary>
public class VersionUpdateSettingsUserControlModel : PropertyChangedBase
{
    static VersionUpdateSettingsUserControlModel()
    {
        Instance = new();
    }

    public static VersionUpdateSettingsUserControlModel Instance { get; }

    public enum UpdateVersionType
    {
        /// <summary>
        /// 测试版
        /// </summary>
        Nightly,

        /// <summary>
        /// 开发版
        /// </summary>
        Beta,

        /// <summary>
        /// 稳定版
        /// </summary>
        Stable,
    }

    /// <summary>
    /// Gets the core version.
    /// </summary>
    public static string CoreVersion { get; } = Marshal.PtrToStringAnsi(MaaService.AsstGetVersion()) ?? "0.0.1";

    public static string CoreVersionDisplay => string.Join("\u200B", CoreVersion.ToCharArray());

    private static readonly string _uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";

    /// <summary>
    /// Gets the UI version.
    /// </summary>
    public static string UiVersion { get; } = _uiVersion == "0.0.1" ? "DEBUG_VERSION" : _uiVersion;

    public static string UiVersionDisplay => string.Join("\u200B", UiVersion.ToCharArray());

    public static DateTime BuildDateTime { get; } = Assembly.GetExecutingAssembly().GetCustomAttribute<BuildDateTimeAttribute>()?.BuildDateTime ?? DateTime.MinValue;

    public static string BuildDateTimeCurrentCultureString => BuildDateTime.ToLocalTimeString();

    private static (DateTime DateTime, string VersionName) _resourceInfo = GetResourceVersionByClientType(SettingsViewModel.GameSettings.ClientType);

    public (DateTime DateTime, string VersionName) ResourceInfo
    {
        get => _resourceInfo;
        set => SetAndNotify(ref _resourceInfo, value);
    }

    private static string _resourceVersion = _resourceInfo.VersionName;

    private string _newResourceFoundInfo = string.Empty;

    public string NewResourceFoundInfo
    {
        get => _newResourceFoundInfo;
        set {
            SetAndNotify(ref _newResourceFoundInfo, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
        }
    }

    private string _newVersionFoundInfo = string.Empty;

    public string NewVersionFoundInfo
    {
        get => _newVersionFoundInfo;
        set {
            SetAndNotify(ref _newVersionFoundInfo, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
        }
    }

    /// <summary>
    /// Gets or sets the resource version.
    /// </summary>
    public string ResourceVersion
    {
        get => _resourceVersion;
        set => SetAndNotify(ref _resourceVersion, value);
    }

    private static DateTime _resourceDateTime = _resourceInfo.DateTime;

    public DateTime ResourceDateTime
    {
        get => _resourceDateTime;
        set => SetAndNotify(ref _resourceDateTime, value);
    }

    private string _resourceDateTimeCurrentCultureString = _resourceDateTime.ToLocalTimeString();

    public string ResourceDateTimeCurrentCultureString
    {
        get => _resourceDateTimeCurrentCultureString;
        set => SetAndNotify(ref _resourceDateTimeCurrentCultureString, value);
    }

    public void ResourceInfoUpdate()
    {
        ResourceInfo = GetResourceVersionByClientType(SettingsViewModel.GameSettings.ClientType);
        ResourceVersion = ResourceInfo.VersionName;
        ResourceDateTime = ResourceInfo.DateTime;
        ResourceDateTimeCurrentCultureString = ResourceDateTime.ToLocalTimeString();
        Instances.SettingsViewModel.UpdateWindowTitle();
    }

    public static (DateTime DateTime, string VersionName) GetResourceVersionByClientType(string clientType)
    {
        bool isDefaultClient = new HashSet<string> { string.Empty, "Official", "Bilibili" }.Contains(clientType);

        string defaultJsonPath = Path.Combine(PathsHelper.ResourceDir, "version.json");
        var jsonPath = isDefaultClient
            ? defaultJsonPath
            : Path.Combine(PathsHelper.ResourceDir, $"global/{clientType}/resource/version.json");

        string versionName;
        if (!File.Exists(defaultJsonPath) || (!isDefaultClient && !File.Exists(jsonPath)))
        {
            return (DateTime.MinValue, string.Empty);
        }

        var versionJson = LoadJson(jsonPath);
        var currentTime = (ulong)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var poolTime = (ulong?)versionJson?["gacha"]?["time"]; // 卡池的开始时间
        var activityTime = (ulong?)versionJson?["activity"]?["time"]; // 活动的开始时间
        var lastUpdated = isDefaultClient
            ? (string?)versionJson?["last_updated"]
            : (string?)LoadJson(defaultJsonPath)?["last_updated"];

        var dateTime = lastUpdated == null
            ? DateTime.MinValue
            : DateTime.ParseExact(lastUpdated, "yyyy-MM-dd HH:mm:ss.fff", null);

        if (currentTime < poolTime && currentTime < activityTime)
        {
            versionName = string.Empty;
        }
        else if (currentTime >= poolTime && currentTime < activityTime)
        {
            versionName = versionJson?["gacha"]?["pool"]?.ToString() ?? string.Empty;
        }
        else if (currentTime < poolTime && currentTime >= activityTime)
        {
            versionName = versionJson?["activity"]?["name"]?.ToString() ?? string.Empty;
        }
        else if (poolTime > activityTime)
        {
            versionName = versionJson?["gacha"]?["pool"]?.ToString() ?? string.Empty;
        }
        else
        {
            versionName = versionJson?["activity"]?["name"]?.ToString() ?? string.Empty;
        }

        return (dateTime, versionName);

        static JObject? LoadJson(string path)
        {
            return JsonConvert.DeserializeObject<JObject>(File.ReadAllText(path));
        }
    }

    private UpdateVersionType _versionType = (UpdateVersionType)Enum.Parse(
        typeof(UpdateVersionType),
        ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionType, nameof(UpdateVersionType.Stable)));

    /// <summary>
    /// Gets or sets the type of version to update.
    /// </summary>
    public UpdateVersionType VersionType
    {
        get => _versionType;
        set {
            SetAndNotify(ref _versionType, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.VersionType, value.ToString());
        }
    }

    /// <summary>
    /// Gets the list of the version type.
    /// </summary>
    public List<GenericCombinedData<UpdateVersionType>> AllVersionTypeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("UpdateCheckNightly"), Value = UpdateVersionType.Nightly },
            new() { Display = LocalizationHelper.GetString("UpdateCheckBeta"), Value = UpdateVersionType.Beta },
            new() { Display = LocalizationHelper.GetString("UpdateCheckStable"), Value = UpdateVersionType.Stable },
        ];

    public List<GenericCombinedData<UpdateVersionType>> VersionTypeList
    {
        get => AllVersionTypeList.Where(v => AllowNightlyUpdates || v.Value != UpdateVersionType.Nightly).ToList();
    }

    public bool AllowNightlyUpdates { get; set; } = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AllowNightlyUpdates, bool.FalseString));

    private bool _hasAcknowledgedNightlyWarning = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.HasAcknowledgedNightlyWarning, bool.FalseString));

    public bool HasAcknowledgedNightlyWarning
    {
        get => _hasAcknowledgedNightlyWarning;
        set {
            SetAndNotify(ref _hasAcknowledgedNightlyWarning, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.HasAcknowledgedNightlyWarning, value.ToString());
        }
    }

    public List<GenericCombinedData<string>> UpdateSourceList { get; } = [
        new() { Display = LocalizationHelper.GetString("GlobalSource"), Value = "Github" },
        new() { Display = LocalizationHelper.GetString("MirrorChyan"), Value = "MirrorChyan" },
    ];

    private string _updateSource = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateSource, "Github");

    /// <summary>
    /// Gets or sets the type of version to update.
    /// </summary>
    public string UpdateSource
    {
        get => _updateSource;
        set {
            SetAndNotify(ref _updateSource, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.UpdateSource, value);
        }
    }

    private bool _forceGithubGlobalSource = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ForceGithubGlobalSource, bool.FalseString));

    public bool ForceGithubGlobalSource
    {
        get => _forceGithubGlobalSource;
        set {
            SetAndNotify(ref _forceGithubGlobalSource, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ForceGithubGlobalSource, value.ToString());
        }
    }

    private string _mirrorChyanCdk = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MirrorChyanCdk, string.Empty));

    public string MirrorChyanCdk
    {
        get => _mirrorChyanCdk;
        set {
            if (string.IsNullOrEmpty(value))
            {
                MirrorChyanCdkExpiredTime = 0;
            }

            if (!SetAndNotify(ref _mirrorChyanCdk, value))
            {
                return;
            }

            if (value.Length == 24)
            {
                Task.Run(async () => {
                    await Instances.VersionUpdateDialogViewModel.VersionUpdateAndAskToRestartAsync();
                    await ResourceUpdater.ResourceUpdateAndReloadAsync();
                });
            }

            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MirrorChyanCdk, value);
        }
    }

    // 时间戳
    private long _mirrorChyanCdkExpiredTime = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MirrorChyanCdkExpiredTime, 0L);

    // 0 表示未设置，1 表示未设置且已过期
    public long MirrorChyanCdkExpiredTime
    {
        get => _mirrorChyanCdkExpiredTime;
        set {
            if (!SetAndNotify(ref _mirrorChyanCdkExpiredTime, value))
            {
                return;
            }

            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.MirrorChyanCdkExpiredTime, value.ToString());
            RefreshMirrorChyanCdkRemaining();
        }
    }

    private bool _mirrorChyanCdkFetchFailed = false;

    public bool MirrorChyanCdkFetchFailed
    {
        get => _mirrorChyanCdkFetchFailed;
        set => SetAndNotify(ref _mirrorChyanCdkFetchFailed, value);
    }

    public DateTime MirrorChyanCdkExpiredDateTime => DateTimeOffset.FromUnixTimeSeconds(MirrorChyanCdkExpiredTime).DateTime;

    public DateTime MirrorChyanCdkExpiredLocalTime => MirrorChyanCdkExpiredDateTime.ToLocalTime();

    /// <summary>
    /// Gets 剩余时间
    /// </summary>
    public TimeSpan MirrorChyanCdkRemaining => MirrorChyanCdkExpiredDateTime - DateTime.Now;

    /// <summary>
    /// Gets a value indicating whether 是否已过期
    /// </summary>
    public bool IsMirrorChyanCdkExpired => MirrorChyanCdkRemaining.TotalSeconds <= 0;

    /// <summary>
    /// Gets 显示用的剩余时间提示
    /// </summary>
    public string MirrorChyanCdkRemainingText =>
        MirrorChyanCdkExpiredTime != 0
        ? IsMirrorChyanCdkExpired
            ? LocalizationHelper.GetString("MirrorChyanCdkExpired")
            : string.Format(LocalizationHelper.GetString("MirrorChyanCdkRemainingDays"),
                            MirrorChyanCdkRemaining.TotalDays.ToString("F1"))
        : string.Empty;

    /// <summary>
    /// Gets uI 显示用颜色
    /// </summary>
    public string MirrorChyanCdkRemainingBrush
    {
        get {
            if (IsMirrorChyanCdkExpired)
            {
                return UiLogColor.Error;
            }

            if (MirrorChyanCdkRemaining.TotalDays <= 7)
            {
                return UiLogColor.Warning;
            }

            return UiLogColor.Success;
        }
    }

    public void RefreshMirrorChyanCdkRemaining()
    {
        OnPropertyChanged(nameof(MirrorChyanCdkExpiredDateTime));
        OnPropertyChanged(nameof(MirrorChyanCdkRemaining));
        OnPropertyChanged(nameof(IsMirrorChyanCdkExpired));
        OnPropertyChanged(nameof(MirrorChyanCdkRemainingText));
        OnPropertyChanged(nameof(MirrorChyanCdkRemainingBrush));
        OnPropertyChanged(nameof(MirrorChyanCdkExpiredLocalTime));
    }

    private bool _startupUpdateCheck = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.StartupUpdateCheck, bool.TrueString));

    // UI 绑定的方法
    [UsedImplicitly]
    public void MirrorChyanCdkCopy()
    {
        System.Windows.Forms.Clipboard.Clear();
        System.Windows.Forms.Clipboard.SetDataObject(MirrorChyanCdk);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to check update.
    /// </summary>
    public bool StartupUpdateCheck
    {
        get => _startupUpdateCheck;
        set {
            SetAndNotify(ref _startupUpdateCheck, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.StartupUpdateCheck, value.ToString());
        }
    }

    private bool _updateAutoCheck = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateAutoCheck, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to check update.
    /// </summary>
    public bool UpdateAutoCheck
    {
        get => _updateAutoCheck;
        set {
            SetAndNotify(ref _updateAutoCheck, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.UpdateAutoCheck, value.ToString());
        }
    }

    private string _proxy = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateProxy, string.Empty);

    /// <summary>
    /// Gets or sets the proxy settings.
    /// </summary>
    public string Proxy
    {
        get => _proxy;
        set {
            SetAndNotify(ref _proxy, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.UpdateProxy, value);
        }
    }

    public List<CombinedData> ProxyTypeList { get; } =
        [
            new() { Display = "HTTP Proxy", Value = "http" },
            new() { Display = "SOCKS5 Proxy", Value = "socks5" },
        ];

    private string _proxyType = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ProxyType, "http");

    public string ProxyType
    {
        get => _proxyType;
        set {
            SetAndNotify(ref _proxyType, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.ProxyType, value);
        }
    }

    private string _githubMainDomain = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.GithubMainDomain, "github.com");

    /// <summary>
    /// Gets or sets the GitHub main domain replacement.
    /// </summary>
    public string GithubMainDomain
    {
        get => _githubMainDomain;
        set {
            SetAndNotify(ref _githubMainDomain, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.GithubMainDomain, value);
        }
    }

    private string _githubRawDomain = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.GithubRawDomain, "raw.githubusercontent.com");

    /// <summary>
    /// Gets or sets the GitHub raw domain replacement.
    /// </summary>
    public string GithubRawDomain
    {
        get => _githubRawDomain;
        set {
            SetAndNotify(ref _githubRawDomain, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.GithubRawDomain, value);
        }
    }

    private string _githubApiDomain = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.GithubApiDomain, "api.github.com");

    /// <summary>
    /// Gets or sets the GitHub API domain replacement.
    /// </summary>
    public string GithubApiDomain
    {
        get => _githubApiDomain;
        set {
            SetAndNotify(ref _githubApiDomain, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.GithubApiDomain, value);
        }
    }

    private string _githubToken = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.GithubToken, string.Empty));

    /// <summary>
    /// Gets or sets the GitHub personal access token.
    /// </summary>
    public string GithubToken
    {
        get => _githubToken;
        set {
            if (!SetAndNotify(ref _githubToken, value))
            {
                return;
            }

            value = SimpleEncryptionHelper.Encrypt(value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.GithubToken, value);
        }
    }

    private bool _isCheckingForUpdates;

    /// <summary>
    /// Gets or sets a value indicating whether the update is being checked.
    /// </summary>
    public bool IsCheckingForUpdates
    {
        get => _isCheckingForUpdates;
        set {
            SetAndNotify(ref _isCheckingForUpdates, value);
        }
    }

    private bool _autoDownloadUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AutoDownloadUpdatePackage, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to auto download update package.
    /// </summary>
    public bool AutoDownloadUpdatePackage
    {
        get => _autoDownloadUpdatePackage;
        set {
            SetAndNotify(ref _autoDownloadUpdatePackage, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.AutoDownloadUpdatePackage, value.ToString());
        }
    }

    private bool _autoInstallUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AutoInstallUpdatePackage, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to auto install update package.
    /// </summary>
    public bool AutoInstallUpdatePackage
    {
        get => _autoInstallUpdatePackage;
        set {
            SetAndNotify(ref _autoInstallUpdatePackage, value);
            ConfigurationHelper.SetGlobalValue(ConfigurationKeys.AutoInstallUpdatePackage, value.ToString());
        }
    }

    /// <summary>
    /// Updates manually.
    /// </summary>
    /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
    [UsedImplicitly]
    public async Task ManualUpdate()
    {
        if (IsCheckingForUpdates)
        {
            return;
        }

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource == "MirrorChyan" && string.IsNullOrEmpty(SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanSelectedButNoCdk"));
            return;
        }

        var ret = await Instances.VersionUpdateDialogViewModel.CheckAndDownloadVersionUpdate();

        var toastMessage = ret switch {
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoNeedToUpdate => string.Empty,
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoNeedToUpdateDebugVersion => LocalizationHelper.GetString("NoNeedToUpdateDebugVersion"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest => LocalizationHelper.GetString("AlreadyLatest"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.UnknownError => LocalizationHelper.GetString("NewVersionDetectFailedTitle"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.NetworkError => LocalizationHelper.GetString("CheckNetworking"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.FailedToGetInfo => LocalizationHelper.GetString("GetReleaseNoteFailed"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.OK => string.Empty,
            VersionUpdateDialogViewModel.CheckUpdateRetT.NewVersionIsBeingBuilt => LocalizationHelper.GetString("NewVersionIsBeingBuilt"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.OnlyGameResourceUpdated => LocalizationHelper.GetString("GameResourceUpdated"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoMirrorChyanCdk => LocalizationHelper.GetString("MirrorChyanSelectedButNoCdk"),
            _ => string.Empty,
        };

        if (toastMessage != string.Empty)
        {
            ToastNotification.ShowDirect(toastMessage);
        }

        if (ret == VersionUpdateDialogViewModel.CheckUpdateRetT.OK)
        {
            _ = Instances.VersionUpdateDialogViewModel.AskToRestart();
        }
    }

    [UsedImplicitly]
    public async Task ManualUpdateResource()
    {
        if (IsCheckingForUpdates)
        {
            return;
        }

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource == "MirrorChyan" && string.IsNullOrEmpty(SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanSelectedButNoCdk"));
            return;
        }

        IsCheckingForUpdates = true;

        var (ret, uri, releaseNote) = await ResourceUpdater.CheckFromMirrorChyanAsync();
        var toastMessage = ret switch {
            VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest => LocalizationHelper.GetString("AlreadyLatest"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.UnknownError => LocalizationHelper.GetString("NewVersionDetectFailedTitle"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.NetworkError => LocalizationHelper.GetString("CheckNetworking"),
            _ => string.Empty,
        };

        if (toastMessage != string.Empty)
        {
            ToastNotification.ShowDirect(toastMessage);
        }

        if (ret == VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest)
        {
            IsCheckingForUpdates = false;
            return;
        }

        bool success = UpdateSource switch {
            "Github" => await ResourceUpdater.UpdateFromGithubAsync(),
            "MirrorChyan" => (ret == VersionUpdateDialogViewModel.CheckUpdateRetT.OK) && await ResourceUpdater.DownloadFromMirrorChyanAsync(uri, releaseNote),
            _ => await ResourceUpdater.UpdateFromGithubAsync(),
        };

        if (success)
        {
            _ = ResourceUpdater.ResourceReloadWhenIdleAsync();
        }
        else
        {
            AchievementTrackerHelper.Instance.AddProgress(AchievementIds.CdnTorture);
        }

        IsCheckingForUpdates = false;
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void ShowChangelog()
    {
        if (Instances.VersionUpdateDialogViewModel.View is System.Windows.Window window)
        {
            if (window.WindowState == System.Windows.WindowState.Minimized)
            {
                window.WindowState = System.Windows.WindowState.Normal;
            }

            window.Activate();
        }
        else
        {
            Instances.WindowManager.ShowWindow(Instances.VersionUpdateDialogViewModel);
        }
    }
}
