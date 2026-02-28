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
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Web;

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
            // Replace GitHub domains if needed
            uri = GithubUrlHelper.ReplaceGithubDomain(uri);

            var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Head, Version = HttpVersion.Version20, };

            // Add GitHub token if the request is to GitHub
            AddGithubTokenIfNeeded(request, extraHeader);

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
        // Replace GitHub domains if needed
        uri = GithubUrlHelper.ReplaceGithubDomain(uri);

        var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Get, Version = HttpVersion.Version20, };

        // Add GitHub token if the request is to GitHub
        AddGithubTokenIfNeeded(request, extraHeader);

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
        // Replace GitHub domains if needed
        uri = GithubUrlHelper.ReplaceGithubDomain(uri);

        var message = new HttpRequestMessage(HttpMethod.Post, uri) { Version = HttpVersion.Version20 };

        // Add GitHub token if the request is to GitHub
        AddGithubTokenIfNeeded(message, extraHeader);

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
        // Replace GitHub domains if needed
        uri = GithubUrlHelper.ReplaceGithubDomain(uri);

        string fileDir = PathsHelper.BaseDir;
        string fileNameWithTemp = fileName + ".temp";
        string fullFilePath = Path.Combine(fileDir, fileName);
        string fullFilePathWithTemp = Path.Combine(fileDir, fileNameWithTemp);
        _logger.Information("Start to download file from {Uri} and save to {TempPath}", uri, fullFilePathWithTemp);

        HttpResponseMessage response;
        try
        {
            var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Head, Version = HttpVersion.Version20, };
            request.Headers.Add("Accept", contentType ?? "application/octet-stream");
            var headResponse = await _client.SendAsync(request);

            bool supportsRangeRequests = headResponse.Headers.AcceptRanges?.Contains("bytes") ?? false;
            long? contentLength = headResponse.Content.Headers.ContentLength;

            if (supportsRangeRequests && contentLength.HasValue && contentLength.Value > 10 * 1024 * 1024) // > 10MB
            {
                return await DownloadFileWithMultiThreadAsync(uri, fullFilePath, fullFilePathWithTemp, contentLength.Value, contentType);
            }

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
                VersionUpdateDialogViewModel.OutputDownloadProgress();

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
                        VersionUpdateDialogViewModel.OutputDownloadProgress(value, fileMaximum, valueInOneSecond, ts);
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

    private async Task<bool> DownloadFileWithMultiThreadAsync(Uri uri, string fullFilePath, string fullFilePathWithTemp, long fileSize, string? contentType)
    {
        _logger.Information("Using dynamic multi-threaded download for {Uri}, file size: {FileSize} bytes", uri, fileSize);

        // 动态线程管理：初始线程数为2，范围1-64，根据实时下载速度动态调整
        int currentThreadCount = 2;
        const int minThreads = 1;
        const int maxThreads = 64;

        var progressLock = new object();
        long totalBytesDownloaded = 0;
        long previousBytesDownloaded = 0;
        DateTime lastSpeedCheck = DateTime.Now;
        DateTime lastAdjustmentTime = DateTime.Now;
        double currentSpeed = 0; // bytes per second
        double previousSpeed = 0;

        var activeThreads = new Dictionary<int, DownloadThreadInfo>();
        var completedChunks = new HashSet<int>();
        int nextThreadId = 0;

        try
        {
            // 初始化进度显示
            VersionUpdateDialogViewModel.OutputDownloadProgress();

            // 启动初始线程
            for (int i = 0; i < currentThreadCount; i++)
            {
                StartNewThread(i);
            }

            // 动态监控和调整线程
            while (true)
            {
                await Task.Delay(1000); // 每秒检查一次

                List<KeyValuePair<int, DownloadThreadInfo>> completedThreads;
                bool shouldExit = false;
                bool shouldUpdate = false;
                int threadsToStart = 0;

                lock (progressLock)
                {
                    // 计算当前下载速度
                    double elapsed = DateTime.Now.Subtract(lastSpeedCheck).TotalSeconds;
                    if (elapsed >= 1.0)
                    {
                        long bytesDownloadedSinceLastCheck = totalBytesDownloaded - previousBytesDownloaded;
                        currentSpeed = bytesDownloadedSinceLastCheck / elapsed;
                        previousBytesDownloaded = totalBytesDownloaded;
                        lastSpeedCheck = DateTime.Now;
                        shouldUpdate = true;

                        // 动态调整线程数（每3秒调整一次）
                        double timeSinceLastAdjustment = DateTime.Now.Subtract(lastAdjustmentTime).TotalSeconds;
                        if (timeSinceLastAdjustment >= 3.0)
                        {
                            AdjustThreadCount();
                            lastAdjustmentTime = DateTime.Now;
                        }
                    }

                    // 检查是否所有线程都已完成
                    if (activeThreads.Count == 0 && totalBytesDownloaded >= fileSize)
                    {
                        shouldExit = true;
                    }

                    // 清理已完成的线程
                    completedThreads = activeThreads.Where(t => t.Value.Task.IsCompleted).ToList();

                    // 计算需要启动的新线程数
                    if (totalBytesDownloaded < fileSize && activeThreads.Count - completedThreads.Count < currentThreadCount)
                    {
                        threadsToStart = currentThreadCount - (activeThreads.Count - completedThreads.Count);
                    }
                }

                // 更新进度显示（在锁外）
                if (shouldUpdate)
                {
                    long bytesDownloadedSinceLastCheck = totalBytesDownloaded - previousBytesDownloaded;
                    double elapsed = DateTime.Now.Subtract(lastSpeedCheck).TotalSeconds;
                    VersionUpdateDialogViewModel.OutputDownloadProgress(
                        totalBytesDownloaded,
                        fileSize,
                        (int)bytesDownloadedSinceLastCheck,
                        elapsed);
                }

                // 处理已完成的线程（在锁外）
                foreach (var thread in completedThreads)
                {
                    var result = await thread.Value.Task;

                    lock (progressLock)
                    {
                        if (result.success)
                        {
                            completedChunks.Add(thread.Key);
                            _logger.Information("Thread {ThreadId} completed successfully", thread.Key);
                        }
                        else
                        {
                            _logger.Error("Thread {ThreadId} failed", thread.Key);
                            return false;
                        }

                        activeThreads.Remove(thread.Key);
                    }
                }

                // 启动新线程（在锁外）
                for (int i = 0; i < threadsToStart; i++)
                {
                    lock (progressLock)
                    {
                        if (totalBytesDownloaded < fileSize)
                        {
                            StartNewThread(nextThreadId++);
                        }
                    }
                }

                if (shouldExit)
                {
                    break;
                }
            }

            // 合并所有下载的块
            _logger.Information("Merging {Count} chunk files", completedChunks.Count);
            await using (var outputStream = new FileStream(fullFilePathWithTemp, FileMode.Create, FileAccess.Write))
            {
                foreach (var chunkId in completedChunks.OrderBy(c => c))
                {
                    string chunkFile = $"{fullFilePathWithTemp}.part{chunkId}";
                    if (File.Exists(chunkFile))
                    {
                        await using (var chunkStream = new FileStream(chunkFile, FileMode.Open, FileAccess.Read))
                        {
                            await chunkStream.CopyToAsync(outputStream);
                        }
                    }
                }
            }

            File.Copy(fullFilePathWithTemp, fullFilePath, true);
            _logger.Information("Dynamic multi-threaded download completed successfully with {ThreadCount} total threads used", nextThreadId);
            return true;
        }
        catch (Exception e)
        {
            _logger.Error(e, "Dynamic multi-threaded download failed");
            return false;
        }
        finally
        {
            // 清理临时文件
            if (File.Exists(fullFilePathWithTemp))
            {
                File.Delete(fullFilePathWithTemp);
            }

            for (int i = 0; i < nextThreadId; i++)
            {
                string chunkFile = $"{fullFilePathWithTemp}.part{i}";
                if (File.Exists(chunkFile))
                {
                    File.Delete(chunkFile);
                }
            }
        }

        void StartNewThread(int threadId)
        {
            lock (progressLock)
            {
                // 计算此线程应下载的范围
                long chunkSize = Math.Max(1024 * 1024, fileSize / 100); // 至少1MB，或文件的1%
                long start = totalBytesDownloaded;
                long end = Math.Min(start + chunkSize - 1, fileSize - 1);

                if (start >= fileSize)
                {
                    return; // 没有更多数据需要下载
                }

                string chunkFile = $"{fullFilePathWithTemp}.part{threadId}";
                totalBytesDownloaded = end + 1; // 预留此范围

                var task = Task.Run(async () =>
                {
                    long chunkBytesDownloaded = 0;
                    try
                    {
                        var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Get, Version = HttpVersion.Version20 };
                        request.Headers.Range = new System.Net.Http.Headers.RangeHeaderValue(start, end);
                        request.Headers.Add("Accept", contentType ?? "application/octet-stream");

                        var response = await _client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead);
                        if (!response.IsSuccessStatusCode)
                        {
                            _logger.Error("Thread {ThreadId} failed with status {StatusCode}", threadId, response.StatusCode);
                            return (false, 0L);
                        }

                        var stream = await response.Content.ReadAsStreamAsync();
                        await using (var fileStream = new FileStream(chunkFile, FileMode.Create, FileAccess.Write))
                        {
                            byte[] buffer = new byte[81920];
                            int bytesRead;

                            while ((bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length)) > 0)
                            {
                                await fileStream.WriteAsync(buffer, 0, bytesRead);
                                chunkBytesDownloaded += bytesRead;
                            }
                        }

                        return (true, chunkBytesDownloaded);
                    }
                    catch (Exception e)
                    {
                        _logger.Error(e, "Thread {ThreadId} failed with exception", threadId);
                        return (false, 0L);
                    }
                });

                activeThreads[threadId] = new DownloadThreadInfo { Task = task, StartByte = start, EndByte = end };
                _logger.Information("Started thread {ThreadId} for range {Start}-{End} (target threads: {CurrentThreadCount})", threadId, start, end, currentThreadCount);
            }
        }

        void AdjustThreadCount()
        {
            // 根据当前速度调整线程数：速度慢时增加线程，速度快时减少线程
            // 目标：解决中国大陆直连GitHub速度慢的问题，通过多线程提升慢速连接的速度

            // 定义速度阈值：512 KB/s 作为"慢速"的判断标准
            const double slowSpeedThreshold = 512 * 1024; // 512 KB/s
            const double goodSpeedThreshold = 2 * 1024 * 1024; // 2 MB/s

            if (currentSpeed < slowSpeedThreshold && currentThreadCount < maxThreads)
            {
                // 速度过慢，增加线程以尝试提升速度
                int increaseAmount = currentSpeed < slowSpeedThreshold / 2 ? 4 : 2; // 非常慢时增加更多
                currentThreadCount = Math.Min(maxThreads, currentThreadCount + increaseAmount);
                _logger.Information("Speed is slow ({Speed:F2} KB/s < {Threshold:F2} KB/s), increasing threads to {ThreadCount}",
                    currentSpeed / 1024, slowSpeedThreshold / 1024, currentThreadCount);
            }
            else if (currentSpeed >= goodSpeedThreshold && currentThreadCount > minThreads)
            {
                // 速度良好，减少线程以避免不必要的资源消耗
                currentThreadCount = Math.Max(minThreads, currentThreadCount - 1);
                _logger.Information("Speed is good ({Speed:F2} MB/s >= {Threshold:F2} MB/s), decreasing threads to {ThreadCount}",
                    currentSpeed / (1024 * 1024), goodSpeedThreshold / (1024 * 1024), currentThreadCount);
            }
            else if (currentSpeed >= slowSpeedThreshold && currentSpeed < goodSpeedThreshold)
            {
                // 速度中等，根据趋势微调
                if (previousSpeed > 0)
                {
                    double speedRatio = currentSpeed / previousSpeed;

                    if (speedRatio < 0.7 && currentThreadCount < maxThreads)
                    {
                        // 速度明显下降，尝试增加线程
                        currentThreadCount = Math.Min(maxThreads, currentThreadCount + 2);
                        _logger.Information("Speed dropping (ratio: {SpeedRatio:F2}), increasing threads to {ThreadCount}",
                            speedRatio, currentThreadCount);
                    }
                    else if (speedRatio > 1.5 && currentThreadCount > minThreads)
                    {
                        // 速度显著提升，可以减少一些线程
                        currentThreadCount = Math.Max(minThreads, currentThreadCount - 1);
                        _logger.Information("Speed improving (ratio: {SpeedRatio:F2}), decreasing threads to {ThreadCount}",
                            speedRatio, currentThreadCount);
                    }
                }
            }

            previousSpeed = currentSpeed;
        }
    }

    private class DownloadThreadInfo
    {
        public Task<(bool success, long bytesDownloaded)> Task { get; set; } = null!;
        public long StartByte { get; set; }
        public long EndByte { get; set; }
    }

    private static WebProxy? GetProxy()
    {
        var proxyIsUri = Uri.TryCreate(Proxy, UriKind.RelativeOrAbsolute, out var uri);
        return (proxyIsUri && (!string.IsNullOrEmpty(Proxy))) is false ? null : new WebProxy(uri);
    }

    private void AddGithubTokenIfNeeded(HttpRequestMessage request, Dictionary<string, string>? extraHeader)
    {
        if (request.RequestUri == null)
        {
            return;
        }

        // Check if this is a GitHub request
        var host = request.RequestUri.Host.ToLower();
        bool isGithubRequest = host.Contains("github.com") || host.Contains("githubusercontent.com");

        // If extraHeader already contains Authorization, don't override
        if (extraHeader?.ContainsKey("Authorization") == true)
        {
            return;
        }

        // If it's a GitHub request and user has configured a token, add it
        if (isGithubRequest)
        {
            var token = SettingsViewModel.VersionUpdateSettings.GithubToken;
            if (!string.IsNullOrEmpty(token))
            {
                request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
                _logger.Information("Added GitHub token to request for {Host}", host);
            }
        }
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
