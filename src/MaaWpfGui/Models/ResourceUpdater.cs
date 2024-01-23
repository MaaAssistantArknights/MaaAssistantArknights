// <copyright file="ResourceUpdater.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels;
using Stylet;

namespace MaaWpfGui.Models
{
    public static class ResourceUpdater
    {
        private const string MaaResourceVersion = "resource/version.json";
        private const string VersionChecksTemp = MaaResourceVersion + ".checks.tmp";

        private static readonly List<string> _maaSingleFiles = new List<string>
        {
            "resource/Arknights-Tile-Pos/overview.json",
            "resource/stages.json",
            "resource/recruitment.json",
            "resource/item_index.json",
            "resource/battle_data.json",
            "resource/infrast.json",
            "resource/global/YoStarJP/resource/recruitment.json",
            "resource/global/YoStarJP/resource/item_index.json",
            "resource/global/YoStarJP/resource/version.json",
            "resource/global/YoStarEN/resource/recruitment.json",
            "resource/global/YoStarEN/resource/item_index.json",
            "resource/global/YoStarEN/resource/version.json",
            "resource/global/txwy/resource/recruitment.json",
            "resource/global/txwy/resource/item_index.json",
            "resource/global/txwy/resource/version.json",
            "resource/global/YoStarKR/resource/recruitment.json",
            "resource/global/YoStarKR/resource/item_index.json",
            "resource/global/YoStarKR/resource/version.json",
        };

        private const string MaaDynamicFilesIndex = "resource/dynamic_list.txt";

        public enum UpdateResult
        {
            /// <summary>
            /// update resource success
            /// </summary>
            Success,

            /// <summary>
            /// update resource failed
            /// </summary>
            Failed,

            /// <summary>
            /// resource not modified
            /// </summary>
            NotModified,
        }

        // 只有 Release 版本才会检查更新
        // ReSharper disable once UnusedMember.Global
        public static async void UpdateAndToast()
        {
            var ret = await Update();

            string toastMessage = ret switch
            {
                UpdateResult.Failed => LocalizationHelper.GetString("GameResourceFailed"),
                UpdateResult.Success => LocalizationHelper.GetString("GameResourceUpdated"),
                _ => string.Empty,
            };
            if (!string.IsNullOrEmpty(toastMessage))
            {
                _ = Execute.OnUIThreadAsync(() =>
                {
                    using var toast = new ToastNotification(toastMessage);
                    toast.Show();
                });
            }
        }

        private static async Task<string> GetResourceApi()
        {
            string mirror = ConfigurationHelper.GetValue(ConfigurationKeys.ResourceApi, MaaUrls.MaaResourceApi);
            if (mirror != MaaUrls.MaaResourceApi && await IsMirrorAccessible(mirror))
            {
                return mirror;
            }

            var mirrorList = new List<string>
            {
                MaaUrls.S3ResourceApi,
                MaaUrls.R2ResourceApi,
                MaaUrls.AnnMirrorResourceApi,
            };

            while (mirrorList.Count != 0)
            {
                // random select a mirror
                var index = new Random().Next(0, mirrorList.Count);
                var mirrorUrl = mirrorList[index];

                if (await IsMirrorAccessible(mirrorUrl))
                {
                    mirror = mirrorUrl;
                    break;
                }

                mirrorList.RemoveAt(index);
            }

            if (mirror != MaaUrls.MaaResourceApi)
            {
                ConfigurationHelper.SetValue(ConfigurationKeys.ResourceApi, mirror);
            }

            return mirror;
        }

        private static async Task<bool> IsMirrorAccessible(string mirrorUrl)
        {
            using var response = await Instances.HttpService.GetAsync(
                new Uri(mirrorUrl + MaaResourceVersion),
                httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);

            return response is { StatusCode: System.Net.HttpStatusCode.OK };
        }

