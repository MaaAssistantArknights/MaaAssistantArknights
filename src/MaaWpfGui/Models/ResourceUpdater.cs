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
        private static readonly List<string> _maaSingleFiles = new List<string>
        {
            "resource/Arknights-Tile-Pos/overview.json",
            "resource/battle_data.json",
            "resource/infrast.json",
            "resource/item_index.json",
            "resource/recruitment.json",
            "resource/stages.json",
            "resource/version.json",
            "resource/global/txwy/resource/item_index.json",
            "resource/global/txwy/resource/recruitment.json",
            "resource/global/txwy/resource/version.json",
            "resource/global/YoStarEN/resource/item_index.json",
            "resource/global/YoStarEN/resource/recruitment.json",
            "resource/global/YoStarEN/resource/version.json",
            "resource/global/YoStarJP/resource/item_index.json",
            "resource/global/YoStarJP/resource/recruitment.json",
            "resource/global/YoStarJP/resource/version.json",
            "resource/global/YoStarKR/resource/item_index.json",
            "resource/global/YoStarKR/resource/recruitment.json",
            "resource/global/YoStarKR/resource/version.json",
        };

        private const string MaaDynamicFilesIndex = "resource/dynamic_list.txt";

        public enum UpdateResult
        {
            Success,
            Failed,
            NotModified,
        }

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

        public static async Task<UpdateResult> Update()
        {
            _updating = false;

            var ret1 = await UpdateSingleFiles();
            var ret2 = await UpdateFilesWithIndex();
            ETagCache.Save();

            if (ret1 == UpdateResult.Failed || ret2 == UpdateResult.Failed)
            {
                return UpdateResult.Failed;
            }

            if (ret1 == UpdateResult.Success || ret2 == UpdateResult.Success)
            {
                return UpdateResult.Success;
            }

            return UpdateResult.NotModified;
        }

        private static async Task<UpdateResult> UpdateSingleFiles()
        {
            UpdateResult ret = UpdateResult.NotModified;

            foreach (var file in _maaSingleFiles)
            {
                var sRet = await UpdateFileWithETag(MaaUrls.MaaResourceApi, file, file);

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
        private static async Task<UpdateResult> UpdateFilesWithIndex()
        {
            var indexSRet = await UpdateFileWithETag(MaaUrls.MaaResourceApi, MaaDynamicFilesIndex, MaaDynamicFilesIndex);
            if (indexSRet == UpdateResult.Failed || indexSRet == UpdateResult.NotModified)
            {
                return indexSRet;
            }

            var ret = UpdateResult.NotModified;
            var context = File.ReadAllText(Path.Combine(Environment.CurrentDirectory, MaaDynamicFilesIndex));

            context.Split('\n').ToList().ForEach(Action);

            return ret;

            // lambda 避免使用异步，任何未被 lambda 处理的异常都可能导致进程崩溃
            async void Action(string file)
            {
                if (string.IsNullOrEmpty(file))
                {
                    return;
                }

                // 这里有几千个文件，请求量太大了，且这些文件一般只新增，不修改。
                // 所以如果本地已经存在这些文件，就不再请求了。
                if (File.Exists(Path.Combine(Environment.CurrentDirectory, file)))
                {
                    return;
                }

                var sRet = await UpdateFileWithETag(MaaUrls.MaaResourceApi, file, file);
                if (sRet == UpdateResult.Failed)
                {
                    ret = UpdateResult.Failed;
                }

                if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                {
                    ret = UpdateResult.Success;
                }
            }
        }

        private static bool _updating;

        private static async Task<UpdateResult> UpdateFileWithETag(string baseUrl, string file, string saveTo)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);
            var url = baseUrl + file;

            // 不存在的文件，不考虑etag，直接下载
            var etag = File.Exists(saveTo) ? ETagCache.Get(url) : string.Empty;

            Dictionary<string, string> header = new Dictionary<string, string>
            {
                { "Accept", "application/octet-stream" },
            };

            if (!string.IsNullOrEmpty(etag))
            {
                header["If-None-Match"] = etag;
            }

            var response = await Instances.HttpService.GetAsync(new Uri(url), header, httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);
            if (response == null)
            {
                return UpdateResult.Failed;
            }

            if (response.StatusCode == System.Net.HttpStatusCode.NotModified)
            {
                return UpdateResult.NotModified;
            }

            if (response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                // TODO: log
                return UpdateResult.Failed;
            }

            if (!_updating)
            {
                _updating = true;
                _ = Execute.OnUIThreadAsync(() =>
                {
                    using var toast = new ToastNotification(LocalizationHelper.GetString("GameResourceUpdating"));
                    toast.Show();
                });
            }

            var tempFile = saveTo + ".tmp";
            try
            {
                using (var stream = await response.Content.ReadAsStreamAsync().ConfigureAwait(false))
                {
                    using var fileStream = new FileStream(tempFile, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
                    await stream.CopyToAsync(fileStream).ConfigureAwait(false);
                }
            }
            catch (Exception)
            {
                // TODO: log
                return UpdateResult.Failed;
            }

            File.Copy(tempFile, saveTo, true);
            File.Delete(tempFile);

            ETagCache.Set(url, response.Headers.ETag.Tag);
            return UpdateResult.Success;
        }
    }
}
