// <copyright file="VersionUpdateDialogViewModel.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Semver;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// The view model of version update.
/// </summary>
public class VersionUpdateDialogViewModel : Screen
{
    private readonly RunningState _runningState;

    /// <summary>
    /// Initializes a new instance of the <see cref="VersionUpdateDialogViewModel"/> class.
    /// </summary>
    public VersionUpdateDialogViewModel()
    {
        _runningState = RunningState.Instance;
    }

    private static readonly ILogger _logger = Log.ForContext<VersionUpdateDialogViewModel>();

    private static string AddContributorLink(string text)
    {
        /*
        //        "@ " -> "@ "
        //       "`@`" -> "`@`"
        //   "@MistEO" -> "[@MistEO](https://github.com/MistEO)"
        // "[@MistEO]" -> "[@MistEO]"
        */
        return Regex.Replace(text, @"([^\[`]|^)@([^\s]+)", "$1[@$2](https://github.com/$2)");
    }

    private readonly string _curVersion = FakeUpdateHelper.IsEnabled
        ? FakeUpdateHelper.CurrentVersion
        : Marshal.PtrToStringAnsi(MaaService.AsstGetVersion()) ?? "0.0.1";

    private string _latestVersion = string.Empty;

    /// <summary>
    /// Gets or sets the update tag.
    /// </summary>
    public string UpdateTag
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.Name = value;
        }
    } = FakeUpdateHelper.IsEnabled ? FakeUpdateHelper.TargetVersion : ConfigFactory.Root.Update.Name;

    private static string LoadUpdateBody()
    {
        var body = MarkdownDataHelper.Get("CHANGELOG");
        if (!string.IsNullOrWhiteSpace(body))
        {
            return body;
        }

        return string.Empty;
    }

    private string _updateInfo = LoadUpdateBody();

    // private static readonly MarkdownPipeline s_markdownPipeline = new MarkdownPipelineBuilder().UseXamlSupportedExtensions().Build();

    /// <summary>
    /// Gets or sets the update info.
    /// </summary>
    public string UpdateInfo
    {
        get {
            try
            {
                return AddContributorLink(_updateInfo);
            }
            catch
            {
                return _updateInfo;
            }
        }

        set {
            SetAndNotify(ref _updateInfo, value);
            MarkdownDataHelper.Set("CHANGELOG", value);
        }
    }

    /// <summary>
    /// Gets or sets the update URL.
    /// </summary>
    public string UpdateUrl { get; set => SetAndNotify(ref field, value); } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether it is the first boot after updating.
    /// </summary>
    public bool IsFirstBootAfterUpdate
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.IsFirstBoot = value;
        }
    } = ConfigFactory.Root.Update.IsFirstBoot;

    /// <summary>
    /// Gets or sets the name of the update package.
    /// </summary>
    public string UpdatePackageName
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.UpdatePackage = value;
        }
    } = ConfigFactory.Root.Update.UpdatePackage;

    /// <summary>
    /// Gets the OS architecture.
    /// </summary>
    private static string OsArchitecture => RuntimeInformation.OSArchitecture.ToString().ToLower();

    /// <summary>
    /// Gets a value indicating whether the OS is arm.
    /// </summary>
    public static bool IsArm => OsArchitecture.StartsWith("arm");

    /*
    private const string RequestUrl = "repos/MaaAssistantArknights/MaaRelease/releases";
    private const string StableRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/latest";
    private const string MaaReleaseRequestUrlByTag = "repos/MaaAssistantArknights/MaaRelease/releases/tags/";
    private const string InfoRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/tags/";
    */

    private const string MaaUpdateApi = "version/summary.json";
    private const int UpdatePackageDownloadMaxAttempts = 3;

    private JObject? _latestJson;
    private JObject? _assetsObject;

    private string? _mirrorcDownloadUrl;
    private string? _mirrorcVersionName;
    private string? _mirrorcReleaseNote;
    private bool _requiresFullPackageConfirmation;

    public static bool HasPendingUpdatePackage()
    {
        return PendingUpdateApplier.HasPendingUpdatePackage();
    }

    public enum CheckUpdateRetT
    {
        /// <summary>
        /// 操作成功
        /// </summary>
        // ReSharper disable once InconsistentNaming
        OK,

        /// <summary>
        /// 未知错误
        /// </summary>
        UnknownError,

        /// <summary>
        /// 无需更新
        /// </summary>
        NoNeedToUpdate,

        /// <summary>
        /// 调试版本无需更新
        /// </summary>
        NoNeedToUpdateDebugVersion,

        /// <summary>
        /// 已经是最新版
        /// </summary>
        AlreadyLatest,

        /// <summary>
        /// 网络错误
        /// </summary>
        NetworkError,

        /// <summary>
        /// 获取信息失败
        /// </summary>
        FailedToGetInfo,

        /// <summary>
        /// 新版正在构建中
        /// </summary>
        NewVersionIsBeingBuilt,

        /// <summary>
        /// 只更新了游戏资源
        /// </summary>
        OnlyGameResourceUpdated,

        /// <summary>
        /// NoMirrorChyanCdk
        /// </summary>
        NoMirrorChyanCdk,
    }

    public enum AppUpdateSource
    {
        /// <summary>
        /// Maa API
        /// </summary>
        MaaApi,

        /// <summary>
        /// MirrorChyan
        /// </summary>
        MirrorChyan,
    }

    /// <summary>
    /// Gets or sets a value indicating whether to show the update.
    /// </summary>
    public bool DoNotShowUpdate
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.Root.Update.DoNotShowUpdate = value;
        }
    } = ConfigFactory.Root.Update.DoNotShowUpdate;

    /// <summary>
    /// 如果是在更新后第一次启动，显示ReleaseNote弹窗，否则检查更新并下载更新包。
    /// </summary>
    /// <returns>Task</returns>
    public async Task ShowUpdateOrDownload()
    {
        if (IsFirstBootAfterUpdate)
        {
            IsFirstBootAfterUpdate = false;
            if (!DoNotShowUpdate)
            {
                Instances.WindowManager.ShowWindow(this);
            }
        }
        else
        {
            if (!SettingsViewModel.VersionUpdateSettings.StartupUpdateCheck)
            {
                return;
            }

            if (!IsDebugVersion())
            {
                if (SettingsViewModel.VersionUpdateSettings.UpdateSource == UpdateSource.MirrorChyan && string.IsNullOrEmpty(SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk))
                {
                    _ = Task.Run(() =>
                        MessageBoxHelper.Show(
                            LocalizationHelper.GetString("MirrorChyanSelectedButNoCdk"),
                            "cdk is empty!",
                            MessageBoxButton.OK,
                            MessageBoxImage.Warning,
                            ok: LocalizationHelper.GetString("Ok")));
                }

                await VersionUpdateAndAskToRestartAsync();
                await ResourceUpdater.ResourceUpdateAndReloadAsync();
            }
            else
            {
                // await ResourceUpdater.CheckAndDownloadResourceUpdate();
                // 跑个空任务避免 async warning
                await Task.Run(() => { });
            }
        }
    }

    /// <summary>
    /// 检查更新并下载更新包，如果成功则提示重启。
    /// </summary>
    /// <returns>Task</returns>
    public async Task VersionUpdateAndAskToRestartAsync()
    {
        if (SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates)
        {
            return;
        }

        var ret = await CheckAndDownloadVersionUpdate();
        if (ret == CheckUpdateRetT.OK)
        {
            _ = AskToRestart();
        }

        var toastMessage = ret switch {
            CheckUpdateRetT.NoNeedToUpdate => string.Empty,
            CheckUpdateRetT.NoNeedToUpdateDebugVersion => string.Empty,
            CheckUpdateRetT.AlreadyLatest => string.Empty,
            CheckUpdateRetT.UnknownError => LocalizationHelper.GetString("NewVersionDetectFailedTitle"),
            CheckUpdateRetT.NetworkError => LocalizationHelper.GetString("CheckNetworking"),
            CheckUpdateRetT.FailedToGetInfo => LocalizationHelper.GetString("GetReleaseNoteFailed"),
            CheckUpdateRetT.OK => string.Empty,
            CheckUpdateRetT.NewVersionIsBeingBuilt => string.Empty,
            CheckUpdateRetT.OnlyGameResourceUpdated => string.Empty,
            CheckUpdateRetT.NoMirrorChyanCdk => LocalizationHelper.GetString("MirrorChyanSelectedButNoCdk"),
            _ => string.Empty,
        };

        if (toastMessage != string.Empty)
        {
            ToastNotification.ShowDirect(toastMessage);
        }
    }

    /// <summary>
    /// 检查更新，并下载更新包。
    /// </summary>
    /// <returns>操作成功返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
    public async Task<CheckUpdateRetT> CheckAndDownloadVersionUpdate()
    {
        try
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = true;

            if (FakeUpdateHelper.IsEnabled)
            {
                return await HandleFakeUpdate();
            }

            var (checkRet, source) = await CheckUpdate();

            if (checkRet != CheckUpdateRetT.OK)
            {
                return checkRet;
            }

            return source switch {
                AppUpdateSource.MaaApi => await HandleUpdateFromMaaApi(),
                AppUpdateSource.MirrorChyan => await HandleUpdateFromMirrorChyan(),
                _ => CheckUpdateRetT.UnknownError,
            };
        }
        finally
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
        }
    }

    private async Task<CheckUpdateRetT> HandleFakeUpdate()
    {
        const double MinimumDetectedNewVersionDisplaySeconds = 0.5d;

        UpdateTag = FakeUpdateHelper.TargetVersion;

        UpdatePackageName = "MirrorChyanApp" + UpdateTag + ".zip";

        SettingsViewModel.VersionUpdateSettings.NewVersionFoundInfo = FakeUpdateHelper.HasPendingFakeUpdate
            ? $"{LocalizationHelper.GetString("NewVersionFoundTitle")}: {UpdateTag}"
            : string.Empty;

        if (!FakeUpdateHelper.HasPendingFakeUpdate)
        {
            return CheckUpdateRetT.AlreadyLatest;
        }

        await Task.Delay(TimeSpan.FromSeconds(MinimumDetectedNewVersionDisplaySeconds));
        await SimulateMirrorChyanDownloadAsync();

        return CheckUpdateRetT.OK;
    }

    private static async Task SimulateMirrorChyanDownloadAsync()
    {
        const long MinPackageSizeMiB = 20;
        const long MaxPackageSizeMiB = 80;
        const long BytesPerMiB = 1024 * 1024;
        const double GigabitBytesPerSecond = 1_000_000_000d / 8d;
        const double LogUpdateIntervalSeconds = 1d;
        const double MinimumDownloadDisplaySeconds = 0.5d;
        const double MinSpeedFactor = 0.25d;
        const double MaxSpeedFactor = 1.20d;
        const double MaxSpeedFactorStepDelta = 0.22d;

        long totalBytes = Random.Shared.NextInt64(MinPackageSizeMiB * BytesPerMiB, (MaxPackageSizeMiB * BytesPerMiB) + 1);

        OutputDownloadProgress(LocalizationHelper.GetString("NewVersionDownloadPreparing"), downloading: false, globalSource: false);

        long downloadedBytes = 0;
        double speedFactor = 1d + ((Random.Shared.NextDouble() - 0.5d) * 0.24d);
        bool hasReportedProgress = false;
        while (downloadedBytes < totalBytes)
        {
            speedFactor = Math.Clamp(
                speedFactor + ((Random.Shared.NextDouble() - 0.5d) * MaxSpeedFactorStepDelta * 2d),
                MinSpeedFactor,
                MaxSpeedFactor);

            double bytesPerSecond = GigabitBytesPerSecond * speedFactor;
            long remainingBytes = totalBytes - downloadedBytes;
            double remainingSeconds = remainingBytes / bytesPerSecond;
            double currentIntervalSeconds;

            if (!hasReportedProgress)
            {
                currentIntervalSeconds = Math.Max(MinimumDownloadDisplaySeconds, Math.Min(LogUpdateIntervalSeconds, remainingSeconds));
            }
            else
            {
                currentIntervalSeconds = Math.Min(LogUpdateIntervalSeconds, remainingSeconds);
            }

            long currentChunk = Math.Min(
                remainingBytes,
                Math.Max(1L, (long)Math.Round(bytesPerSecond * currentIntervalSeconds)));
            long currentValue = downloadedBytes + currentChunk;

            await Task.Delay(TimeSpan.FromSeconds(currentIntervalSeconds));

            OutputDownloadProgress(currentValue, totalBytes, (int)currentChunk, currentIntervalSeconds);

            downloadedBytes = currentValue;
            hasReportedProgress = true;
        }

        await Task.Delay(TimeSpan.FromSeconds(MinimumDownloadDisplaySeconds));
        OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadCompletedTitle"));
    }

    private async Task<CheckUpdateRetT> HandleUpdateFromMaaApi()
    {
        // 保存新版本的信息
        var name = _latestJson?["name"]?.ToString();
        UpdateTag = string.IsNullOrEmpty(name) ? (_latestJson?["tag_name"]?.ToString() ?? string.Empty) : name;
        SettingsViewModel.VersionUpdateSettings.NewVersionFoundInfo = $"{LocalizationHelper.GetString("NewVersionFoundTitle")}: {UpdateTag}";
        var body = _latestJson?["body"]?.ToString() ?? string.Empty;
        if (string.IsNullOrEmpty(body))
        {
            var curHash = ComparableHash(_curVersion);
            var latestHash = ComparableHash(_latestVersion);

            if (curHash != null && latestHash != null)
            {
                body = $"**Full Changelog**: [{curHash} -> {latestHash}](https://github.com/MaaAssistantArknights/MaaAssistantArknights/compare/{curHash}...{latestHash})";
            }
        }

        UpdateInfo = body;
        UpdateUrl = _latestJson?["html_url"]?.ToString() ?? string.Empty;

        bool otaFound = _assetsObject != null;
        bool goDownload = otaFound && SettingsViewModel.VersionUpdateSettings.AutoDownloadUpdatePackage;

        ShowUpdateInfo(otaFound, LocalizationHelper.GetString("NewVersionFoundButtonGoWebpage"), true);

        UpdatePackageName = _assetsObject?["name"]?.ToString() ?? string.Empty;

        if (!goDownload || string.IsNullOrWhiteSpace(UpdatePackageName))
        {
            OutputDownloadProgress(string.Empty, downloading: false);
            return CheckUpdateRetT.NoNeedToUpdate;
        }

        if (_assetsObject == null)
        {
            return CheckUpdateRetT.FailedToGetInfo;
        }

        string plannedPackagePath = GetPlannedUpdatePackagePath(UpdatePackageName);
        if (_requiresFullPackageConfirmation && !ConfirmFullPackageUpdate(plannedPackagePath))
        {
            _logger.Information("Full package download canceled by user before download: {PackagePath}", plannedPackagePath);
            OutputDownloadProgress(string.Empty, downloading: false);
            return CheckUpdateRetT.NoNeedToUpdate;
        }

        string? rawUrl = _assetsObject["browser_download_url"]?.ToString();
        var urls = new List<string>();

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource is UpdateSource.GitHub or UpdateSource.GitHubMirror && !SettingsViewModel.VersionUpdateSettings.ForceGithubGlobalSource)
        {
            var mirrors = _assetsObject["mirrors"]?.ToObject<List<string>>();

            if (mirrors != null)
            {
                urls.AddRange(mirrors);
            }
        }

        // 负载均衡
        // var rand = new Random();
        // urls = urls.OrderBy(_ => rand.Next()).ToList();
        if (rawUrl != null)
        {
            urls.Add(GithubMirrorHelper.ApplyMirrorIfNeeded(rawUrl));
        }

        _logger.Information("Start test legacy download urls");

        // run latency test parallel
        var tasks = urls.ConvertAll(url => Instances.HttpService.HeadAsync(new Uri(url)));
        var latencies = await Task.WhenAll(tasks);

        var proxy = ConfigFactory.Root.Update.Proxy;
        var hasProxy = !string.IsNullOrEmpty(proxy);

        // select the fastest mirror
        _logger.Information("Selecting the fastest mirror:");
        var selected = 0;
        for (int i = 0; i < latencies.Length; i++)
        {
            // ReSharper disable once StringLiteralTypo
            var isInChina = urls[i].Contains("s3.maa-org.net") || urls[i].Contains("maa-ota.annangela.cn");

            if (latencies[i] < 0)
            {
                _logger.Warning("\turl: {CDNUrl} not available", urls[i]);
                continue;
            }

            _logger.Information("\turl: {CDNUrl}, legacy: {1:0.00}ms", urls[i], latencies[i]);

            if (hasProxy && isInChina)
            {
                // 如果设置了代理，国内镜像的延迟加上一个固定值
                latencies[i] += 6480;
            }

            if (latencies[selected] < 0 || (latencies[i] >= 0 && latencies[i] < latencies[selected]))
            {
                selected = i;
            }
        }

        if (latencies[selected] < 0)
        {
            _logger.Error("All mirrors are not available");
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadFailedTitle"));
            return CheckUpdateRetT.NetworkError;
        }

        _logger.Information("Selected mirror: {CDNUrl}", urls[selected]);

        var downloaded = await DownloadGithubAssets(urls[selected], _assetsObject);
        if (downloaded)
        {
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadCompletedTitle"));
        }
        else
        {
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadFailedTitle"));
            {
                using var toast = new ToastNotification(LocalizationHelper.GetString("NewVersionDownloadFailedTitle"));
                toast.AppendContentText(LocalizationHelper.GetString("NewVersionDownloadFailedDesc"))
                    .AddButton(LocalizationHelper.GetString("NewVersionFoundButtonGoWebpage"), ToastNotification.GetActionTagForOpenWeb(UpdateUrl))
                    .Show();
            }

            return CheckUpdateRetT.NoNeedToUpdate;
        }

        return CheckUpdateRetT.OK;

        string? ComparableHash(string version)
        {
            if (IsStdVersion(version) || IsBetaVersion(version))
            {
                return version;
            }

            if (!SemVersion.TryParse(version, SemVersionStyles.AllowLowerV, out var semVersion) ||
                !IsNightlyVersion(semVersion))
            {
                return null;
            }

            // v4.6.6-1.g{Hash}
            // v4.6.7-beta.2.8.g{Hash}
            var commitHash = semVersion.PrereleaseIdentifiers[^1].ToString();
            if (commitHash.StartsWith('g'))
            {
                commitHash = commitHash.Remove(0, 1);
            }

            return commitHash;
        }
    }

    private void ShowUpdateInfo(bool otaFound, string? text, bool globalSource)
    {
        bool goDownload = otaFound && SettingsViewModel.VersionUpdateSettings.AutoDownloadUpdatePackage;

        using var toast = new ToastNotification((otaFound ? LocalizationHelper.GetString("NewVersionFoundTitle") : LocalizationHelper.GetString("NewVersionFoundButNoPackageTitle")) + " : " + UpdateTag);
        if (goDownload)
        {
            OutputDownloadProgress(LocalizationHelper.GetString("NewVersionDownloadPreparing"), false, globalSource);
            toast.AppendContentText(globalSource
                ? LocalizationHelper.GetString("NewVersionFoundDescDownloadingWithGlobalSource")
                : LocalizationHelper.GetString("NewVersionFoundDescDownloadingWithMirrorChyan"));
        }

        if (!otaFound)
        {
            toast.AppendContentText(LocalizationHelper.GetString("NewVersionFoundButNoPackageDesc"));
        }

        int count = 0;
        foreach (var line in UpdateInfo.Split('\n'))
        {
            if (line.StartsWith('#') || string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            toast.AppendContentText(line);
            if (++count >= 10)
            {
                break;
            }
        }

        if (!string.IsNullOrEmpty(text))
        {
            toast.AddButton(text, ToastNotification.GetActionTagForOpenWeb(globalSource ? UpdateUrl : MaaUrls.MirrorChyanManualUpdate));
        }

        toast.ShowUpdateVersion();
    }

    private async Task<CheckUpdateRetT> HandleUpdateFromMirrorChyan()
    {
        if (string.IsNullOrEmpty(_mirrorcDownloadUrl))
        {
            return CheckUpdateRetT.FailedToGetInfo;
        }

        UpdateTag = _mirrorcVersionName ?? string.Empty;
        UpdateInfo = _mirrorcReleaseNote ?? string.Empty;
        SettingsViewModel.VersionUpdateSettings.NewVersionFoundInfo = $"{LocalizationHelper.GetString("NewVersionFoundTitle")}: {UpdateTag}";

        bool goDownload = SettingsViewModel.VersionUpdateSettings.AutoDownloadUpdatePackage;

        ShowUpdateInfo(true, LocalizationHelper.GetString("NewVersionFoundButtonGoWebpage"), false);

        if (!goDownload)
        {
            OutputDownloadProgress(string.Empty, downloading: false);
            return CheckUpdateRetT.NoNeedToUpdate;
        }

        UpdatePackageName = "MirrorChyanApp" + _mirrorcVersionName + ".zip";
        string plannedPackagePath = GetPlannedUpdatePackagePath(UpdatePackageName);
        if (_requiresFullPackageConfirmation && !ConfirmFullPackageUpdate(plannedPackagePath))
        {
            _logger.Information("MirrorChyan full package download canceled by user before download: {PackagePath}", plannedPackagePath);
            OutputDownloadProgress(string.Empty, downloading: false);
            return CheckUpdateRetT.NoNeedToUpdate;
        }

        var downloaded = await DownloadFromMirrorChyan(_mirrorcDownloadUrl,
                    UpdatePackageName);

        if (downloaded)
        {
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadCompletedTitle"));
        }
        else
        {
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadFailedTitle"));
            {
                using var toast = new ToastNotification(LocalizationHelper.GetString("NewVersionDownloadFailedTitle"));
                toast.AppendContentText(LocalizationHelper.GetString("NewVersionDownloadFailedDesc"))
                     .Show();
            }

            return CheckUpdateRetT.NoNeedToUpdate;
        }

        AchievementTrackerHelper.Instance.Unlock(AchievementIds.MirrorChyanFirstUse);
        return CheckUpdateRetT.OK;
    }

    public async Task AskToRestart()
    {
        await AskToRestartCore(
            LocalizationHelper.GetString("NewVersionDownloadCompletedDesc"),
            LocalizationHelper.GetString("NewVersionDownloadCompletedTitle"));
    }

    public async Task AskToRestartForImportedPackage()
    {
        await AskToRestartCore(
            LocalizationHelper.GetString("LocalUpdatePackageImportedDesc"),
            LocalizationHelper.GetString("LocalUpdatePackageImportedTitle"));
    }

    /// <summary>
    /// 完整性修复流程的结果。
    /// </summary>
    public enum IntegrityRepairResult
    {
        /// <summary>
        /// 完整包已注册为待应用更新，等待重启。
        /// </summary>
        Succeeded,

        /// <summary>
        /// 用户在完整包风险确认弹窗中主动取消。
        /// </summary>
        Canceled,

        /// <summary>
        /// 解析下载源、下载或注册包失败。
        /// </summary>
        Failed,
    }

    /// <summary>
    /// 资源完整性修复：重新下载当前渠道的完整包并注册为待应用更新。
    /// 用户已在启动时的完整性检查弹窗中确认，不做版本新旧判断（允许重装同版本）。
    /// </summary>
    /// <returns>修复流程的结果，用于区分成功、用户取消与失败。</returns>
    public async Task<IntegrityRepairResult> RunIntegrityRepairAsync()
    {
        _logger.Information("Starting integrity repair");
        OutputDownloadProgress(LocalizationHelper.GetString("ResourceIntegrityRepairDownloading"), downloading: false);

        // 后续需要弹窗询问重启，保持 UI 上下文，不使用 ConfigureAwait(false)

        // 先解析下载源并确认完整包覆盖风险，再执行下载；
        // 与其他完整包更新入口保持一致，用户拒绝时不必下载 200MB+ 的完整包
        // 仅当更新来源配置为 MirrorChyan 且已填写 CDK 时走 MirrorChyan，其余来源直接走 maaApi
        var cdk = SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk.Trim();
        bool useMirrorChyan = SettingsViewModel.VersionUpdateSettings.UpdateSource == UpdateSource.MirrorChyan && !string.IsNullOrEmpty(cdk);

        var mirrorChyanPackage = useMirrorChyan ? await ResolveMirrorChyanRepairPackageAsync(cdk) : null;
        var maaApiPackage = mirrorChyanPackage is null ? await ResolveMaaApiRepairPackageAsync() : null;
        if (mirrorChyanPackage is null && maaApiPackage is null)
        {
            FailIntegrityRepair("Integrity repair: no full package source resolved");
            return IntegrityRepairResult.Failed;
        }

        string plannedPackagePath = GetPlannedUpdatePackagePath(mirrorChyanPackage?.PackageName ?? maaApiPackage!.PackageName);
        if (!ConfirmFullPackageUpdate(plannedPackagePath))
        {
            _logger.Information("Integrity repair full package application canceled by user: {PackagePath}", plannedPackagePath);

            // 反馈走任务队列的下载日志，与更新包下载失败的提示通道一致
            OutputDownloadProgress(LocalizationHelper.GetString("ResourceIntegrityRepairCanceled"), downloading: false);
            return IntegrityRepairResult.Canceled;
        }

        string? packagePath = null;
        if (mirrorChyanPackage is not null)
        {
            _logger.Information("Integrity repair: downloading full package {PackageName} from MirrorChyan", mirrorChyanPackage.PackageName);
            OutputDownloadProgress(LocalizationHelper.GetString("ResourceIntegrityRepairDownloading"), downloading: true, globalSource: false);
            if (await DownloadFromMirrorChyan(mirrorChyanPackage.DownloadUrl, mirrorChyanPackage.PackageName))
            {
                packagePath = GetPlannedUpdatePackagePath(mirrorChyanPackage.PackageName);
            }
            else
            {
                _logger.Warning("Integrity repair: MirrorChyan download failed, falling back to maaApi");
            }
        }

        if (packagePath is null)
        {
            maaApiPackage ??= await ResolveMaaApiRepairPackageAsync();
            if (maaApiPackage is null)
            {
                FailIntegrityRepair("Integrity repair: no full package resolved from maaApi");
                return IntegrityRepairResult.Failed;
            }

            _logger.Information("Integrity repair: downloading full package {PackageName} from maaApi", maaApiPackage.PackageName);
            OutputDownloadProgress(LocalizationHelper.GetString("ResourceIntegrityRepairDownloading"), downloading: true, globalSource: true);
            foreach (string url in maaApiPackage.DownloadUrls)
            {
                if (await DownloadGithubAssets(GithubMirrorHelper.ApplyMirrorIfNeeded(url), maaApiPackage.Asset))
                {
                    packagePath = GetPlannedUpdatePackagePath(maaApiPackage.PackageName);
                    break;
                }
            }
        }

        if (packagePath is null)
        {
            FailIntegrityRepair("Integrity repair download failed from all sources");
            return IntegrityRepairResult.Failed;
        }

        string arch = IsArm ? "arm64" : "x64";

        var importResult = PendingUpdateApplier.TryRegisterLocalPackage(
            packagePath,
            _curVersion,
            arch,
            inspection: null,
            allowSameVersion: true);
        if (importResult.Status is not PendingUpdateApplier.LocalPackageImportStatus.FullPackageRegistered
            and not PendingUpdateApplier.LocalPackageImportStatus.OtaPackageRegistered)
        {
            _logger.Error("Integrity repair package rejected: status={Status}, packagePath={PackagePath}", importResult.Status, packagePath);
            FailIntegrityRepair("Integrity repair package rejected");
            return IntegrityRepairResult.Failed;
        }

        _logger.Information("Integrity repair package registered: {PackagePath}", packagePath);
        OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("NewVersionDownloadCompletedTitle"));
        await AskToRestartForImportedPackage();
        return IntegrityRepairResult.Succeeded;
    }

    /// <summary>
    /// 输出完整性修复失败的状态（gui.log 与任务队列下载日志，不弹 Toast）。
    /// </summary>
    /// <param name="reason">失败原因（仅记录日志）。</param>
    private void FailIntegrityRepair(string reason)
    {
        _logger.Error(reason);
        OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("ResourceIntegrityRepairFailed"));
    }

    private sealed record MirrorChyanRepairPackage(string PackageName, string DownloadUrl);

    private sealed record MaaApiRepairPackage(string PackageName, JObject Asset, IReadOnlyList<string> DownloadUrls);

    /// <summary>
    /// 获取当前更新渠道的 MirrorChyan / maaApi 标识（stable / beta / alpha）。
    /// </summary>
    /// <returns>渠道标识字符串。</returns>
    private static string GetUpdateChannel()
    {
        return SettingsViewModel.VersionUpdateSettings.VersionType switch {
            VersionUpdateSettingsUserControlModel.UpdateVersionType.Beta => "beta",
            VersionUpdateSettingsUserControlModel.UpdateVersionType.Nightly => "alpha",
            _ => "stable",
        };
    }

    /// <summary>
    /// 构造 MirrorChyan 更新查询 URL。
    /// </summary>
    /// <param name="cdk">MirrorChyan CDK。</param>
    /// <param name="currentVersion">当前版本；传 null 时不携带，MirrorChyan 视为全新安装并直接返回完整包。</param>
    /// <returns>查询 URL。</returns>
    private string BuildMirrorChyanUpdateUrl(string cdk, string? currentVersion)
    {
        string arch = IsArm ? "arm64" : "x64";
        var spid = HardwareInfoUtility.GetMachineGuid().StableHash();
        string currentVersionPart = string.IsNullOrEmpty(currentVersion) ? string.Empty : $"current_version={currentVersion}&";
        return $"{MaaUrls.MirrorChyanAppUpdate}?{currentVersionPart}cdk={cdk}&user_agent=MaaWpfGui&os=win&arch={arch}&channel={GetUpdateChannel()}&sp_id={spid}";
    }

    /// <summary>
    /// 从 MirrorChyan 解析当前渠道完整包的下载信息（不执行下载）。
    /// </summary>
    /// <param name="cdk">已非空的 MirrorChyan CDK（调用方保证更新来源为 MirrorChyan）。</param>
    /// <returns>解析结果；解析失败时返回 null，交给 maaApi 兜底。</returns>
    private async Task<MirrorChyanRepairPackage?> ResolveMirrorChyanRepairPackageAsync(string cdk)
    {
        try
        {
            // 修复场景不传当前版本：MirrorChyan 视为全新安装，直接返回完整包，
            // 避免已是最新版本时拿不到下载地址
            string url = BuildMirrorChyanUpdateUrl(cdk, currentVersion: null);

            var data = await FetchMirrorChyanJsonAsync(url).ConfigureAwait(false);
            string? downloadUrl = data?["data"]?["url"]?.ToString();
            string? versionName = data?["data"]?["version_name"]?.ToString();
            if (string.IsNullOrEmpty(downloadUrl) || string.IsNullOrEmpty(versionName))
            {
                // 交给 maaApi 兜底
                return null;
            }

            // 文件名需符合完整包命名规范（MAA-vX.X.X-win-x64.zip），否则注册时的包名校验不通过
            return new MirrorChyanRepairPackage($"MAA-{versionName}-win-{(IsArm ? "arm64" : "x64")}.zip", downloadUrl);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to resolve full package from MirrorChyan for integrity repair");
            return null;
        }
    }

    /// <summary>
    /// 从 maaApi 解析当前渠道完整包的下载信息（不执行下载）。
    /// </summary>
    /// <returns>解析结果；解析失败时返回 null。</returns>
    private async Task<MaaApiRepairPackage?> ResolveMaaApiRepairPackageAsync()
    {
        try
        {
            string versionType = GetUpdateChannel();

            var (_, json) = await Instances.MaaApiService.RequestMaaApiWithCache($"version/{versionType}.json", false).ConfigureAwait(false);
            var assets = (JArray?)json?["details"]?["assets"];
            if (assets is null)
            {
                return null;
            }

            // 与 GetVersionDetailsByMaaApi 一致，只认 MAA-<版本>-win-<架构>.zip 命名，
            // 避免误选同样含 win 的 DebugSymbol 等组件资产
            string? latestVersion = json?["version"]?.ToString();
            if (string.IsNullOrEmpty(latestVersion))
            {
                _logger.Error("No version found on maaApi for integrity repair");
                return null;
            }

            string versionPrefix = $"maa-{latestVersion.ToLower()}-";
            foreach (var asset in assets)
            {
                string? name = asset["name"]?.ToString().ToLower();
                if (name is null || (IsArm ^ name.Contains("arm")) || !name.Contains("win") || name.Contains("ota"))
                {
                    continue;
                }

                if (!name.Contains(versionPrefix) || asset is not JObject fullPackage)
                {
                    continue;
                }

                string packageName = fullPackage["name"]!.ToString();
                string? rawUrl = fullPackage["browser_download_url"]?.ToString();
                if (string.IsNullOrEmpty(rawUrl))
                {
                    return null;
                }

                var urls = new List<string>();
                if (fullPackage["mirrors"]?.ToObject<List<string>>() is { } mirrors)
                {
                    urls.AddRange(mirrors);
                }

                urls.Add(rawUrl);
                return new MaaApiRepairPackage(packageName, fullPackage, urls);
            }

            _logger.Error("No full package asset found on maaApi for integrity repair");
            return null;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to resolve full package from maaApi for integrity repair");
            return null;
        }
    }

    public static bool ConfirmFullPackageUpdate(string packagePath)
    {
        string baseDir = Path.GetFullPath(PathsHelper.BaseDir);
        string normalizedPackagePath = Path.IsPathRooted(packagePath)
            ? Path.GetFullPath(packagePath)
            : GetPlannedUpdatePackagePath(packagePath);

        MessageBoxResult result = MessageBoxHelper.Show(
            LocalizationHelper.GetStringFormat("PendingFullUpdateManualConfirmDesc", baseDir, normalizedPackagePath),
            LocalizationHelper.GetString("PendingFullUpdateManualConfirmTitle"),
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning,
            yes: LocalizationHelper.GetString("PendingFullUpdateManualConfirmYes"),
            no: LocalizationHelper.GetString("PendingFullUpdateManualConfirmNo"));

        return result == MessageBoxResult.Yes;
    }

    private async Task AskToRestartCore(string description, string title)
    {
        // 自动安装，或用户点「立即更新/确定」：按启动设置决定是否写入 --skip-startup-auto-run。
        // 选「稍后」不会走到这里，之后手动启动是正常流程，不会带 skip 参数。
        string[] updateRestartArgs = Bootstrapper.GetUpdateRestartArgsIfEnabled();

        if (SettingsViewModel.VersionUpdateSettings.AutoInstallUpdatePackage)
        {
            if (FakeUpdateHelper.HasPendingFakeUpdate)
            {
                await _runningState.UntilIdleAsync(1000);
                _ = FakeUpdateHelper.Updating();
                return;
            }

            await Bootstrapper.RestartAfterIdleAsync(updateRestartArgs);
            return;
        }

        await _runningState.UntilIdleAsync(10000);

        var result = MessageBoxHelper.Show(
            description,
            title,
            MessageBoxButton.OKCancel,
            MessageBoxImage.Question,
            ok: LocalizationHelper.GetString("Ok"),
            cancel: LocalizationHelper.GetString("ManualRestart"));
        if (result == MessageBoxResult.OK)
        {
            if (FakeUpdateHelper.HasPendingFakeUpdate)
            {
                _ = FakeUpdateHelper.Updating();
                return;
            }

            if (updateRestartArgs.Length > 0)
            {
                Bootstrapper.ShutdownAndRestartWithArgs(updateRestartArgs);
            }
            else
            {
                Bootstrapper.ShutdownAndRestartWithoutArgs();
            }
        }
    }

    /// <summary>
    /// 检查更新。
    /// </summary>
    /// <returns>检查到更新返回 <see langword="true"/>，反之则返回 <see langword="false"/>。</returns>
    private async Task<(CheckUpdateRetT Ret, AppUpdateSource? Source)> CheckUpdate()
    {
        // 调试版不检查更新
        if (IsDebugVersion())
        {
            return (CheckUpdateRetT.NoNeedToUpdateDebugVersion, null);
        }

        if (SettingsViewModel.VersionUpdateSettings.UpdateSource == UpdateSource.MirrorChyan)
        {
            try
            {
                var ret = await CheckUpdateByMirrorChyan();
                if (ret is CheckUpdateRetT.OK or CheckUpdateRetT.AlreadyLatest)
                {
                    return (ret, AppUpdateSource.MirrorChyan);
                }
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to check update by MirrorChyan, rollback to maaApi");
            }
        }

        try
        {
            var ret = await CheckUpdateByMaaApi();
            return (ret, AppUpdateSource.MaaApi);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to check update by Maa API.");
            return (CheckUpdateRetT.FailedToGetInfo, AppUpdateSource.MaaApi);
        }
    }

    private async Task<CheckUpdateRetT> CheckUpdateByMaaApi()
    {
        var (_, json) = await Instances.MaaApiService.RequestMaaApiWithCache(MaaUpdateApi);

        if (json is null)
        {
            _logger.Error("Failed to get update info from MAA API.");
            return CheckUpdateRetT.FailedToGetInfo;
        }

        string versionType = GetUpdateChannel();

        var latestVersion = json[versionType]?["version"]?.ToString();

        latestVersion ??= string.Empty;

        if (!NeedToUpdate(latestVersion))
        {
            return CheckUpdateRetT.AlreadyLatest;
        }

        return await GetVersionDetailsByMaaApi(versionType);
    }

    private async Task<CheckUpdateRetT> GetVersionDetailsByMaaApi(string versionType)
    {
        _requiresFullPackageConfirmation = false;

        var (_, json) = await Instances.MaaApiService.RequestMaaApiWithCache($"version/{versionType}.json", false);
        if (json is null)
        {
            return CheckUpdateRetT.NetworkError;
        }

        string? latestVersion = json["version"]?.ToString();
        if (string.IsNullOrEmpty(latestVersion))
        {
            return CheckUpdateRetT.FailedToGetInfo;
        }

        if (!NeedToUpdate(latestVersion))
        {
            return CheckUpdateRetT.AlreadyLatest;
        }

        _latestVersion = latestVersion;
        _latestJson = json["details"] as JObject;
        if (_latestJson == null)
        {
            return CheckUpdateRetT.FailedToGetInfo;
        }

        _assetsObject = null;

        JObject? fullPackage = null;

        var curVersionLower = _curVersion.ToLower();
        var latestVersionLower = _latestVersion.ToLower();
        foreach (var curAssets in ((JArray?)_latestJson["assets"])!)
        {
            string? name = curAssets["name"]?.ToString().ToLower();
            if (name == null)
            {
                continue;
            }

            if (IsArm ^ name.Contains("arm"))
            {
                continue;
            }

            if (!name.Contains("win"))
            {
                continue;
            }

            if (name.Contains($"maa-{latestVersionLower}-"))
            {
                fullPackage = curAssets as JObject;
            }

            // ReSharper disable once InvertIf
            if (name.Contains("ota") && name.Contains($"{curVersionLower}_{latestVersionLower}"))
            {
                _assetsObject = curAssets as JObject;
                break;
            }
        }

        if (_assetsObject == null && fullPackage != null && SettingsViewModel.VersionUpdateSettings.AutoDownloadUpdatePackage)
        {
            _assetsObject = fullPackage;
            _requiresFullPackageConfirmation = true;
            _logger.Warning("No OTA package found, but full package found.");
            using var toast = new ToastNotification(LocalizationHelper.GetString("NewVersionNoOtaPackage"));
            toast.Show(30);
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NewVersionNoOtaPackage"), UiLogColor.Warning);
        }

        return CheckUpdateRetT.OK;
    }

    private async Task<CheckUpdateRetT> CheckUpdateByMirrorChyan()
    {
        _requiresFullPackageConfirmation = false;

        var cdk = SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk.Trim();
        string url = BuildMirrorChyanUpdateUrl(cdk, currentVersion: _curVersion);

        var data = await FetchMirrorChyanJsonAsync(url);
        if (data is null)
        {
            _logger.Error("mirrorc failed");
            _logger.Information("current_version: {CurVersion}, cdk: {Mask}, arch: {Arch}, channel: {Channel}",
                _curVersion, cdk.Mask(), IsArm ? "arm64" : "x64", GetUpdateChannel());
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = true;
            return CheckUpdateRetT.NetworkError;
        }

        var mirrorChyanCdkExpired = data["data"]?["cdk_expired_time"]?.ToObject<long?>();
        if (mirrorChyanCdkExpired.HasValue)
        {
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkExpiredTime = mirrorChyanCdkExpired.Value;
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = false;
        }
        else
        {
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = true;
        }

        var errorResult = HandleMirrorChyanErrorCode(data, mirrorChyanCdkExpired);
        if (errorResult.HasValue)
        {
            return errorResult.Value;
        }

        var version = data["data"]?["version_name"]?.ToString();
        if (string.IsNullOrEmpty(version))
        {
            return CheckUpdateRetT.UnknownError;
        }

        if (!NeedToUpdate(version))
        {
            return CheckUpdateRetT.AlreadyLatest;
        }

        data = await TryWaitForMirrorChyanOtaAsync(url, data);

        // 到这里已经确定有新版本了
        _logger.Information("New version found: {Version}", version);

        _mirrorcVersionName = version;
        _mirrorcReleaseNote = data["data"]?["release_note"]?.ToString();

        if (string.IsNullOrEmpty(cdk))
        {
            return CheckUpdateRetT.NoMirrorChyanCdk;
        }

        _mirrorcDownloadUrl = data["data"]?["url"]?.ToString();

        return CheckUpdateRetT.OK;
    }

    /// <summary>
    /// 向 MirrorChyan 发送 GET 请求并解析 JSON 响应。
    /// </summary>
    /// <returns>解析成功返回 JObject，失败返回 null。</returns>
    private static async Task<JObject?> FetchMirrorChyanJsonAsync(string url)
    {
        try
        {
            using var response = await Instances.HttpService.GetAsync(new(url), uriPartial: UriPartial.Path);
            var jsonStr = await response.Content.ReadAsStringAsync();
            _logger.Information("MirrorChyan response: {JsonStr}", jsonStr);
            try
            {
                return (JObject?)JsonConvert.DeserializeObject(jsonStr);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to deserialize json from MirrorChyan");
                return null;
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to send GET request to {Uri}", new Uri(url).GetLeftPart(UriPartial.Path));
            return null;
        }
    }

    /// <summary>
    /// 处理 MirrorChyan 错误码，显示对应的 Toast 提示。
    /// </summary>
    /// <returns>成功（无需处理）返回 null，出错返回 CheckUpdateRetT.UnknownError。</returns>
    private static CheckUpdateRetT? HandleMirrorChyanErrorCode(JObject data, long? mirrorChyanCdkExpired)
    {
        var errorCode = data["code"]?.ToObject<MirrorChyanErrorCode>() ?? MirrorChyanErrorCode.Undivided;
        if (errorCode == MirrorChyanErrorCode.Success)
        {
            return null;
        }

        switch (errorCode)
        {
            case MirrorChyanErrorCode.KeyExpired:
                ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkExpired"));
                SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = false;

                // 有人会第一次就填过期的 cdk 吗
                if (SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkExpiredTime == 0)
                {
                    SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkExpiredTime = 1;
                }

                // 如果上次查出来的时间比现在的还新，说明换了 cdk，重置过期时间
                if (!SettingsViewModel.VersionUpdateSettings.IsMirrorChyanCdkExpired)
                {
                    SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkExpiredTime = mirrorChyanCdkExpired ?? 1;
                }

                break;

            case MirrorChyanErrorCode.KeyInvalid:
                ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkInvalid"));
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.MirrorChyanCdkError);
                break;

            case MirrorChyanErrorCode.ResourceQuotaExhausted:
                ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkQuotaExhausted"));
                break;

            case MirrorChyanErrorCode.KeyMismatched:
                ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkMismatched"));
                break;

            case MirrorChyanErrorCode.KeyBlocked:
                ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkBlocked"));
                break;

            case MirrorChyanErrorCode.InvalidParams:
            case MirrorChyanErrorCode.ResourceNotFound:
            case MirrorChyanErrorCode.InvalidOs:
            case MirrorChyanErrorCode.InvalidArch:
            case MirrorChyanErrorCode.InvalidChannel:
            case MirrorChyanErrorCode.Undivided:
                ToastNotification.ShowDirect(data["msg"]?.ToString() ?? LocalizationHelper.GetString("GameResourceFailed"));
                break;
        }

        return CheckUpdateRetT.UnknownError;
    }

    /// <summary>
    /// MirrorChyan 在收到首个请求后会开始打包 OTA，打包过程中返回完整包。
    /// 等待 10s 后重试，通常此时 OTA 包已就绪；若重试后仍为完整包则走完整包更新途径。
    /// </summary>
    private async Task<JObject> TryWaitForMirrorChyanOtaAsync(string url, JObject data)
    {
        if (data["data"]?["update_type"]?.ToObject<string>() != "full")
        {
            return data;
        }

        _logger.Information("MirrorChyan returned full package, OTA may be building. Will retry after 10s.");
        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanBuildingOta"));
        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NewVersionIsBeingBuilt"), UiLogColor.Info);

        await Task.Delay(10000);

        // 重试请求，检查 OTA 包是否已就绪
        var retryData = await FetchMirrorChyanJsonAsync(url);
        if (retryData != null && retryData["data"]?["update_type"]?.ToObject<string>() != "full")
        {
            // 重试成功，OTA 包已就绪，使用新的响应数据
            _logger.Information("MirrorChyan OTA package ready after retry.");
            return retryData;
        }

        // 重试后仍是完整包或重试失败，走完整包更新途径
        _logger.Warning("MirrorChyan still returning full package after retry (or retry failed).");
        _requiresFullPackageConfirmation = true;
        if (SettingsViewModel.VersionUpdateSettings.AutoDownloadUpdatePackage)
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("NewVersionNoOtaPackage"));
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("NewVersionNoOtaPackage"), UiLogColor.Warning);
        }

        return data;
    }

    private bool NeedToUpdate(string latestVersion)
    {
        if (IsDebugVersion())
        {
            return false;
        }

        bool curParsed = SemVersion.TryParse(_curVersion, SemVersionStyles.AllowLowerV, out var curVersionObj);
        bool latestPared = SemVersion.TryParse(latestVersion, SemVersionStyles.AllowLowerV, out var latestVersionObj);
        if (curParsed && latestPared && curVersionObj != null && latestVersionObj != null)
        {
            return curVersionObj.CompareSortOrderTo(latestVersionObj) < 0;
        }

        return string.CompareOrdinal(_curVersion, latestVersion) < 0;
    }

    private static string GetPlannedUpdatePackagePath(string packageName)
    {
        return Path.GetFullPath(Path.Combine(PathsHelper.BaseDir, packageName));
    }

    /// <summary>
    /// 获取 GitHub Assets 对象对应的文件
    /// </summary>
    /// <param name="url">下载链接</param>
    /// <param name="assetsObject">Github Assets 对象</param>
    /// <returns>操作成功返回 true，反之则返回 false</returns>
    private static async Task<bool> DownloadGithubAssets(string url, JObject assetsObject)
    {
        try
        {
            var uri = new Uri(url);
            return await DownloadUpdatePackageWithRetryAsync(
                    () => Instances.HttpService.DownloadFileAsync(
                        uri,
                        assetsObject["name"]!.ToString(),
                        assetsObject["content_type"]?.ToString()),
                    uri)
                .ConfigureAwait(false);
        }
        catch (Exception)
        {
            return false;
        }
    }

    private static async Task<bool> DownloadFromMirrorChyan(string url, string filename)
    {
        try
        {
            var uri = new Uri(url);
            return await DownloadUpdatePackageWithRetryAsync(
                    () => Instances.HttpService.DownloadFileAsync(uri, filename),
                    uri)
                .ConfigureAwait(false);
        }
        catch (Exception)
        {
            return false;
        }
    }

    private static async Task<bool> DownloadUpdatePackageWithRetryAsync(Func<Task<bool>> download, Uri uri)
    {
        for (var attempt = 1; attempt <= UpdatePackageDownloadMaxAttempts; attempt++)
        {
            if (await download().ConfigureAwait(false))
            {
                return true;
            }

            if (attempt < UpdatePackageDownloadMaxAttempts)
            {
                var delay = TimeSpan.FromSeconds(attempt);
                _logger.Warning(
                    "Update package download failed for {Uri} (attempt {Attempt}/{MaxAttempts}); retrying in {Delay}",
                    uri.GetLeftPart(UriPartial.Path),
                    attempt,
                    UpdatePackageDownloadMaxAttempts,
                    delay);
                OutputDownloadProgress(
                    LocalizationHelper.GetStringFormat(
                        "NewVersionDownloadRetrying",
                        delay.TotalSeconds,
                        attempt,
                        UpdatePackageDownloadMaxAttempts),
                    downloading: false);
                await Task.Delay(delay).ConfigureAwait(false);
            }
        }

        _logger.Error(
            "Update package download finally failed after reaching maximum attempts for {Uri} (max attempts {MaxAttempts})",
            uri.GetLeftPart(UriPartial.Path),
            UpdatePackageDownloadMaxAttempts);
        return false;
    }

    public static void OutputDownloadProgress(long value = 0, long maximum = 1, int len = 0, double ts = 1, string? toolTip = null)
    {
        string progress = $"[{value / 1048576.0:F}MiB/{maximum / 1048576.0:F}MiB ({value * 100.0 / maximum:F}%)";

        double speedInKiBPerSecond = len / ts / 1024.0;

        var speedDisplay = speedInKiBPerSecond >= 1024
            ? $"{speedInKiBPerSecond / 1024.0:F} MiB/s"
            : $"{speedInKiBPerSecond:F} KiB/s";

        OutputDownloadProgress(progress + $" {speedDisplay}", toolTip: toolTip);
    }

    private static bool _globalSource = true;

    public static void OutputDownloadProgress(string output, bool downloading = true, bool? globalSource = null, string? toolTip = null)
    {
        globalSource ??= _globalSource;
        _globalSource = globalSource.Value;

        string fullText;
        if (downloading)
        {
            string key = globalSource.Value ? "NewVersionFoundDescDownloadingWithGlobalSource" : "NewVersionFoundDescDownloadingWithMirrorChyan";
            fullText = LocalizationHelper.GetString(key) + "\n" + output;
        }
        else
        {
            fullText = output;
        }

        Instances.TaskQueueViewModel?.UpdateDownloadLog(fullText, toolTip);
    }

    public bool IsDebugVersion(string? version = null)
    {
        // return false;
        version ??= _curVersion;

        // match case 1: DEBUG_VERSION
        // match case 2: v{Major}.{Minor}.{Patch}-{CommitDistance}-g{CommitHash}
        // match case 3: {CommitHash}
        return Regex.IsMatch(version, @"^(.*DEBUG.*|v\d+(\.\d+){1,3}-\d+-g[0-9a-f]{6,}|[^v][0-9a-f]{6,})$");
    }

    public bool IsStdVersion(string? version = null)
    {
        // 正式版：vX.X.X
        // DevBuild (CI)：yyyy-MM-dd-HH-mm-ss-{CommitHash[..7]}
        // DevBuild (Local)：yyyy-MM-dd-HH-mm-ss-{CommitHash[..7]}-Local
        // Release (Local Commit)：v.{CommitHash[..7]}-Local
        // Release (Local Tag)：{Tag}-Local
        // Debug (Local)：DEBUG_VERSION
        // Script Compiled：c{CommitHash[..7]}
        version ??= _curVersion;

        if (IsDebugVersion(version))
        {
            return false;
        }

        if (version.StartsWith('c') || version.StartsWith("20") || version.Contains("Local"))
        {
            return false;
        }

        if (!SemVersion.TryParse(version, SemVersionStyles.AllowLowerV, out var semVersion))
        {
            return false;
        }

        return !semVersion.IsPrerelease;
    }

    public bool IsBetaVersion(string? version = null)
    {
        version ??= _curVersion;

        if (IsDebugVersion(version))
        {
            return false;
        }

        if (version.StartsWith('c') || version.StartsWith("20") || version.Contains("Local"))
        {
            return false;
        }

        if (!SemVersion.TryParse(version, SemVersionStyles.AllowLowerV, out var semVersion))
        {
            return false;
        }

        return semVersion.IsPrerelease && !IsNightlyVersion(semVersion);
    }

    public static bool IsNightlyVersion(SemVersion version)
    {
        if (!version.IsPrerelease)
        {
            return false;
        }

        // ReSharper disable once CommentTypo
        // v{Major}.{Minor}.{Patch}-{Prerelease}.{CommitDistance}.g{CommitHash}
        // v4.6.7-beta.2.1.g1234567
        // v4.6.8-5.g1234567
        var lastId = version.PrereleaseIdentifiers.LastOrDefault().ToString();
        return lastId.StartsWith('g') && lastId.Length >= 7;
    }
}
