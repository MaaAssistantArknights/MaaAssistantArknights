// <copyright file="ResourceUpdater.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Threading.Tasks;
using MaaWpfGui.Constants;
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
            bool download = await DownloadFullPackageAsync(MaaUrls.GithubResourceUpdate, "MaaResourceGithub.zip").ConfigureAwait(false);
            if (!download)
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            // 解压到 MaaResource 文件夹
            try
            {
                if (Directory.Exists("MaaResourceGithub"))
                {
                    Directory.Delete("MaaResourceGithub", true);
                }

                ZipFile.ExtractToDirectory("MaaResourceGithub.zip", "MaaResourceGithub");
            }
            catch (Exception e)
            {
                _logger.Error("Failed to extract MaaResourceGithub.zip: " + e.Message);
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            // 把 \MaaResource-main 中的 cache 和 resource 文件夹复制到当前目录
            try
            {
                string sourcePath = Path.Combine("MaaResourceGithub", "MaaResource-main");
                string[] foldersToCopy = ["cache", "resource"];

                foreach (var folder in foldersToCopy)
                {
                    string sourceFolder = Path.Combine(sourcePath, folder);
                    string destinationFolder = Path.Combine(Directory.GetCurrentDirectory(), folder);

                    DirectoryMerge(sourceFolder, destinationFolder);
                }
            }
            catch (Exception e)
            {
                _logger.Error("Failed to copy folders: " + e.Message);
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            // 删除 MaaResource 文件夹 和 MaaResource.zip
            try
            {
                Directory.Delete("MaaResourceGithub", true);
                File.Delete("MaaResourceGithub.zip");
            }
            catch (Exception e)
            {
                _logger.Error("Failed to delete MaaResource: " + e.Message);
            }

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;

            return true;
        }

        /// <summary>
        /// 从 MirrorChyan 检查更新
        /// </summary>
        /// <returns>返回一个 <see cref="CheckUpdateRetT"/> 枚举值，指示更新检查的结果。
        /// <list type="bullet">
        /// <item><description><see cref="CheckUpdateRetT.AlreadyLatest"/>：已是最新版本。</description></item>
        /// <item><description><see cref="CheckUpdateRetT.OK"/>：有新版本。</description></item>
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

            var response = await Instances.HttpService.GetAsync(new(url), logQuery: false);
            _logger.Information($"current_version: {currentVersion}, cdk: {cdk.Mask()}");

            if (response is null)
            {
                _logger.Error("mirrorc failed");
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return (CheckUpdateRetT.NetworkError, null, null);
            }

            var jsonStr = await response.Content.ReadAsStringAsync();
            _logger.Information(jsonStr);
            JObject? data = null;
            try
            {
                data = (JObject?)JsonConvert.DeserializeObject(jsonStr);
            }
            catch (Exception ex)
            {
                _logger.Error("Failed to deserialize json: " + ex.Message);
            }

            if (data is null)
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return (CheckUpdateRetT.UnknownError, null, null);
            }

            var errorCode = data["code"]?.ToObject<Enums.MirrorChyanErrorCode>() ?? Enums.MirrorChyanErrorCode.Undivided;
            if (errorCode != Enums.MirrorChyanErrorCode.Success)
            {
                switch (errorCode)
                {
                    case Enums.MirrorChyanErrorCode.KeyExpired:
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkExpired"));
                        break;
                    case Enums.MirrorChyanErrorCode.KeyInvalid:
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkInvalid"));
                        break;
                    case Enums.MirrorChyanErrorCode.ResourceQuotaExhausted:
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkQuotaExhausted"));
                        break;
                    case Enums.MirrorChyanErrorCode.KeyMismatched:
                        ToastNotification.ShowDirect(LocalizationHelper.GetString("MirrorChyanCdkMismatched"));
                        break;
                    case Enums.MirrorChyanErrorCode.InvalidParams:
                    case Enums.MirrorChyanErrorCode.ResourceNotFound:
                    case Enums.MirrorChyanErrorCode.InvalidOs:
                    case Enums.MirrorChyanErrorCode.InvalidArch:
                    case Enums.MirrorChyanErrorCode.InvalidChannel:
                    case Enums.MirrorChyanErrorCode.Undivided:
                        ToastNotification.ShowDirect(data["msg"]?.ToString() ?? LocalizationHelper.GetString("GameResourceFailed"));
                        break;
                }

                return (CheckUpdateRetT.UnknownError, null, null);
            }

            if (!DateTime.TryParse(data["data"]?["version_name"]?.ToString(), out var version))
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return (CheckUpdateRetT.UnknownError, null, null);
            }

            if (DateTime.Compare(currentVersionDateTime, version) >= 0)
            {
                return (CheckUpdateRetT.AlreadyLatest, null, null);
            }

            // 到这里已经确定有新版本了
            var releaseNote = data["data"]?["release_note"]?.ToString();
            _logger.Information($"New version found: {version:yyyy-MM-dd+HH:mm:ss.fff}, {releaseNote}");

            releaseNote = LocalizationHelper.FormatResourceVersion(releaseNote, version);

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Format(LocalizationHelper.GetString("MirrorChyanResourceUpdateShortTip"), releaseNote);

            if (string.IsNullOrEmpty(cdk))
            {
                ToastNotification.ShowDirect(string.Format(LocalizationHelper.GetString("MirrorChyanResourceUpdateTip"), releaseNote));
                return (CheckUpdateRetT.OK, null, releaseNote);
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

            ToastNotification.ShowDirect(string.Format(LocalizationHelper.GetString("GameResourceUpdatingMirrorChyan"), releaseNote));
            bool download = await DownloadFullPackageAsync(url, "MaaResourceMirrorchyan.zip").ConfigureAwait(false);
            if (!download)
            {
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            try
            {
                if (Directory.Exists("MaaResourceMirrorchyan"))
                {
                    Directory.Delete("MaaResourceMirrorchyan", true);
                }

                ZipFile.ExtractToDirectory("MaaResourceMirrorchyan.zip", "MaaResourceMirrorchyan");
            }
            catch (Exception e)
            {
                _logger.Error("Failed to extract MaaResourceMirrorchyan.zip: " + e.Message);
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            try
            {
                const string SourcePath = "MaaResourceMirrorchyan";
                string destinationPath = Directory.GetCurrentDirectory();
                DirectoryMerge(SourcePath, destinationPath);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to copy folders: " + e.Message);
                ToastNotification.ShowDirect(LocalizationHelper.GetString("GameResourceFailed"));
                return false;
            }

            try
            {
                Directory.Delete("MaaResourceMirrorchyan", true);
                File.Delete("MaaResourceMirrorchyan.zip");
            }
            catch (Exception e)
            {
                _logger.Error("Failed to delete MaaResourceMirrorchyan: " + e.Message);
            }

            SettingsViewModel.VersionUpdateSettings.NewResourceFoundInfo = string.Empty;

            return true;
        }

        /// <summary>
        /// 检查并下载资源更新。
        /// </summary>
        /// <returns>返回一个 <see cref="CheckUpdateRetT"/> 枚举值，指示更新检查和下载的结果。
        /// <list type="bullet">
        /// <item><description><see cref="CheckUpdateRetT.AlreadyLatest"/>：已是最新版本。</description></item>
        /// <item><description><see cref="CheckUpdateRetT.OK"/>：有新版本。（海外源不会自动下载）</description></item>
        /// <item><description><see cref="CheckUpdateRetT.OnlyGameResourceUpdated"/>：下载成功。</description></item>
        /// <item><description><see cref="CheckUpdateRetT.NetworkError"/>：网络错误。</description></item>
        /// <item><description><see cref="CheckUpdateRetT.UnknownError"/>：其他错误。</description></item>
        /// </list></returns>
        public static async Task<CheckUpdateRetT> CheckAndDownloadResourceUpdate()
        {
            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = true;

            // 可以用 MirrorChyan 资源更新了喵
            var (ret, uri, releaseNote) = await CheckFromMirrorChyanAsync();
            if (ret != CheckUpdateRetT.OK || string.IsNullOrEmpty(uri))
            {
                SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
                return ret;
            }

            switch (SettingsViewModel.VersionUpdateSettings.UpdateSource)
            {
                case "Github":
                    break;

                case "MirrorChyan":
                    if (await DownloadFromMirrorChyanAsync(uri, releaseNote))
                    {
                        SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
                        return CheckUpdateRetT.OnlyGameResourceUpdated;
                    }

                    break;
            }

            SettingsViewModel.VersionUpdateSettings.IsCheckingForUpdates = false;
            return ret;
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

        private static async Task<bool> DownloadFullPackageAsync(string url, string saveTo)
        {
            using var response = await Instances.HttpService.GetAsync(new Uri(url));

            if (response is not { StatusCode: System.Net.HttpStatusCode.OK })
            {
                return false;
            }

            return await HttpResponseHelper.SaveResponseToFileAsync(response, saveTo);
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
