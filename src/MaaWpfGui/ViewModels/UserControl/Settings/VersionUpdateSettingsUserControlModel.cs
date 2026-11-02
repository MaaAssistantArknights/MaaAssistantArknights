// <copyright file="VersionUpdateSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Threading;
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
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Models.UpdateCredentialModel;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 软件更新设置
/// </summary>
public class VersionUpdateSettingsUserControlModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<VersionUpdateSettingsUserControlModel>();

    static VersionUpdateSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public VersionUpdateSettingsUserControlModel()
    {
        GitHubCredential = new(
            SimpleEncryptionHelper.Decrypt(ConfigFactory.Root.Update.GitHubToken),
            ConfigFactory.Root.Update.GitHubTokenExpiresAt,
            UpdateCredentialStatus.None,
            new("Settings.Update.GitHub.Token.Checking", "Settings.Update.GitHub.Token.Invalid",
                "Settings.Update.GitHub.Token.Expired", "Settings.Update.GitHub.Token.Failed",
                "Settings.Update.GitHub.Token.RemainingDays", "Settings.Update.GitHub.Token.NeverExpires"),
            () => UpdateSource == UpdateSource.GitHub,
            value => value.Length == (value.StartsWith("github_pat_", StringComparison.Ordinal) ? 93 : 40),
            value => ConfigFactory.Root.Update.GitHubToken = SimpleEncryptionHelper.Encrypt(value),
            (expiresAt, _) => ConfigFactory.Root.Update.GitHubTokenExpiresAt = expiresAt,
            revision => QueueCredentialUpdate(UpdateSource.GitHub, revision));

        long mirrorExpiration = ConfigFactory.Root.Update.MirrorChyanCdkExpiredTime;
        MirrorChyanCredential = new(
            SimpleEncryptionHelper.Decrypt(ConfigFactory.Root.Update.MirrorChyanCdk),
            MirrorChyanUpdateHelper.ParseExpiration(mirrorExpiration.ToString()),
            mirrorExpiration == 1 ? UpdateCredentialStatus.Expired : UpdateCredentialStatus.None,
            new("Settings.Update.MirrorChyan.Checking", "VersionUpdate.MirrorChyan.CdkInvalid",
                "VersionUpdate.MirrorChyan.CdkExpired", "Settings.Update.MirrorChyan.ValidationFailed",
                "Settings.Update.MirrorChyan.CdkRemainingDays", UnknownExpiration: "Settings.Update.MirrorChyan.ExpirationUnknown"),
            () => UpdateSource == UpdateSource.MirrorChyan,
            value => value.Length == 24,
            value => ConfigFactory.Root.Update.MirrorChyanCdk = SimpleEncryptionHelper.Encrypt(value),
            (expiresAt, status) => ConfigFactory.Root.Update.MirrorChyanCdkExpiredTime =
                expiresAt?.ToUnixTimeSeconds() ?? (status == UpdateCredentialStatus.Expired ? 1 : 0),
            revision => QueueCredentialUpdate(UpdateSource.MirrorChyan, revision));

        GitHubCredential.PropertyChanged += (_, args) => {
            if (args.PropertyName == nameof(UpdateCredentialModel.Value) && UpdateSource == UpdateSource.GitHub)
            {
                CancelPendingCredentialUpdate();
            }
        };
        MirrorChyanCredential.PropertyChanged += (_, args) => {
            if (args.PropertyName == nameof(UpdateCredentialModel.Value) && UpdateSource == UpdateSource.MirrorChyan)
            {
                CancelPendingCredentialUpdate();
            }
        };

        PropertyDependsOnUtility.InitializePropertyDependencies(this);
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
    /// Gets the core version. UI 预览模式下 Core 未加载，不触碰 native，降级为占位值。
    /// </summary>
    private static readonly string _coreVersion = Main.Bootstrapper.IsCoreInitSkipped
        ? "N/A"
        : Marshal.PtrToStringAnsi(MaaService.AsstGetVersion()) ?? "0.0.1";

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

    private static (DateTimeOffset DateTime, string VersionName) _resourceInfo = GetResourceVersionByClientType(ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ClientType);

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
        (UpdateVersionType.Nightly, "Settings.Update.Channel.Nightly"),
        (UpdateVersionType.Beta, "Settings.Update.Channel.Beta"),
        (UpdateVersionType.Stable, "Settings.Update.Channel.Stable"));

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

    public LocalizedObservableList<UpdateSource> UpdateSourceList { get; } = new(
        (UpdateSource.GitHub, "Settings.Update.Source.Global"),
        (UpdateSource.MirrorChyan, "Settings.Update.Source.MirrorChyan"));

    /// <summary>
    /// Gets or sets the type of version to update.
    /// </summary>
    public UpdateSource UpdateSource
    {
        get; set {
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            ConfigFactory.Root.Update.UpdateSource = value;
            CancelPendingCredentialUpdate();
            GitHubCredential.CancelCheck();
            MirrorChyanCredential.CancelCheck();
        }
    } = ConfigFactory.Root.Update.UpdateSource;

    private readonly SemaphoreSlim _gitHubTokenValidationLock = new(1, 1);
    private (UpdateSource Source, int Revision)? _pendingCredentialUpdate;
    private bool _credentialUpdateRunning;

    public UpdateCredentialModel GitHubCredential { get; }

    public UpdateCredentialModel MirrorChyanCredential { get; }

    [PropertyDependsOn(nameof(UpdateSource))]
    public UpdateCredentialModel CurrentUpdateCredential => UpdateSource == UpdateSource.GitHub ? GitHubCredential : MirrorChyanCredential;

    public string GitHubToken => GitHubCredential.Value;

    public string MirrorChyanCdk => MirrorChyanCredential.Value;

    public void RefreshUpdateCredentials()
    {
        GitHubCredential.RefreshDisplay();
        MirrorChyanCredential.RefreshDisplay();
    }

    private void CancelPendingCredentialUpdate()
    {
        _pendingCredentialUpdate = null;
    }

    private void QueueCredentialUpdate(UpdateSource source, int revision)
    {
        var credential = source == UpdateSource.GitHub ? GitHubCredential : MirrorChyanCredential;
        if (source != UpdateSource || revision != credential.Revision || string.IsNullOrEmpty(credential.Value))
        {
            return;
        }

        _pendingCredentialUpdate = (source, revision);
        TryStartPendingCredentialUpdate();
    }

    private void TryStartPendingCredentialUpdate()
    {
        if (IsCheckingForUpdates || _credentialUpdateRunning || _pendingCredentialUpdate is not { } pending)
        {
            return;
        }

        CancelPendingCredentialUpdate();
        var credential = pending.Source == UpdateSource.GitHub ? GitHubCredential : MirrorChyanCredential;
        if (pending.Source != UpdateSource || pending.Revision != credential.Revision || string.IsNullOrEmpty(credential.Value))
        {
            return;
        }

        _credentialUpdateRunning = true;
        _ = CheckUpdatesAfterCredentialChangedAsync(pending.Source, pending.Revision);
    }

    private async Task CheckUpdatesAfterCredentialChangedAsync(UpdateSource source, int revision)
    {
        var credential = source == UpdateSource.GitHub ? GitHubCredential : MirrorChyanCredential;
        try
        {
            await Instances.VersionUpdateDialogViewModel.VersionUpdateAndAskToRestartAsync();
            if (source == UpdateSource && revision == credential.Revision)
            {
                await ResourceUpdater.ResourceUpdateAndReloadAsync();
            }
        }
        catch (OperationCanceledException)
        {
            // 输入或更新源已改变，忽略旧请求。
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to check updates after credential change");
        }
        finally
        {
            await Execute.OnUIThreadAsync(() => {
                _credentialUpdateRunning = false;
                TryStartPendingCredentialUpdate();
            });
        }
    }

    public async Task<UpdateCredentialRequest?> GetValidatedGitHubRequestAsync()
    {
        await _gitHubTokenValidationLock.WaitAsync().ConfigureAwait(false);
        try
        {
            var request = await GitHubCredential.BeginCheckAsync().ConfigureAwait(false);
            if (request is not { } check)
            {
                return null;
            }

            var result = await GitHubUpdateHelper.ValidateTokenAsync(check.Value, check.CancellationToken).ConfigureAwait(false);
            var status = result.Result switch {
                GitHubUpdateHelper.GitHubTokenValidationResult.Valid => UpdateCredentialStatus.Valid,
                GitHubUpdateHelper.GitHubTokenValidationResult.Invalid => UpdateCredentialStatus.Invalid,
                _ => UpdateCredentialStatus.Failed,
            };
            bool applied = await GitHubCredential.ApplyAsync(check, new(status, result.ExpiresAt)).ConfigureAwait(false);
            return applied && GitHubCredential.IsCurrent(check) && GitHubCredential.Status == UpdateCredentialStatus.Valid && !GitHubCredential.IsExpired
                ? check
                : null;
        }
        catch (OperationCanceledException)
        {
            return null;
        }
        finally
        {
            _gitHubTokenValidationLock.Release();
        }
    }

    public Task InvalidateGitHubTokenAsync(string token, CancellationToken cancellationToken = default) => GitHubCredential.InvalidateAsync(token, cancellationToken);

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
            if (field == value)
            {
                return;
            }

            ConfigFactory.Root.Update.Proxy = value;
            SetAndNotify(ref field, value);
            CancelCredentialChecksAfterProxyChanged();
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
            if (field == value)
            {
                return;
            }

            ConfigFactory.Root.Update.ProxyType = value;
            SetAndNotify(ref field, value);
            CancelCredentialChecksAfterProxyChanged();
        }
    } = ConfigFactory.Root.Update.ProxyType;

    private void CancelCredentialChecksAfterProxyChanged()
    {
        CancelPendingCredentialUpdate();
        GitHubCredential.CancelCheck();
        MirrorChyanCredential.CancelCheck();
    }

    /// <summary>
    /// Gets or sets a value indicating whether the update is being checked.
    /// </summary>
    public bool IsCheckingForUpdates
    {
        get; set {
            if (SetAndNotify(ref field, value) && !value)
            {
                Execute.OnUIThread(TryStartPendingCredentialUpdate);
            }
        }
    }

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
                    LocalizationHelper.GetString("Settings.Update.ShowUpdaterProgress.Warning"),
                    LocalizationHelper.GetString("Common.Warning"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning,
                    yes: LocalizationHelper.GetString("Common.Confirm"),
                    no: LocalizationHelper.GetString("Common.Cancel"));
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

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource == UpdateSource.MirrorChyan && string.IsNullOrEmpty(SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("Settings.Update.MirrorChyan.SelectedButNoCdk"));
            return;
        }

        var ret = await Instances.VersionUpdateDialogViewModel.CheckAndDownloadVersionUpdate();

        var toastMessage = ret switch {
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoNeedToUpdate => string.Empty,
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoNeedToUpdateDebugVersion => LocalizationHelper.GetString("Settings.Update.NoNeedToUpdateDebugVersion"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest => LocalizationHelper.GetString("Settings.Update.AlreadyLatest"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.UnknownError => LocalizationHelper.GetString("VersionUpdate.NewVersionDetectFailed.Title"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.NetworkError => LocalizationHelper.GetString("VersionUpdate.CheckNetworking"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.FailedToGetInfo => LocalizationHelper.GetString("VersionUpdate.GetReleaseNoteFailed"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.UpdatePackageDownloadFailed => LocalizationHelper.GetString("VersionUpdate.NewVersionDownloadFailed.Title"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.OK => string.Empty,
            VersionUpdateDialogViewModel.CheckUpdateRetT.NewVersionIsBeingBuilt => LocalizationHelper.GetString("VersionUpdate.NewVersionIsBeingBuilt"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.OnlyGameResourceUpdated => LocalizationHelper.GetString("VersionUpdate.GameResource.Updated"),
            VersionUpdateDialogViewModel.CheckUpdateRetT.NoMirrorChyanCdk => LocalizationHelper.GetString("Settings.Update.MirrorChyan.SelectedButNoCdk"),
            _ => string.Empty,
        };

        if (toastMessage != string.Empty && !CurrentUpdateCredential.HasError)
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

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource == UpdateSource.MirrorChyan && string.IsNullOrEmpty(SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("Settings.Update.MirrorChyan.SelectedButNoCdk"));
            return;
        }

        IsCheckingForUpdates = true;
        try
        {
            var credential = CurrentUpdateCredential;
            int revision = credential.Revision;
            var updateSource = UpdateSource;
            var request = await GetValidatedGitHubRequestAsync();
            if (updateSource != UpdateSource || revision != credential.Revision ||
                (updateSource == UpdateSource.GitHub && credential.HasError))
            {
                return;
            }

            request ??= updateSource == UpdateSource.MirrorChyan ? await credential.BeginCheckAsync() : null;
            var cancellationToken = request?.CancellationToken ?? default;
            var (ret, uri, releaseNote) = updateSource == UpdateSource.GitHub && request is { } check
                ? await ResourceUpdater.CheckFromGitHubAsync(check.Value, cancellationToken)
                : await ResourceUpdater.CheckFromMirrorChyanAsync(request);
            if (updateSource != UpdateSource || revision != credential.Revision)
            {
                return;
            }

            var toastMessage = ret switch {
                VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest => LocalizationHelper.GetString("Settings.Update.AlreadyLatest"),
                VersionUpdateDialogViewModel.CheckUpdateRetT.UnknownError => LocalizationHelper.GetString("VersionUpdate.NewVersionDetectFailed.Title"),
                VersionUpdateDialogViewModel.CheckUpdateRetT.NetworkError => LocalizationHelper.GetString("VersionUpdate.CheckNetworking"),
                _ => string.Empty,
            };

            if (toastMessage != string.Empty && !credential.HasError)
            {
                ToastNotification.ShowDirect(toastMessage);
            }

            if (ret is VersionUpdateDialogViewModel.CheckUpdateRetT.AlreadyLatest or
                VersionUpdateDialogViewModel.CheckUpdateRetT.CredentialError or VersionUpdateDialogViewModel.CheckUpdateRetT.NoNeedToUpdate ||
                (request != null && ret != VersionUpdateDialogViewModel.CheckUpdateRetT.OK))
            {
                return;
            }

            bool success = updateSource switch {
                UpdateSource.GitHub => await ResourceUpdater.UpdateFromGithubAsync(cancellationToken),
                UpdateSource.MirrorChyan => (ret == VersionUpdateDialogViewModel.CheckUpdateRetT.OK) && await ResourceUpdater.DownloadFromMirrorChyanAsync(uri, releaseNote, cancellationToken),
                _ => await ResourceUpdater.UpdateFromGithubAsync(cancellationToken),
            };

            if (success)
            {
                _ = ResourceUpdater.ResourceReloadWhenIdleAsync();
            }
            else
            {
                AchievementTrackerHelper.Instance.AddProgress(AchievementIds.CdnTorture);
            }
        }
        catch (OperationCanceledException)
        {
            VersionUpdateDialogViewModel.OutputDownloadProgress(string.Empty, downloading: false);
        }
        finally
        {
            IsCheckingForUpdates = false;
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void ShowChangelog()
    {
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.ChangelogReader);
        _ = Instances.VersionUpdateDialogViewModel.DownloadMissingContributorAvatarsAsync();
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

        RefreshUpdateCredentials();
    }
}
