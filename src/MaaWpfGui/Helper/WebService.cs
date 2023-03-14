// <copyright file="WebService.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper
{
    public static class WebService
    {
        public const string RequestUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36 Edg/97.0.1072.76";

        private static string s_proxy = ViewStatusStorage.Get("VersionUpdate.Proxy", string.Empty);

        public static string Proxy
        {
            get => s_proxy; set
            {
                s_proxy = value;
                NormalClient = BuildNormalClient();
            }
        }

#nullable enable
        public static HttpClient NormalClient { get; private set; } = BuildNormalClient();

        private static HttpClient BuildNormalClient() => new HttpClient(new HttpClientHandler()
        {
            Proxy = string.IsNullOrWhiteSpace(s_proxy) ? null : new WebProxy(s_proxy),
        });

        public static async Task<string?> GetJsonAsync(string url)
        {
            try
            {
                using var message = new HttpRequestMessage(HttpMethod.Get, url);
                message.Headers.Accept.ParseAdd("application/json");
                using var response = await NormalClient.SendAsync(message);
                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                return null;
            }
        }

        public static async Task<string?> PostJsonAsync<T>(string url, T content)
        {
            return await PostJsonAsync(url, JsonConvert.SerializeObject(content));
        }

        public static async Task<string?> PostJsonAsync(string url, string body)
        {
            try
            {
                using var message = new HttpRequestMessage(HttpMethod.Post, url);
                message.Headers.Accept.ParseAdd("application/json");
                message.Content = new StringContent(body, Encoding.UTF8, "application/json");
                using var response = await NormalClient.SendAsync(message);
                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                return null;
            }
        }
#nullable disable

        public static string RequestGet(string url)
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

        public static string RequestPost(string url, string body)
        {
            try
            {
                HttpWebRequest httpWebRequest = (HttpWebRequest)WebRequest.Create(url);
                httpWebRequest.Method = "POST";
                httpWebRequest.UserAgent = RequestUserAgent;
                httpWebRequest.Accept = "application/vnd.github.v3+json";
                if (!string.IsNullOrWhiteSpace(Proxy))
                {
                    httpWebRequest.Proxy = new WebProxy(Proxy);
                }

                var bytes = Encoding.UTF8.GetBytes(body);
                httpWebRequest.ContentType = "application/json";
                httpWebRequest.ContentLength = bytes.Length;
                using (var requestStream = httpWebRequest.GetRequestStream())
                {
                    requestStream.Write(bytes, 0, bytes.Length);
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

        private const string CacheDir = "cache/";
        private const string MaaApi = "https://ota.maa.plus/MaaAssistantArknights/api/";

        // 如果请求失败，则读取缓存。否则写入缓存
        public static JObject RequestMaaApiWithCache(string api)
        {
            if (!Directory.Exists(CacheDir))
            {
                Directory.CreateDirectory(CacheDir);
            }

            var url = MaaApi + api;

            var response = RequestGet(url);
            if (response == null)
            {
                return LoadApiCache(api);
            }

            try
            {
                var json = (JObject)JsonConvert.DeserializeObject(response);
                var cache = CacheDir + api;
                Directory.CreateDirectory(Path.GetDirectoryName(cache));
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
            var cache = CacheDir + api;
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
