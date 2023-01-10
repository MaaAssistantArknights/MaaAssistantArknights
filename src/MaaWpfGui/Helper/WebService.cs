using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper
{
    public static class WebService
    {
        public const string RequestUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36 Edg/97.0.1072.76";

        public static string Proxy { get; set; } = string.Empty;

        public static string RequestUrl(string url)
        {
            try
            {
                HttpWebRequest httpWebRequest = (HttpWebRequest)WebRequest.Create(url);
                httpWebRequest.Method = "GET";
                httpWebRequest.UserAgent = RequestUserAgent;
                httpWebRequest.Accept = "application/vnd.github.v3+json";
                if (!string.IsNullOrWhiteSpace(Proxy))
                {
                    httpWebRequest.Proxy = new WebProxy(Proxy);
                }

                var httpWebResponse = httpWebRequest.GetResponse() as HttpWebResponse;
                if (httpWebResponse.StatusCode != HttpStatusCode.OK)
                {
                    return null;
                }

                var streamReader = new StreamReader(httpWebResponse.GetResponseStream(), Encoding.UTF8);
                var responseContent = streamReader.ReadToEnd();
                streamReader.Close();
                httpWebResponse.Close();
                return responseContent;
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                return null;
            }
        }

        private const string CacheDir = "cache/gui/";
        private const string MaaApi = "https://ota.maa.plus/MaaAssistantArknights/api/";

        // 如果请求失败，则读取缓存。否则写入缓存
        public static JObject RequestMaaApiWithCache(string api)
        {
            if (!Directory.Exists(CacheDir))
            {
                Directory.CreateDirectory(CacheDir);
            }

            var url = MaaApi + api;

            var response = RequestUrl(url);
            if (response == null)
            {
                return LoadApiCache(api);
            }

            try
            {
                var json = (JObject)JsonConvert.DeserializeObject(response);
                var cache = CacheDir + api.Replace('/', '_');
                File.WriteAllText(cache, response);

                return json;
            }
            catch
            {
                return null;
            }
        }

        public static JObject LoadApiCache(string api)
        {
            var cache = CacheDir + api.Replace('/', '_');
            if (!File.Exists(cache))
            {
                return null;
            }

            try
            {
                return (JObject)JsonConvert.DeserializeObject(File.ReadAllText(cache));
            }
            catch
            {
                return null;
            }
        }
    }
}
