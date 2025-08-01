// <copyright file="HttpService.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Web
{
    public class HttpService : IHttpService
    {
        private readonly string UserAgent;

        private static string Proxy
        {
            get
            {
                var proxy = SettingsViewModel.VersionUpdateSettings.Proxy;
                if (string.IsNullOrEmpty(proxy))
                {
                    return string.Empty;
                }

                return proxy.Contains("://") ? proxy : SettingsViewModel.VersionUpdateSettings.ProxyType + $"://{proxy}";
            }
        }

        private readonly ILogger _logger = Log.ForContext<HttpService>();

        private HttpClient _client;

        public HttpService()
        {
            string uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";
            UserAgent = $"MaaWpfGui/{uiVersion}";

            ConfigurationHelper.ConfigurationUpdateEvent += (key, old, value) =>
            {
                if (key != ConfigurationKeys.UpdateProxy)
                {
                    return;
                }

                if (old == value)
                {
                    return;
                }

                if (GetProxy() == null)
                {
                    _logger.Warning("Proxy is not a valid URI, and HttpClient is not null, keep using the original HttpClient");
                    return;
                }

                _client = BuildHttpClient();
            };

            _client = BuildHttpClient();
        }

        public async Task<double> HeadAsync(Uri uri, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
        {
            try
            {
                var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Head, Version = HttpVersion.Version20, };

                if (extraHeader != null)
                {
                    foreach (var kvp in extraHeader)
                    {
                        request.Headers.Add(kvp.Key, kvp.Value);
                    }
                }

                request.Headers.ConnectionClose = true;

                var stopwatch = Stopwatch.StartNew();
                var response = await _client.SendAsync(request).ConfigureAwait(false);
                stopwatch.Stop();
                response.Log(uriPartial, stopwatch.Elapsed.TotalMilliseconds);

                return response.IsSuccessStatusCode is false ? -1.0 : stopwatch.Elapsed.TotalMilliseconds;
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", uri);
                return -1.0;
            }
        }

        public async Task<string?> GetStringAsync(Uri uri, Dictionary<string, string>? extraHeader = null, HttpCompletionOption httpCompletionOption = HttpCompletionOption.ResponseContentRead)
        {
            try
            {
                var response = await GetAsync(uri, extraHeader, httpCompletionOption);
                if (response.StatusCode != HttpStatusCode.OK)
                {
                    return null;
                }

                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", uri);
                return null;
            }
        }

        public async Task<Stream?> GetStreamAsync(Uri uri, Dictionary<string, string>? extraHeader = null, HttpCompletionOption httpCompletionOption = HttpCompletionOption.ResponseContentRead)
        {
            try
            {
                var response = await GetAsync(uri, extraHeader, httpCompletionOption);
                if (response.StatusCode != HttpStatusCode.OK)
                {
                    return null;
                }

                return await response.Content.ReadAsStreamAsync();
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", uri);
                return null;
            }
        }

        public async Task<HttpResponseMessage> GetAsync(Uri uri, Dictionary<string, string>? extraHeader = null, HttpCompletionOption httpCompletionOption = HttpCompletionOption.ResponseHeadersRead, UriPartial uriPartial = UriPartial.Query)
        {
            var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Get, Version = HttpVersion.Version20, };
            if (extraHeader != null)
            {
                foreach (var kvp in extraHeader)
                {
                    request.Headers.Add(kvp.Key, kvp.Value);
                }
            }

            var stopwatch = Stopwatch.StartNew();
            var response = await _client.SendAsync(request, httpCompletionOption);
            stopwatch.Stop();
            response.Log(uriPartial, stopwatch.Elapsed.TotalMilliseconds);
            return response;
        }

        public async Task<string?> PostAsJsonAsync<T>(Uri uri, T content, Dictionary<string, string>? extraHeader = null)
        {
            try
            {
                var response = await PostAsync(uri, new StringContent(JsonSerializer.Serialize(content), Encoding.UTF8, "application/json"), extraHeader);
                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send POST request to {Uri}", uri);
                return null;
            }
        }

        public async Task<string?> PostAsFormUrlEncodedAsync(Uri uri, Dictionary<string, string?> content, Dictionary<string, string>? extraHeader = null)
        {
            try
            {
                var response = await PostAsync(uri, new FormUrlEncodedContent(content), extraHeader);
                return await response.Content.ReadAsStringAsync();
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send POST request to {Uri}", uri);
                return null;
            }
        }

        public async Task<HttpResponseMessage> PostAsync(Uri uri, HttpContent content, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
        {
            var message = new HttpRequestMessage(HttpMethod.Post, uri) { Version = HttpVersion.Version20 };
            if (extraHeader is not null)
            {
                foreach (var header in extraHeader)
                {
                    message.Headers.Add(header.Key, header.Value);
                }
            }

            message.Headers.Accept.ParseAdd("application/json");
            message.Content = content;
            var stopwatch = Stopwatch.StartNew();
            var response = await _client.SendAsync(message);
            stopwatch.Stop();
            response.Log(uriPartial, stopwatch.Elapsed.TotalMilliseconds);
            return response;
        }

        public async Task<bool> DownloadFileAsync(Uri uri, string fileName, string? contentType = "application/octet-stream")
        {
            string fileDir = Directory.GetCurrentDirectory();
            string fileNameWithTemp = fileName + ".temp";
            string fullFilePath = Path.Combine(fileDir, fileName);
            string fullFilePathWithTemp = Path.Combine(fileDir, fileNameWithTemp);
            _logger.Information("Start to download file from {Uri} and save to {TempPath}", uri, fullFilePathWithTemp);

            HttpResponseMessage response;
            try
            {
                response = await GetAsync(uri, extraHeader: new Dictionary<string, string> { { "Accept", contentType ?? "application/octet-stream" } }, httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);
                if (response.StatusCode != HttpStatusCode.OK)
                {
                    return false;
                }
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", uri);
                return false;
            }

            var success = true;
            try
            {
                var stream = await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
                await using (var tempFileStream = new FileStream(fullFilePathWithTemp, FileMode.Create, FileAccess.Write))
                {
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
                        tempFileStream.Write(buffer, 0, byteLen);
                        byteLen = await stream.ReadAsync(buffer, 0, buffer.Length);
                    }
                }

                File.Copy(fullFilePathWithTemp, fullFilePath, true);
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to copy file stream {TempFile}", fullFilePathWithTemp);
                success = false;
            }
            finally
            {
                if (File.Exists(fullFilePathWithTemp))
                {
                    _logger.Information("Remove download temp file {TempFile}", fullFilePathWithTemp);
                    File.Delete(fullFilePathWithTemp);
                }
            }

            return success;
        }

        private static WebProxy? GetProxy()
        {
            var proxyIsUri = Uri.TryCreate(Proxy, UriKind.RelativeOrAbsolute, out var uri);
            return (proxyIsUri && (!string.IsNullOrEmpty(Proxy))) is false ? null : new WebProxy(uri);
        }

        private HttpClient BuildHttpClient()
        {
            var handler = new HttpClientHandler
            {
                AutomaticDecompression = DecompressionMethods.All,
                AllowAutoRedirect = true,
            };

            var proxy = GetProxy();
            if (proxy != null)
            {
                _logger.Information("Rebuild HttpClient with proxy {Proxy}", Proxy);
                handler.Proxy = proxy;
                handler.UseProxy = true;
            }

            HttpClient client = new HttpClient(handler);
            client.DefaultRequestHeaders.Add("User-Agent", UserAgent);
            client.Timeout = TimeSpan.FromSeconds(15);
            return client;
        }
    }
}
