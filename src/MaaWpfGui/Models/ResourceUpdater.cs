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
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
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

            if (ret == UpdateResult.Success)
            {
                _ = Execute.OnUIThreadAsync(() =>
                {
                    using var toast = new ToastNotification(LocalizationHelper.GetString("GameResourceUpdated"));
                    toast.Show();
                });
            }
        }

        private static async Task<string> GetResourceApi()
        {
#if DEBUG
            // 跑个空任务避免 async 警告
            await Task.Run(() => { });
            return MaaUrls.MaaResourceApi;

#else
            var response = await Instances.HttpService.GetAsync(new Uri(MaaUrls.AnnMirrorResourceApi), httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);
            return response is { StatusCode: System.Net.HttpStatusCode.OK }
                ? MaaUrls.AnnMirrorResourceApi
                : MaaUrls.MaaResourceApi;
#endif
        }

        private static async Task<bool> CheckUpdate(string baseUrl)
        {
            var url = baseUrl + MaaResourceVersion;

            var response = await ETagCache.FetchResponseWithEtag(url);
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

            var ret1 = await UpdateFilesWithIndex(baseUrl);

            if (ret1 == UpdateResult.Failed)
            {
                // 模板图片如果没更新成功，但是item_index.json更新成功了，这种情况会导致
                // 下次启动时检查item_index发现对应的文件不存在，则会弹窗报错
                // 所以如果模板图片没更新成功，干脆就不更新item_index.json了
                // 地图数据等也是同理
                return UpdateResult.Failed;
            }

            var ret2 = await UpdateSingleFiles(baseUrl);

            if (ret2 == UpdateResult.Failed)
            {
                return UpdateResult.Failed;
            }

            if (ret1 != UpdateResult.Success && ret2 != UpdateResult.Success)
            {
                return UpdateResult.NotModified;
            }

            PostProcVersionChecks();
            return UpdateResult.Success;

        }

        private static async Task<UpdateResult> UpdateSingleFiles(string baseUrl)
        {
            UpdateResult ret = UpdateResult.NotModified;

            // TODO: 加个文件存这些文件的 hash，如果 hash 没变就不下载了，只需要请求一次
            foreach (var file in _maaSingleFiles)
            {
                var sRet = await UpdateFileWithETag(baseUrl, file, file);

                if (sRet == UpdateResult.Failed)
                {
                    ret = UpdateResult.Failed;
                }

                if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                {
                    ret = UpdateResult.Success;
                }
            }

            return ret;
        }

        // 地图文件、掉落材料的图片、基建技能图片
        // 这些文件数量不固定，需要先获取索引文件，再根据索引文件下载
        private static async Task<UpdateResult> UpdateFilesWithIndex(string baseUrl)
        {
            var indexSRet = await UpdateFileWithETag(baseUrl, MaaDynamicFilesIndex, MaaDynamicFilesIndex);
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

            foreach (var file in context.Split('\n').ToList()
                         .Where(file => !string.IsNullOrEmpty(file))
                         .Where(file => !File.Exists(Path.Combine(Environment.CurrentDirectory, file))))
            {
                try
                {
                    var sRet = await UpdateFileWithETag(baseUrl, file, file);
                    if (sRet == UpdateResult.Failed)
                    {
                        return UpdateResult.Failed;
                    }

                    if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                    {
                        ret = UpdateResult.Success;
                    }
                }
                catch (Exception)
                {
                    // ignore
                }
            }

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

        private static async Task<UpdateResult> UpdateFileWithETag(string baseUrl, string file, string saveTo)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);
            var url = baseUrl + file;

            var response = await ETagCache.FetchResponseWithEtag(url, !File.Exists(saveTo));

            var updateResult = ResponseToUpdateResult(response);
            if (updateResult != UpdateResult.Success)
            {
                return updateResult;
            }

            if (!await HttpResponseHelper.SaveResponseToFileAsync(response, saveTo))
            {
                return UpdateResult.Failed;
            }

            ETagCache.Set(response);

            return UpdateResult.Success;
        }
    }
}
