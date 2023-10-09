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
        private static readonly List<string> _MaaSingleFiles = new List<string>
        {
            "resource/Arknights-Tile-Pos/overview.json",
            "resource/stages.json",
            "resource/recruitment.json",
            "resource/item_index.json",
            "resource/battle_data.json",
            "resource/infrast.json",
            "resource/version.json",
            "resource/global/YoStarJP/resource/recruitment.json",
            "resource/global/YoStarJP/resource/item_index.json",
            "resource/global/YoStarJP/resource/version.json",
            "resource/global/YoStarEN/resource/recruitment.json",
            "resource/global/YoStarEN/resource/item_index.json",
            "resource/global/YoStarEN/resource/version.json",
            "resource/global/YoStarKR/resource/recruitment.json",
            "resource/global/YoStarKR/resource/item_index.json",
            "resource/global/YoStarKR/resource/version.json",
            "resource/global/txwy/resource/recruitment.json",
            "resource/global/txwy/resource/item_index.json",
            "resource/global/txwy/resource/version.json",
        };

        private const string _MaaDynamicFilesIndex = "resource/dynamic_list.txt";

        public enum UpdateResult
        {
            Success,
            Failed,
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

        private static async Task<string> GetResourceAPI()
        {
            var response = await Instances.HttpService.GetAsync(new Uri(MaaUrls.AnnMirrorResourceApi), httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);
            if (response.StatusCode == System.Net.HttpStatusCode.OK)
            {
                return MaaUrls.AnnMirrorResourceApi;
            }

            return MaaUrls.MaaResourceApi;
        }

        public static async Task<UpdateResult> Update()
        {
            _updating = false;

            var baseUrl = await GetResourceAPI();
            var ret2 = await UpdateFilesWithIndex(baseUrl);
            var ret1 = await UpdateSingleFiles(baseUrl);
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

        private static async Task<UpdateResult> UpdateSingleFiles(string baseUrl)
        {
            UpdateResult ret = UpdateResult.NotModified;

            foreach (var file in _MaaSingleFiles)
            {
                var sRet = await UpdateFileWithETag(baseUrl, file, file);

                switch (sRet)
                {
                    case UpdateResult.Failed:
                        ret = UpdateResult.Failed;
                        break;

                    case UpdateResult.Success:
                        ETagCache.Save();
                        break;

                    case UpdateResult.NotModified:
                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
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
            var indexSRet = await UpdateFileWithETag(baseUrl, _MaaDynamicFilesIndex, _MaaDynamicFilesIndex);
            switch (indexSRet)
            {
                case UpdateResult.Failed:
                    return UpdateResult.Failed;

                case UpdateResult.Success:
                    ETagCache.Save();
                    break;

                case UpdateResult.NotModified:
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }

            var indexPath = Path.Combine(Environment.CurrentDirectory, _MaaDynamicFilesIndex);
            if (!File.Exists(indexPath))
            {
                return UpdateResult.Failed;
            }

            var ret = UpdateResult.NotModified;
            var context = File.ReadAllText(indexPath);

            // ReSharper disable once AsyncVoidLambda
            context.Split('\n').ToList().ForEach(async file =>
            {
                try
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
                catch (Exception)
                {
                    // ignore
                }
            });

            return ret;
        }

        private static bool _updating;

        private static async Task<UpdateResult> UpdateFileWithETag(string baseUrl, string file, string saveTo)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);
            var url = baseUrl + file;

            var response = await ETagCache.FetchResponseWithEtag(url, !File.Exists(saveTo));

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
                using var stream = await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
                using var fileStream = new FileStream(tempFile, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
                await stream.CopyToAsync(fileStream).ConfigureAwait(false);
            }
            catch (Exception)
            {
                // TODO: log
                return UpdateResult.Failed;
            }

            File.Copy(tempFile, saveTo, true);
            File.Delete(tempFile);

            ETagCache.Set(response);
            return UpdateResult.Success;
        }
    }
}
