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
using static MaaWpfGui.ViewModels.UI.VersionUpdateViewModel;

namespace MaaWpfGui.Models
{
    public static class ResourceUpdater
    {
        private static readonly ILogger _logger = Log.ForContext("SourceContext", "ResourceUpdater");

        public static async Task<bool> UpdateFromGithubAsync()
        {
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdating"));

            if (!await DownloadFullPackageAsync(MaaUrls.GithubResourceUpdate, "MaaResourceGithub.zip", true).ConfigureAwait(false))
            {
                Fail();
                return false;
            }

            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceUpdatePreparing"));

            const string GithubZipFile = "MaaResourceGithub.zip";
            const string ExtractFolder = "MaaResourceGithub";

            // 解压到 MaaResource 文件夹
            try
            {
                if (Directory.Exists(ExtractFolder))
                {
                    Directory.Delete(ExtractFolder, true);
                }

                ZipFile.ExtractToDirectory(GithubZipFile, ExtractFolder);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to extract MaaResourceGithub.zip: " + e.Message);
                Fail();
                return false;
            }

            // 把 \MaaResource-main 中的 cache 和 resource 文件夹复制到当前目录
            try
            {
                string basePath = Path.Combine(ExtractFolder, "MaaResource-main");
                foreach (var folder in new[] { "cache", "resource" })
                {
                    DirectoryMerge(
                        Path.Combine(basePath, folder),
                        Path.Combine(Directory.GetCurrentDirectory(), folder));
                }
            }
            catch (Exception e)
            {
                _logger.Error("Failed to copy folders: " + e.Message);
                Fail();
                return false;
            }

            // 删除 MaaResource 文件夹 和 MaaResource.zip
            try
            {
                Directory.Delete(ExtractFolder, true);
                File.Delete(GithubZipFile);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to delete MaaResource files: " + e.Message);
            }

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceUpdated"));
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
                return (CheckUpdateRetT.UnknownError, null, null);
            }

            var errorCode = data["code"]?.ToObject<MirrorChyanErrorCode>() ?? MirrorChyanErrorCode.Undivided;
            if (errorCode != MirrorChyanErrorCode.Success)
            {
                switch (errorCode)
                {
                    case MirrorChyanErrorCode.KeyExpired:
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkExpired"));
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

            if (!DateTime.TryParse(data["data"]?["version_name"]?.ToString(), out var versionTime))
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return (CheckUpdateRetT.UnknownError, null, null);
            }

            if (DateTime.Compare(currentVersionDateTime, versionTime) >= 0)
            {
                return (CheckUpdateRetT.AlreadyLatest, null, null);
            }

            // 到这里已经确定有新版本了
            var releaseNote = data["data"]?["release_note"]?.ToString();
            _logger.Information("New version found: {DateTime:yyyy-MM-dd+HH:mm:ss.fff}, {ReleaseNote}", versionTime, releaseNote);

            releaseNote = LocalizationHelper.FormatVersion(releaseNote, versionTime);

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Format(LocalizationHelper.GetString("MirrorChyanResourceUpdateShortTip"), releaseNote);

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

            ToastNotification.ShowDirect(string.Format(
                LocalizationHelper.GetString("GameResourceUpdatingMirrorChyan"), releaseNote));

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
                DirectoryMerge(ExtractFolder, Directory.GetCurrentDirectory());
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
            OutputDownloadProgress(downloading: false, output: LocalizationHelper.GetString("GameResourceUpdated"));

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
                    ToastNotification.ShowDirect(string.Format(LocalizationHelper.GetString("MirrorChyanResourceUpdateTip"), releaseNote));
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
                ResourceReload();
            }
        }

        public static void ResourceReload()
        {
            Instances.AsstProxy.LoadResource();
            DataHelper.Reload();
            SettingsViewModel.VersionUpdateSettings.ResourceInfoUpdate();
            ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceUpdated"));
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
}
