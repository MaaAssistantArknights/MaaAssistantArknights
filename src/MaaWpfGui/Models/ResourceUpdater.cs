using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Models
{
    public class ResourceUpdater
    {
        private const string MaaResourceApi = "https://ota.maa.plus/MaaAssistantArknights/MaaAssistantArknights/";
        private static readonly List<string> MaaSingleFiles = new List<string>
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

        private static readonly string MaaDynamicFilesIndex = "resource/dynamic_list.json";

        public enum UpdateResult
        {
            Success,
            Failed,
            NotModified,
        }

        public async Task<UpdateResult> Update()
        {
            var ret1 = await updateSingleFiles();
            var ret2 = await updateFilesWithIndex();

            if (ret1 == UpdateResult.Failed || ret2 == UpdateResult.Failed)
            {
                return UpdateResult.Failed;
            }
            else if (ret1 == UpdateResult.Success || ret2 == UpdateResult.Success)
            {
                return UpdateResult.Success;
            }

            return UpdateResult.NotModified;
        }

        private async Task<UpdateResult> updateSingleFiles()
        {
            UpdateResult ret = UpdateResult.NotModified;

            foreach (var file in MaaSingleFiles)
            {
                var sRet = await UpdateFileWithETage(MaaResourceApi, file, file);

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
        private async Task<UpdateResult> updateFilesWithIndex()
        {
            var sRet = await UpdateFileWithETage(MaaResourceApi, MaaDynamicFilesIndex, MaaDynamicFilesIndex);
            if (sRet == UpdateResult.Failed || sRet == UpdateResult.NotModified)
            {
                return sRet;
            }

            var ret = UpdateResult.NotModified;
            var context = File.ReadAllText(Path.Combine(Environment.CurrentDirectory, MaaDynamicFilesIndex));

            context.Split('\n').ToList().ForEach(async file =>
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

                var sRet = await UpdateFileWithETage(MaaResourceApi, file, file);
                if (sRet == UpdateResult.Failed)
                {
                    ret = UpdateResult.Failed;
                }

                if (ret == UpdateResult.NotModified && sRet == UpdateResult.Success)
                {
                    ret = UpdateResult.Success;
                }
            });

            return ret;
        }

        public static async Task<UpdateResult> UpdateFileWithETage(string baseUrl, string file, string saveTo)
        {
            saveTo = Path.Combine(Environment.CurrentDirectory, saveTo);
            var url = baseUrl + file;

            // 不存在的文件，不考虑etag，直接下载
            var etag = File.Exists(saveTo) ? ETagCache.Get(url) : string.Empty;

            var response = await Instances.HttpService.GetAsync(new Uri(url), new Dictionary<string, string>
                {
                    { "If-None-Match", etag },
                }).ConfigureAwait(false);

            if (response.StatusCode == System.Net.HttpStatusCode.NotModified)
            {
                return UpdateResult.NotModified;
            }

            if (response.StatusCode != System.Net.HttpStatusCode.OK)
            {
                // TODO: log
                return UpdateResult.Failed;
            }

            var content = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            ETagCache.Set(url, response.Headers.ETag.Tag);

            File.WriteAllText(Path.Combine(Environment.CurrentDirectory, saveTo), content);

            return UpdateResult.Success;
        }
    }
}
