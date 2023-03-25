// <copyright file="HttpService.cs" company="MaaAssistantArknights">
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
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Web
{
    public class HttpService : IHttpService
    {
        public const string UserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36 Edg/97.0.1072.76";

        private static string Proxy => ConfigurationHelper.GetValue(ConfigurationKeys.UpdateProxy, string.Empty);

        private readonly ILogger _logger = Log.ForContext<HttpService>();

        private HttpClient _client;

        public HttpService()
        {
            ConfigurationHelper.ConfigurationUpdateEvent += (key, old, value) =>
            {
                if (key != ConfigurationKeys.UpdateProxy)
                {
                    return;
                }

                if (old != value)
                {
                    BuildHttpClient();
                }
            };

            BuildHttpClient();
        }

        public async Task<string> GetStringAsync(Uri uri, Dictionary<string, string> extraHeader = null)
        {
            var response = await GetAsync(uri, extraHeader);

            if (response != null)
            {
                return await response.Content.ReadAsStringAsync();
            }

            return null;
        }

        public async Task<Stream> GetStreamAsync(Uri uri, Dictionary<string, string> extraHeader = null)
        {
            var response = await GetAsync(uri, extraHeader);

            if (response != null)
            {
                return await response.Content.ReadAsStreamAsync();
            }

            return null;
        }

        public async Task<HttpResponseMessage> GetAsync(Uri uri, Dictionary<string, string> extraHeader = null)
        {
            try
            {
                var request = new HttpRequestMessage
                {
                    RequestUri = uri,
                    Method = HttpMethod.Get,
                };

                if (extraHeader != null)
                {
                    foreach (var kvp in extraHeader)
                    {
                        request.Headers.Add(kvp.Key, kvp.Value);
                    }
                }

                var response = await _client.SendAsync(request);

                return response.IsSuccessStatusCode is false ? null : response;
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", uri);
                return null;
            }
        }

        public async Task<string> PostAsJsonAsync<T>(Uri uri, T content, Dictionary<string, string> extraHeader = null)
        {
            try
            {
                var body = JsonSerializer.Serialize(content);
                var message = new HttpRequestMessage(HttpMethod.Post, uri);
                message.Headers.Accept.ParseAdd("application/json");
                message.Content = new StringContent(body, Encoding.UTF8, "application/json");
                var response = await _client.SendAsync(message);
                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send POST request to {Uri}", uri);
                return null;
            }
        }

        public async Task<bool> DownloadFileAsync(Uri uri, string fileName, string contentType = null)
        {
            string fileDir = Directory.GetCurrentDirectory();
            string fileNameWithTemp = fileName + ".temp";
            string fullFilePath = Path.Combine(fileDir, fileName);
            string fullFilePathWithTemp = Path.Combine(fileDir, fileNameWithTemp);

            var response = GetAsync(uri, extraHeader: new Dictionary<string, string>
            {
                { "Accept", contentType },
            }).ConfigureAwait(false).GetAwaiter().GetResult();

            if (response is null)
            {
                return false;
            }

            var success = true;
            try
            {
                var stream = response.Content.ReadAsStreamAsync().ConfigureAwait(false).GetAwaiter().GetResult();
                using var fileStream = new FileStream(fullFilePathWithTemp, FileMode.Create, FileAccess.Write);

                // 记录初始化
                long value = 0;
                int valueInOneSecond = 0;
                long fileMaximum = response.Content.Headers.ContentLength ?? 1;
                DateTime beforeDt = DateTime.Now;

                // Dangerous action
                VersionUpdateViewModel.OutputDownloadProgress();

                byte[] buffer = new byte[81920];
                int byteLen = await stream.ReadAsync(buffer, 0, buffer.Length);

                while (byteLen > 0)
                {
                    valueInOneSecond += byteLen;
                    double ts = DateTime.Now.Subtract(beforeDt).TotalSeconds;
                    if (ts > 1)
                    {
                        beforeDt = DateTime.Now;
                        value += valueInOneSecond;

                        // Dangerous action
                        VersionUpdateViewModel.OutputDownloadProgress(value, fileMaximum, valueInOneSecond, ts);
                        valueInOneSecond = 0;
                    }

                    // 输入输出
                    fileStream.Write(buffer, 0, byteLen);
                    byteLen = await stream.ReadAsync(buffer, 0, buffer.Length);
                }

                File.Copy(fullFilePathWithTemp, fullFilePath, true);
            }
            catch (Exception e)
            {
                _logger.Error("Failed to download file from {Uri}", uri);
                success = false;
            }
            finally
            {
                if (File.Exists(fullFilePathWithTemp))
                {
                    File.Delete(fullFilePathWithTemp);
                }
            }

            return success;
        }

        private void BuildHttpClient()
        {
            var proxyIsUri = Uri.TryCreate(Proxy, UriKind.RelativeOrAbsolute, out var uri);
            if (!string.IsNullOrEmpty(Proxy) && (!(_client is null)))
            {
                return;
            }

            var handler = new HttpClientHandler { AllowAutoRedirect = true, };

            if (proxyIsUri)
            {
                handler.Proxy = new WebProxy(uri);
                handler.UseProxy = true;
            }

            _client?.Dispose();
            _client = new HttpClient(handler);
            _client.DefaultRequestHeaders.Add("User-Agent", UserAgent);
        }
    }
}