        private static async Task<bool> CheckUpdate(string baseUrl)
        {
            var url = baseUrl + MaaResourceVersion;

            using var response = await ETagCache.FetchResponseWithEtag(url);
            if (!(response is { StatusCode: System.Net.HttpStatusCode.OK }))
            {
                return false;
            }

            var tmp = Path.Combine(Environment.CurrentDirectory, VersionChecksTemp);

            if (!await HttpResponseHelper.SaveResponseToFileAsync(response, tmp))
            {
                return false;
            }

            _versionUrl = url;
            _versionEtag = response.Headers.ETag.Tag;
            _ = Execute.OnUIThreadAsync(() =>
            {
                using var toast = new ToastNotification(LocalizationHelper.GetString("GameResourceUpdating"));
                toast.Show();
            });

            return true;
        }

        private static string _versionUrl = string.Empty;
        private static string _versionEtag = string.Empty;

        private static void PostProcVersionChecks()
        {
            var tmp = Path.Combine(Environment.CurrentDirectory, VersionChecksTemp);
            var version = Path.Combine(Environment.CurrentDirectory, MaaResourceVersion);

            if (File.Exists(tmp))
            {
                File.Copy(tmp, version, true);
                File.Delete(tmp);
            }
            else
            {
                return;
            }

            ETagCache.Set(_versionUrl, _versionEtag);
        }

        public static async Task<UpdateResult> Update()
        {
            var baseUrl = await GetResourceApi();
            bool needUpdate = await CheckUpdate(baseUrl);
            if (!needUpdate)
            {
                return UpdateResult.NotModified;
            }

            OutputDownloadProgress(1, LocalizationHelper.GetString("GameResourceUpdatePreparing"));
            var ret1 = await UpdateFilesWithIndex(baseUrl);

            if (ret1 == UpdateResult.Failed)
            {
                // 模板图片如果没更新成功，但是item_index.json更新成功了，这种情况会导致
                // 下次启动时检查item_index发现对应的文件不存在，则会弹窗报错
                // 所以如果模板图片没更新成功，干脆就不更新item_index.json了
                // 地图数据等也是同理
                return UpdateResult.Failed;
            }

            OutputDownloadProgress(2, LocalizationHelper.GetString("GameResourceUpdatePreparing"));
            var ret2 = await UpdateSingleFiles(baseUrl);

            if (ret2 == UpdateResult.Failed)
            {
                return UpdateResult.Failed;
            }

            PostProcVersionChecks();

            if (ret1 == UpdateResult.Success || ret2 == UpdateResult.Success)
            {
                OutputDownloadProgress(LocalizationHelper.GetString("GameResourceUpdated"));
                return UpdateResult.Success;
            }

            OutputDownloadProgress(LocalizationHelper.GetString("GameResourceNotModified"));
            return UpdateResult.NotModified;
        }

        private static async Task<UpdateResult> UpdateSingleFiles(string baseUrl, int maxRetryTime = 2)
        {
            UpdateResult ret = UpdateResult.NotModified;

            var maxCount = _maaSingleFiles.Count;
            var count = 0;

            // TODO: 加个文件存这些文件的 hash，如果 hash 没变就不下载了，只需要请求一次
            foreach (var file in _maaSingleFiles)
            {
                await Task.Delay(1000);

                var sRet = await UpdateFileWithETag(baseUrl, file.Replace("#", "%23"), file, maxRetryTime);

                if (sRet == UpdateResult.Failed)
                {
                    OutputDownloadProgress(LocalizationHelper.GetString("GameResourceFailed"));
                    return UpdateResult.Failed;
                }

                OutputDownloadProgress(2, ++count, maxCount);
                if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                {
                    ret = UpdateResult.Success;
                }
            }

            OutputDownloadProgress(2, "Update completed");
            return ret;
        }

