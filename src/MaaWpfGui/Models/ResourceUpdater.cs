// <copyright file="ResourceUpdater.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.IO.Compression;
using System.Globalization;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.ViewModels.Dialogs.VersionUpdateDialogViewModel;

namespace MaaWpfGui.Models;

public static class ResourceUpdater
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "ResourceUpdater");

    public static async Task<bool> UpdateFromGithubAsync()
    {
        ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdating"));

        const string GithubZipFileName = "MaaResourceGithub.zip";
        string githubZipFile = Path.Combine(PathsHelper.BaseDir, GithubZipFileName);
        string extractFolder = Path.Combine(PathsHelper.BaseDir, "MaaResourceGithub");

        if (!await DownloadFullPackageAsync(MaaUrls.GithubResourceUpdate, githubZipFile, true).ConfigureAwait(false))
        {
            Fail();
            return false;
        }

        OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceUpdatePreparing"));

        if (!ExtractAndMergeResourcePackage(githubZipFile, extractFolder))
        {
            Fail();
            return false;
        }

        SafeDeleteFile(githubZipFile);

        SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;
        OutputDownloadProgress(
            downloading: false,
            output: LocalizationHelper.GetString("GameResourceUpdated"),
            toolTip: LocalizationHelper.GetString("ResourceUpdateTip"));
        return true;

        static void Fail()
        {
            string msg = LocalizationHelper.GetString("GameResourceFailed");
            ToastNotification.ShowDirect(msg);
            OutputDownloadProgress(downloading: false, output: msg);
        }
    }

    /// <summary>
    /// 从 MirrorChyan 检查更新
    /// </summary>
    /// <returns>返回一个 <see cref="CheckUpdateRetT"/> 枚举值，指示更新检查的结果。
    /// <list type="bullet">
    /// <item><description><see cref="CheckUpdateRetT.AlreadyLatest"/>：已是最新版本。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.OK"/>：有新版本。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.NoMirrorChyanCdk"/>：有新版本，但未填写 cdk</description></item>
    /// <item><description><see cref="CheckUpdateRetT.NetworkError"/>：网络错误。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.UnknownError"/>：其他错误。</description></item>
    /// </list></returns>
    public static async Task<(CheckUpdateRetT Ret, string? UpdateUrl, string? ReleaseNote)> CheckFromMirrorChyanAsync()
    {
        // https://mirrorc.top/api/resources/MaaResource/latest?current_version=<当前版本日期，从 version.json 里拿时间戳>&cdk=<cdk>&sp_id=<唯一识别码>
        // 响应格式为 {"code":0,"msg":"success","data":{"version_name":"2025-01-22 14:28:32.839","version_number":9,"url":"<增量更新网址>"}}
        const string BaseUrl = MaaUrls.MirrorChyanResourceUpdate;
        var currentVersionDateTime = VersionUpdateSettingsUserControlModel
            .GetResourceVersionByClientType(SettingsViewModel.GameSettings.ClientType)
            .DateTime;
        var currentVersion = currentVersionDateTime.ToString("yyyy-MM-dd+HH:mm:ss.fff");
        var cdk = SettingsViewModel.VersionUpdateSettings.MirrorChyanCdk.Trim();
        var spid = HardwareInfoUtility.GetMachineGuid().StableHash();

        var url = $"{BaseUrl}?current_version={currentVersion}&cdk={cdk}&user_agent=MaaWpfGui&sp_id={spid}";

        HttpResponseMessage? response = null;
        try
        {
            response = await Instances.HttpService.GetAsync(new(url), uriPartial: UriPartial.Path);
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send GET request to {Uri}", new Uri(url).GetLeftPart(UriPartial.Path));
            _logger.Information("current_version: {CurrentVersion}, cdk: {Mask}", currentVersion, cdk.Mask());
        }

        if (response is null)
        {
            _logger.Error("mirrorc failed");
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = true;
            return (CheckUpdateRetT.NetworkError, null, null);
        }

        var jsonStr = await response.Content.ReadAsStringAsync();
        _logger.Information("{jsonStr}", jsonStr);
        JObject? data = null;
        try
        {
            data = (JObject?)JsonConvert.DeserializeObject(jsonStr);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to deserialize json.");
        }

        if (data is null)
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
            SettingsViewModel.VersionUpdateSettings.MirrorChyanCdkFetchFailed = true;
            return (CheckUpdateRetT.UnknownError, null, null);
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

        var errorCode = data["code"]?.ToObject<MirrorChyanErrorCode>() ?? MirrorChyanErrorCode.Undivided;
        if (errorCode != MirrorChyanErrorCode.Success)
        {
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

            return (CheckUpdateRetT.UnknownError, null, null);
        }

        if (!DateTimeOffset.TryParseExact(
                data["data"]?["version_name"]?.ToString(),
                "yyyy-MM-dd HH:mm:ss.fff",
                System.Globalization.CultureInfo.InvariantCulture,
                System.Globalization.DateTimeStyles.AssumeUniversal | System.Globalization.DateTimeStyles.AdjustToUniversal,
                out var versionTime))
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
            return (CheckUpdateRetT.UnknownError, null, null);
        }

        if (currentVersionDateTime >= versionTime)
        {
            return (CheckUpdateRetT.AlreadyLatest, null, null);
        }

        // 到这里已经确定有新版本了
        var releaseNote = data["data"]?["release_note"]?.ToString();
        _logger.Information("New version found: {DateTime:yyyy-MM-dd+HH:mm:ss.fff}, {ReleaseNote}", versionTime, releaseNote);

        releaseNote = LocalizationHelper.FormatVersion(releaseNote, versionTime);

        SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = LocalizationHelper.GetStringFormat("MirrorChyanResourceUpdateShortTip", releaseNote);

        if (string.IsNullOrEmpty(cdk))
        {
            return (CheckUpdateRetT.NoMirrorChyanCdk, null, releaseNote);
        }

        var uri = data["data"]?["url"]?.ToString();
        if (!string.IsNullOrEmpty(uri))
        {
            return (CheckUpdateRetT.OK, uri, releaseNote);
        }

        ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
        return (CheckUpdateRetT.UnknownError, null, null);
    }

    public static async Task<bool> DownloadFromMirrorChyanAsync(string? url, string? releaseNote)
    {
        if (string.IsNullOrEmpty(url))
        {
            return false;
        }

        ToastNotification.ShowDirect(LocalizationHelper.GetStringFormat(
            "GameResourceUpdatingMirrorChyan", releaseNote));

        const string MirrorchyanZipFile = "MaaResourceMirrorchyan.zip";
        const string ExtractFolder = "MaaResourceMirrorchyan";

        OutputDownloadProgress(string.Empty, globalSource: false);
        if (!await DownloadFullPackageAsync(url, MirrorchyanZipFile, false).ConfigureAwait(false))
        {
            Fail();
            return false;
        }

        OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceUpdatePreparing"));

        try
        {
            if (Directory.Exists(ExtractFolder))
            {
                Directory.Delete(ExtractFolder, true);
            }

            ZipFile.ExtractToDirectory(MirrorchyanZipFile, ExtractFolder);
        }
        catch (Exception e)
        {
            _logger.Error("Failed to extract MaaResourceMirrorchyan.zip: " + e.Message);
            Fail();
            return false;
        }

        try
        {
            DirectoryMerge(ExtractFolder, PathsHelper.BaseDir);
        }
        catch (Exception e)
        {
            _logger.Error("Failed to copy folders: " + e.Message);
            Fail();
            return false;
        }

        try
        {
            Directory.Delete(ExtractFolder, true);
            File.Delete(MirrorchyanZipFile);
        }
        catch (Exception e)
        {
            _logger.Error("Cleanup failed: " + e.Message);
        }

        SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;
        AchievementTrackerHelper.Instance.Unlock(AchievementIds.MirrorChyanFirstUse);
        OutputDownloadProgress(
            downloading: false,
            output: LocalizationHelper.GetString("GameResourceUpdated"),
            toolTip: LocalizationHelper.GetString("ResourceUpdateTip"));

        return true;

        static void Fail()
        {
            string msg = LocalizationHelper.GetString("GameResourceFailed");
            ToastNotification.ShowDirect(msg);
            OutputDownloadProgress(downloading: false, output: msg);
        }
    }

    /// <summary>
    /// 检查并下载资源更新。
    /// </summary>
    /// <returns>返回一个 <see cref="CheckUpdateRetT"/> 枚举值，指示更新检查和下载的结果。
    /// <list type="bullet">
    /// <item><description><see cref="CheckUpdateRetT.AlreadyLatest"/>：已是最新版本。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.OK"/>：有新版本。（海外源不会自动下载）</description></item>
    /// <item><description><see cref="CheckUpdateRetT.NoMirrorChyanCdk"/>：有新版本，但未填写 cdk</description></item>
    /// <item><description><see cref="CheckUpdateRetT.OnlyGameResourceUpdated"/>：下载成功。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.NetworkError"/>：网络错误。</description></item>
    /// <item><description><see cref="CheckUpdateRetT.UnknownError"/>：其他错误。</description></item>
    /// </list></returns>
    public static async Task<CheckUpdateRetT> CheckAndDownloadResourceUpdate()
    {
        try
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = true;

            var (ret, uri, releaseNote) = await CheckFromMirrorChyanAsync();
            if (ret == CheckUpdateRetT.NoMirrorChyanCdk)
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetStringFormat("MirrorChyanResourceUpdateTip", releaseNote));
            }

            if (ret != CheckUpdateRetT.OK)
            {
                return ret;
            }

            if (SettingsViewModel.VersionUpdateSettings.UpdateSource == "MirrorChyan" &&
                await DownloadFromMirrorChyanAsync(uri, releaseNote))
            {
                return CheckUpdateRetT.OnlyGameResourceUpdated;
            }

            return ret;
        }
        finally
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
        }
    }

    public static async Task ResourceUpdateAndReloadAsync()
    {
        if (SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates)
        {
            return;
        }

        var ret = await CheckAndDownloadResourceUpdate();
        if (ret == CheckUpdateRetT.OnlyGameResourceUpdated)
        {
            _ = ResourceReloadWhenIdleAsync();
        }
    }

    public static void ResourceReload()
    {
        Instances.AsstProxy.LoadResource();
        DataHelper.Reload();
        SettingsViewModel.VersionUpdateSettings.ResourceInfoUpdate();
        ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdated"));
    }

    private static bool _isReloading = false;

    public static async Task ResourceReloadWhenIdleAsync()
    {
        if (_isReloading)
        {
            _logger.Information("Resource is already reloading, skip this request.");
            return;
        }

        _isReloading = true;
        await Instances.AsstProxy.LoadResourceWhenIdleAsync();
        DataHelper.Reload();
        SettingsViewModel.VersionUpdateSettings.ResourceInfoUpdate();
        ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdated"));
        _isReloading = false;
    }

    /// <summary>
    /// 本地资源包导入结果。
    /// </summary>
    public enum LocalResourcePackageImportStatus
    {
        /// <summary>不是资源更新包。</summary>
        NotResourcePackage,

        /// <summary>资源版本低于或等于本地版本，已拒绝。</summary>
        VersionTooOld,

        /// <summary>导入成功。</summary>
        Imported,

        /// <summary>导入失败。</summary>
        Failed,
    }

    /// <summary>
    /// 检测压缩包是否为资源更新包，并尝试读取其版本时间戳。
    /// 只认 <c>MaaResource-main/resource/version.json</c> 这一层固定前缀结构，
    /// 避免误匹配完整包根目录的 <c>resource/version.json</c> 或 global 子目录中的同名文件。
    /// </summary>
    /// <param name="packagePath">压缩包路径。</param>
    /// <param name="versionDateTime">包内资源版本时间戳（last_updated）；无法读取则为 MinValue，且方法返回 false。</param>
    /// <returns>匹配 <c>MaaResource-main/resource/version.json</c> 且能解析出版本时间戳则返回 true。</returns>
    public static bool IsResourcePackage(string packagePath, out DateTimeOffset versionDateTime)
    {
        versionDateTime = DateTimeOffset.MinValue;
        try
        {
            using var archive = ZipFile.OpenRead(packagePath);
            var versionEntry = archive.Entries.FirstOrDefault(e =>
                e.FullName.Equals("MaaResource-main/resource/version.json", StringComparison.OrdinalIgnoreCase));
            if (versionEntry == null)
            {
                return false;
            }

            // 必须能解析出版本时间戳才算有效资源包，避免绕过版本校验
            return TryReadLastUpdated(versionEntry, out versionDateTime);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to inspect zip for resource package detection: {PackagePath}", packagePath);
            return false;
        }
    }

    /// <summary>
    /// 导入本地资源更新包。校验版本（小于等于本地则拒绝）、解压合并到 resource 目录并重载。
    /// </summary>
    /// <param name="packagePath">压缩包路径。</param>
    /// <param name="packageDateTime">由 <see cref="IsResourcePackage"/> 预检测得到的时间戳，避免重复扫描 zip。</param>
    /// <returns>导入结果状态。</returns>
    public static async Task<LocalResourcePackageImportStatus> ImportLocalResourcePackageAsync(
        string packagePath,
        DateTimeOffset packageDateTime)
    {
        if (SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates)
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
            return LocalResourcePackageImportStatus.Failed;
        }

        SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = true;
        try
        {
            var localDateTime = SettingsViewModel.VersionUpdateSettings.ResourceDateTime;

            // 版本校验：包内版本小于等于本地版本则拒绝（不执行解压）
            if (packageDateTime != DateTimeOffset.MinValue && packageDateTime <= localDateTime)
            {
                _logger.Information(
                    "Resource package rejected: package version {PackageVersion} (UTC) / {PackageVersionLocal} (local) is not newer than local {LocalVersion} (UTC) / {LocalVersionLocal} (local)",
                    packageDateTime,
                    packageDateTime.ToLocalTime(),
                    localDateTime,
                    localDateTime.ToLocalTime());
                ToastNotification.ShowDirect(LocalizationHelper.GetStringFormat(
                    "LocalResourcePackageTooOld",
                    packageDateTime.ToLocalTimeString(),
                    localDateTime.ToLocalTimeString()));
                return LocalResourcePackageImportStatus.VersionTooOld;
            }

            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdating"));

            // 解压 + 合并到本地 resource 目录（后台线程）
            string extractFolder = Path.Combine(PathsHelper.BaseDir, "MaaResourceImport");
            bool extractSuccess = await Task.Run(() =>
                ExtractAndMergeResourcePackage(packagePath, extractFolder)).ConfigureAwait(false);

            if (!extractSuccess)
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return LocalResourcePackageImportStatus.Failed;
            }

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;
            return LocalResourcePackageImportStatus.Imported;
        }
        finally
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
        }
    }

    /// <summary>
    /// 导入本地资源更新包并重载资源。重载采用 fire-and-forget，避免长时间占用 <see cref="VersionUpdateSettings.IsCheckingForUpdates"/>。
    /// </summary>
    /// <param name="packagePath">压缩包路径。</param>
    /// <param name="packageDateTime">由 <see cref="IsResourcePackage"/> 预检测得到的时间戳。</param>
    /// <returns>导入结果状态（重载是否完成不包含在内）。</returns>
    public static async Task<LocalResourcePackageImportStatus> ImportLocalResourcePackageAndReloadAsync(
        string packagePath,
        DateTimeOffset packageDateTime)
    {
        var status = await ImportLocalResourcePackageAsync(packagePath, packageDateTime).ConfigureAwait(false);
        if (status == LocalResourcePackageImportStatus.Imported)
        {
            // 先释放 IsCheckingForUpdates，再异步重载（可能等待任务队列空闲，耗时较长）
            _ = ResourceReloadWhenIdleAsync();
        }

        return status;
    }

    /// <summary>
    /// 解压资源包并合并到本地 resource 目录（GitHub 更新 / 拖入导入共用）。
    /// 包内须含 <c>MaaResource-main/resource/</c> 目录。
    /// </summary>
    /// <param name="zipPath">压缩包路径。</param>
    /// <param name="extractFolder">临时解压目录（应为绝对路径）。</param>
    /// <returns>成功返回 true。</returns>
    private static bool ExtractAndMergeResourcePackage(string zipPath, string extractFolder)
    {
        // 解压
        try
        {
            if (Directory.Exists(extractFolder))
            {
                Directory.Delete(extractFolder, true);
            }

            ZipFile.ExtractToDirectory(zipPath, extractFolder);
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to extract resource package {ZipPath}", zipPath);
            SafeDeleteDirectory(extractFolder);
            return false;
        }

        // 查找 resource 目录（固定 MaaResource-main/resource/）
        string? resourceDir = FindResourceDirectory(extractFolder);
        if (resourceDir == null)
        {
            _logger.Warning("No resource/ directory found in package: {ZipPath}", zipPath);
            SafeDeleteDirectory(extractFolder);
            return false;
        }

        // 合并到本地 resource 目录
        try
        {
            DirectoryMerge(resourceDir, PathsHelper.ResourceDir);
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to merge resource package from {ZipPath}", zipPath);
            SafeDeleteDirectory(extractFolder);
            return false;
        }

        SafeDeleteDirectory(extractFolder);
        return true;
    }

    /// <summary>
    /// 在解压目录中查找 resource 文件夹（<c>MaaResource-main/resource/</c>）。
    /// </summary>
    /// <param name="extractFolder">解压目录路径。</param>
    /// <returns>resource 目录完整路径，未找到则返回 null。</returns>
    private static string? FindResourceDirectory(string extractFolder)
    {
        // 固定结构：MaaResource-main/resource/
        string path = Path.Combine(extractFolder, "MaaResource-main", "resource");
        return Directory.Exists(path) ? path : null;
    }

    /// <summary>
    /// 从 zip entry 中读取 version.json 的 last_updated 时间戳。
    /// </summary>
    /// <param name="entry">version.json 的 zip entry。</param>
    /// <param name="dateTime">解析出的时间戳。</param>
    /// <returns>解析成功返回 true。</returns>
    private static bool TryReadLastUpdated(ZipArchiveEntry entry, out DateTimeOffset dateTime)
    {
        dateTime = DateTimeOffset.MinValue;
        try
        {
            using var stream = entry.Open();
            using var reader = new StreamReader(stream);
            var json = JsonConvert.DeserializeObject<JObject>(reader.ReadToEnd());
            var lastUpdated = json?["last_updated"]?.ToString();
            if (string.IsNullOrEmpty(lastUpdated))
            {
                return false;
            }

            dateTime = DateTimeOffset.ParseExact(
                lastUpdated,
                "yyyy-MM-dd HH:mm:ss.fff",
                CultureInfo.InvariantCulture,
                DateTimeStyles.AssumeUniversal);
            return true;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to parse resource version.json from zip entry: {Entry}", entry.FullName);
            return false;
        }
    }

    private static void SafeDeleteFile(string filePath)
    {
        try
        {
            if (File.Exists(filePath))
            {
                File.Delete(filePath);
            }
        }
        catch (Exception e)
        {
            _logger.Error("Failed to delete {FilePath}: {Message}", filePath, e.Message);
        }
    }

    private static void SafeDeleteDirectory(string dirPath)
    {
        try
        {
            if (Directory.Exists(dirPath))
            {
                Directory.Delete(dirPath, true);
            }
        }
        catch (Exception e)
        {
            _logger.Error("Failed to cleanup directory {DirPath}: {Message}", dirPath, e.Message);
        }
    }

    private static async Task<bool> DownloadFullPackageAsync(string url, string saveTo, bool globalSource)
    {
        try
        {
            return await Instances.HttpService.DownloadFileAsync(new(url), saveTo, "application/zip");
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send GET request to {Uri}", url);
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceFailed"), globalSource: globalSource);
            return false;
        }
    }

    private static void DirectoryMerge(string sourceDirName, string destDirName)
    {
        DirectoryInfo dir = new DirectoryInfo(sourceDirName);
        DirectoryInfo[] dirs = dir.GetDirectories();

        if (!dir.Exists)
        {
            throw new DirectoryNotFoundException("Source directory does not exist or could not be found: " + sourceDirName);
        }

        if (!Directory.Exists(destDirName))
        {
            Directory.CreateDirectory(destDirName);
        }

        FileInfo[] files = dir.GetFiles();
        foreach (FileInfo file in files)
        {
            if (file.Name == ".gitignore")
            {
                continue;
            }

            string tempPath = Path.Combine(destDirName, file.Name);
            file.CopyTo(tempPath, true); // 覆盖现有文件
        }

        foreach (DirectoryInfo subDir in dirs)
        {
            string tempPath = Path.Combine(destDirName, subDir.Name);
            DirectoryMerge(subDir.FullName, tempPath);
        }
    }
}
