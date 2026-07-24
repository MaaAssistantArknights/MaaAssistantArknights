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
using System.Windows;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
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
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
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
    private static readonly string _coreVersion = Marshal.PtrToStringAnsi(MaaService.AsstGetVersion()) ?? "0.0.1";

    public static string CoreVersion => FakeUpdateHelper.IsEnabled ? FakeUpdateHelper.CurrentVersion : _coreVersion;

    public static string CoreVersionDisplay => string.Join("\u200B", CoreVersion.ToCharArray());

    private static readonly string _uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";

    /// <summary>
    /// Gets the UI version.
    /// </summary>
    public static string UiVersion => FakeUpdateHelper.IsEnabled
        ? FakeUpdateHelper.CurrentVersion
        : _uiVersion == "0.0.1"
            ? "DEBUG_VERSION"
            : _uiVersion;

    public static string UiVersionDisplay => string.Join("\u200B", UiVersion.ToCharArray());

    public static DateTimeOffset BuildDateTime { get; } = Assembly.GetExecutingAssembly().GetCustomAttribute<BuildDateTimeAttribute>()?.BuildTime ?? DateTimeOffset.MinValue;

    public static string BuildDateTimeCurrentCultureString => BuildDateTime.ToLocalTimeString();

    private static (DateTimeOffset DateTime, string VersionName) _resourceInfo = GetResourceVersionByClientType(SettingsViewModel.GameSettings.ClientType);

    public (DateTimeOffset DateTime, string VersionName) ResourceInfo
    {
        get => _resourceInfo;
        set => SetAndNotify(ref _resourceInfo, value);
    }

    private static string _resourceVersion = _resourceInfo.VersionName;

    public string NewResourceFoundInfo
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
        }
    } = string.Empty;

    public string NewVersionFoundInfo
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            Instances.SettingsViewModel.UpdateWindowTitle();
        }
    } = string.Empty;

    /// <summary>
    /// Gets or sets the resource version.
    /// </summary>
    public string ResourceVersion
    {
        get => _resourceVersion;
        set => SetAndNotify(ref _resourceVersion, value);
    }

    private static DateTimeOffset _resourceDateTime = _resourceInfo.DateTime;

    public DateTimeOffset ResourceDateTime
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

    public static (DateTimeOffset DateTime, string VersionName) GetResourceVersionByClientType(ClientType clientType)
    {
        bool isDefaultClient = new HashSet<ClientType> { ClientType.Official, ClientType.Bilibili }.Contains(clientType);

        string defaultJsonPath = Path.Combine(PathsHelper.ResourceDir, "version.json");
        var jsonPath = isDefaultClient
            ? defaultJsonPath
            : Path.Combine(PathsHelper.ResourceDir, $"global/{clientType.ToCustomString()}/resource/version.json");

        string versionName;
        if (!File.Exists(defaultJsonPath) || (!isDefaultClient && !File.Exists(jsonPath)))
        {
            return (DateTimeOffset.MinValue, string.Empty);
        }

        var versionJson = LoadJson(jsonPath);
        var currentTime = (ulong)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var poolTime = (ulong?)versionJson?["gacha"]?["time"]; // 卡池的开始时间
        var activityTime = (ulong?)versionJson?["activity"]?["time"]; // 活动的开始时间
        var lastUpdated = isDefaultClient
            ? (string?)versionJson?["last_updated"]
            : (string?)LoadJson(defaultJsonPath)?["last_updated"];

        var dateTime = lastUpdated == null
            ? DateTimeOffset.MinValue
            : DateTimeOffset.ParseExact(lastUpdated, "yyyy-MM-dd HH:mm:ss.fff", null, System.Globalization.DateTimeStyles.AssumeUniversal);

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

    /// <summary>
    /// Gets or sets the type of version to update.
    /// </summary>
    public UpdateVersionType VersionType
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.VersionType = value;
        }
    } = ConfigFactory.Root.Update.VersionType;

    /// <summary>
    /// Gets the list of the version type.
    /// </summary>
    public LocalizedObservableList<UpdateVersionType> AllVersionTypeList { get; } = new(
        (UpdateVersionType.Nightly, "UpdateCheckNightly"),
        (UpdateVersionType.Beta, "UpdateCheckBeta"),
        (UpdateVersionType.Stable, "UpdateCheckStable"));

    public List<GenericCombinedData<UpdateVersionType>> VersionTypeList
    {
        get => [.. AllVersionTypeList.Items.Where(v => AllowNightlyUpdates || v.Value != UpdateVersionType.Nightly)];
    }

    public bool AllowNightlyUpdates { get; set; } = ConfigFactory.Root.Update.AllowNightlyUpdates;

    public bool HasAcknowledgedNightlyWarning
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.HasAcknowledgedNightlyWarning = value;
        }
    } = ConfigFactory.Root.Update.HasAcknowledgedNightlyWarning;

    public LocalizedObservableList<string> UpdateSourceList { get; } = new(
        ("Github", "GlobalSource"),
        ("MirrorChyan", "MirrorChyan"));

    /// <summary>
    /// Gets or sets the type of version to update.
    /// </summary>
    public string UpdateSource
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.UpdateSource = value;
        }
    } = ConfigFactory.Root.Update.UpdateSource;

    public bool ForceGithubGlobalSource
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.ForceGithubGlobalSource = value;
        }
    } = ConfigFactory.Root.Update.ForceGithubGlobalSource;

    public string MirrorChyanCdk
    {
        get; set {
            if (string.IsNullOrEmpty(value))
            {
                MirrorChyanCdkExpiredTime = 0;
            }

            if (!SetAndNotify(ref field, value))
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

            ConfigFactory.Root.Update.MirrorChyanCdk = SimpleEncryptionHelper.Encrypt(value);
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.Root.Update.MirrorChyanCdk);

    // 0 表示未设置，1 表示未设置且已过期
    public long MirrorChyanCdkExpiredTime
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            ConfigFactory.Root.Update.MirrorChyanCdkExpiredTime = value;
            RefreshMirrorChyanCdkRemaining();
        }
    } = ConfigFactory.Root.Update.MirrorChyanCdkExpiredTime;

    public bool MirrorChyanCdkFetchFailed { get; set => SetAndNotify(ref field, value); }

    public DateTimeOffset MirrorChyanCdkExpiredDateTime => DateTimeOffset.FromUnixTimeSeconds(MirrorChyanCdkExpiredTime);

    public DateTime MirrorChyanCdkExpiredLocalTime => MirrorChyanCdkExpiredDateTime.LocalDateTime;

    /// <summary>
    /// Gets 剩余时间
    /// </summary>
    public TimeSpan MirrorChyanCdkRemaining => MirrorChyanCdkExpiredDateTime - DateTimeOffset.Now;

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
            : LocalizationHelper.GetStringFormat("MirrorChyanCdkRemainingDays",
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

    // UI 绑定的方法
    [UsedImplicitly]
    public void MirrorChyanCdkCopy()
    {
        Clipboard.Clear();
        Clipboard.SetDataObject(MirrorChyanCdk);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to check update.
    /// </summary>
    public bool StartupUpdateCheck
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.CheckOnStartup = value;
        }
    } = ConfigFactory.Root.Update.CheckOnStartup;

    /// <summary>
    /// Gets or sets a value indicating whether to check update.
    /// </summary>
    public bool UpdateAutoCheck
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.CheckOnSchedule = value;
        }
    } = ConfigFactory.Root.Update.CheckOnSchedule;

    /// <summary>
    /// Gets or sets the proxy settings.
    /// </summary>
    public string Proxy
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.Proxy = value;
        }
    } = ConfigFactory.Root.Update.Proxy;

    public List<CombinedData> ProxyTypeList { get; } =
        [
            new() { Display = "HTTP Proxy", Value = "Http" },
            new() { Display = "SOCKS5 Proxy", Value = "Socks5" },
        ];

    public string ProxyType
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.ProxyType = value;
        }
    } = ConfigFactory.Root.Update.ProxyType;

    /// <summary>
    /// Gets or sets a value indicating whether the update is being checked.
    /// </summary>
    public bool IsCheckingForUpdates { get; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether to auto download update package.
    /// </summary>
    public bool AutoDownloadUpdatePackage
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.AutoDownloadUpdatePackage = value;
        }
    } = ConfigFactory.Root.Update.AutoDownloadUpdatePackage;

    /// <summary>
    /// Gets or sets a value indicating whether to auto install update package.
    /// </summary>
    public bool AutoInstallUpdatePackage
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.AutoInstallUpdatePackage = value;
        }
    } = ConfigFactory.Root.Update.AutoInstallUpdatePackage;

    /// <summary>
    /// Gets or sets a value indicating whether to show the updater console window.
    /// </summary>
    public bool ShowUpdaterConsole
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.ShowUpdaterConsole = value;
        }
    } = ConfigFactory.Root.Update.ShowUpdaterConsole;

    /// <summary>
    /// Gets or sets a value indicating whether to show the updater progress window.
    /// </summary>
    public bool ShowUpdaterProgress
    {
        get; set {
            // 关闭进度窗口属于有风险的操作，需要二次确认
            if (!value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("ShowUpdaterProgressWarning"),
                    LocalizationHelper.GetString("Warning"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning,
                    yes: LocalizationHelper.GetString("Confirm"),
                    no: LocalizationHelper.GetString("Cancel"));
                if (result != MessageBoxResult.Yes)
                {
                    return;
                }
            }

            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.ShowUpdaterProgress = value;
        }
    } = ConfigFactory.Root.Update.ShowUpdaterProgress;

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

        if (ret == VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.LatestVersionInspector);
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
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.ChangelogReader);
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

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        AllVersionTypeList.RefreshLocalization();
        UpdateSourceList.RefreshLocalization();
    }
}