        // 地图文件、掉落材料的图片、基建技能图片
        // 这些文件数量不固定，需要先获取索引文件，再根据索引文件下载
        private static async Task<UpdateResult> UpdateFilesWithIndex(string baseUrl, int maxRetryTime = 2)
        {
            var indexSRet = await UpdateFileWithETag(baseUrl, MaaDynamicFilesIndex, MaaDynamicFilesIndex, maxRetryTime);
            if (indexSRet == UpdateResult.Failed)
            {
                return UpdateResult.Failed;
            }

            var indexPath = Path.Combine(Environment.CurrentDirectory, MaaDynamicFilesIndex);
            if (!File.Exists(indexPath))
            {
                return UpdateResult.Failed;
            }

            var ret = UpdateResult.NotModified;
            var context = File.ReadAllText(indexPath);
            var maxCount = context
                .Split('\n')
                .ToList()
                .Where(file => !string.IsNullOrEmpty(file))
                .Count(file => !File.Exists(Path.Combine(Environment.CurrentDirectory, file)));
            var count = 0;

            foreach (var file in context.Split('\n').ToList()
                         .Where(file => !string.IsNullOrEmpty(file))
                         .Where(file => !File.Exists(Path.Combine(Environment.CurrentDirectory, file))))
            {
                await Task.Delay(1000);

                var sRet = await UpdateFileWithETag(baseUrl, file.Replace("#", "%23"), file, maxRetryTime);
                if (sRet == UpdateResult.Failed)
                {
                    OutputDownloadProgress(LocalizationHelper.GetString("GameResourceFailed"));
                    return UpdateResult.Failed;
                }

                OutputDownloadProgress(1, ++count, maxCount);
                if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                {
                    ret = UpdateResult.Success;
                }
            }

            OutputDownloadProgress(1, "Update completed");
            return ret;
        }

        private static UpdateResult ResponseToUpdateResult(HttpResponseMessage response)
        {
            if (response == null)
            {
                return UpdateResult.Failed;
            }

            if (response.StatusCode == System.Net.HttpStatusCode.NotModified)
            {
                return UpdateResult.NotModified;
            }

            return response.StatusCode == System.Net.HttpStatusCode.OK
                ? UpdateResult.Success
                : UpdateResult.Failed;
        }

        private static async Task<UpdateResult> UpdateFileWithETag(string baseUrl, string file, string saveTo, int maxRetryTime = 0)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);
            var url = baseUrl + file;

            int retryCount = 0;
            UpdateResult updateResult;
            do
            {
                using var response = await ETagCache.FetchResponseWithEtag(url, !File.Exists(saveTo));
                updateResult = ResponseToUpdateResult(response);

                switch (updateResult)
                {
                    case UpdateResult.Success
                        when !await HttpResponseHelper.SaveResponseToFileAsync(response, saveTo):
                        return UpdateResult.Failed;
                    case UpdateResult.Success:
                        ETagCache.Set(response);
                        return UpdateResult.Success;
                    case UpdateResult.NotModified:
                        return UpdateResult.NotModified;
                    case UpdateResult.Failed:
                    default:
                        await Task.Delay(5000);
                        break;
                }
            }
            while (retryCount++ < maxRetryTime);

            return updateResult;
        }

        private static ObservableCollection<LogItemViewModel> _logItemViewModels;

        private static void OutputDownloadProgress(int index, int count = 0, int maxCount = 1)
        {
            OutputDownloadProgress(index, $"{count}/{maxCount}({100 * count / maxCount}%)");
        }

        private static void OutputDownloadProgress(int index, string output)
        {
            OutputDownloadProgress($"index {index}/2: {output}");
        }

        private static void OutputDownloadProgress(string output)
        {
            _logItemViewModels = Instances.TaskQueueViewModel.LogItemViewModels;
            if (_logItemViewModels == null)
            {
                return;
            }

            var log = new LogItemViewModel(LocalizationHelper.GetString("GameResourceUpdating") + "\n" + output, UiLogColor.Download);

            Application.Current.Dispatcher.Invoke(() =>
            {
                if (_logItemViewModels.Count > 0 && _logItemViewModels[0].Color == UiLogColor.Download)
                {
                    if (!string.IsNullOrEmpty(output))
                    {
                        _logItemViewModels[0] = log;
                    }
                    else
                    {
                        _logItemViewModels.RemoveAt(0);
                    }
                }
                else if (!string.IsNullOrEmpty(output))
                {
                    _logItemViewModels.Clear();
                    _logItemViewModels.Add(log);
                }
            });
        }
    }
}
