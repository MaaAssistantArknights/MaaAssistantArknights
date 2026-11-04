// <copyright file="HttpService.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.Dialogs;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Serilog;

namespace MaaWpfGui.Services.Web;

public class HttpService : IHttpService
{
    private readonly string _userAgent;

    private readonly ILogger _logger = Log.ForContext<HttpService>();
    private readonly Lock _clientLock = new();
    private readonly Dictionary<HttpClient, int> _activeRequests = [];
    private readonly HashSet<HttpClient> _retiredClients = [];
    private HttpClient _client;

    private static readonly TimeSpan _downloadIdleTimeout = TimeSpan.FromSeconds(30);

    public HttpService()
    {
        string uiVersion = Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "0.0.1";
        _userAgent = $"MaaWpfGui/{uiVersion}";
        _client = BuildHttpClient(TryGetProxy(out var proxy) ? proxy : null);

        VersionUpdateSettingsUserControlModel.Instance.PropertyChanged += (sender, args) => {
            if (args.PropertyName != nameof(VersionUpdateSettingsUserControlModel.Proxy) && args.PropertyName != nameof(VersionUpdateSettingsUserControlModel.ProxyType))
            {
                return;
            }
            if (!TryGetProxy(out var updatedProxy))
            {
                _logger.Warning("Proxy is not a valid URI, keeping the current HttpClient");
                return;
            }

            var replacement = BuildHttpClient(updatedProxy);
            HttpClient previous;
            bool disposePrevious;
            lock (_clientLock)
            {
                previous = _client;
                _client = replacement;
                disposePrevious = !_activeRequests.ContainsKey(previous);
                if (!disposePrevious)
                {
                    _retiredClients.Add(previous);
                }
            }

            if (disposePrevious)
            {
                previous.Dispose();
            }
        };
    }

    public async Task<double> HeadAsync(Uri uri, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
    {
        try
        {
            using var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Head, Version = HttpVersion.Version20, };

            if (extraHeader != null)
            {
                foreach (var kvp in extraHeader)
                {
                    // Add 对含 “/” “=” 等字符的值抛 FormatException 且异常消息带原值，凭据类 header 会明文泄漏进日志
                    if (!request.Headers.TryAddWithoutValidation(kvp.Key, kvp.Value))
                    {
                        _logger.Warning("Failed to add external header: {Header}", kvp.Key);
                    }
                }
            }

            request.Headers.ConnectionClose = true;

            var stopwatch = Stopwatch.StartNew();
            using var response = await SendAsync(request, HttpCompletionOption.ResponseContentRead).ConfigureAwait(false);
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
            using var response = await GetAsync(uri, extraHeader, httpCompletionOption);
            if (response.StatusCode != HttpStatusCode.OK)
            {
                return null;
            }

            using var timeout = new CancellationTokenSource(_downloadIdleTimeout);
            return await response.Content.ReadAsStringAsync(timeout.Token);
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

    public async Task<HttpResponseMessage> GetAsync(Uri uri, Dictionary<string, string>? extraHeader = null, HttpCompletionOption httpCompletionOption = HttpCompletionOption.ResponseHeadersRead, UriPartial uriPartial = UriPartial.Query, CancellationToken token = default)
    {
        using var request = new HttpRequestMessage { RequestUri = uri, Method = HttpMethod.Get, Version = HttpVersion.Version20, };
        if (extraHeader != null)
        {
            foreach (var kvp in extraHeader)
            {
                // Add 对含 “/” “=” 等字符的值抛 FormatException 且异常消息带原值，凭据类 header 会明文泄漏进日志
                if (!request.Headers.TryAddWithoutValidation(kvp.Key, kvp.Value))
                {
                    _logger.Warning("Failed to add external header: {Header}", kvp.Key);
                }
            }
        }

        var stopwatch = Stopwatch.StartNew();
        var response = await SendAsync(request, httpCompletionOption, token);
        stopwatch.Stop();
        response.Log(uriPartial, stopwatch.Elapsed.TotalMilliseconds);
        return response;
    }

    public async Task<string?> PostAsJsonAsync<T>(Uri uri, T content, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
    {
        try
        {
            using var requestContent = new StringContent(JsonSerializer.Serialize(content), Encoding.UTF8, "application/json");
            using var response = await PostAsync(uri, requestContent, extraHeader, uriPartial);
            return await response.Content.ReadAsStringAsync();
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send POST request to {Uri}", uri.GetLeftPart(uriPartial));
            return null;
        }
    }

    public async Task<string?> PostAsFormUrlEncodedAsync(Uri uri, Dictionary<string, string?> content, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
    {
        try
        {
            using var requestContent = new FormUrlEncodedContent(content);
            using var response = await PostAsync(uri, requestContent, extraHeader, uriPartial);
            return await response.Content.ReadAsStringAsync();
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send POST request to {Uri}", uri.GetLeftPart(uriPartial));
            return null;
        }
    }

    public async Task<HttpResponseMessage> PostAsync(Uri uri, HttpContent content, Dictionary<string, string>? extraHeader = null, UriPartial uriPartial = UriPartial.Query)
    {
        var message = new HttpRequestMessage(HttpMethod.Post, uri) { Version = HttpVersion.Version20, Content = content };
        try
        {
            if (extraHeader is not null)
            {
                foreach (var header in extraHeader)
                {
                    if (!IsValidHeaderName(header.Key))
                    {
                        _logger.Warning("Skipped external header with invalid name: {Header}", header.Key);
                        continue;
                    }

                    // 内容头（如 Content-Type）不属于请求头集合，Add 会抛 Misused header 异常；
                    // TryAddWithoutValidation 对此类头部返回 false，转而写入 Content.Headers
                    if (message.Headers.TryAddWithoutValidation(header.Key, header.Value))
                    {
                        continue;
                    }

                    // 先移除内容头默认值（如 StringContent 自带的 Content-Type），避免重复发送
                    message.Content.Headers.Remove(header.Key);
                    if (!message.Content.Headers.TryAddWithoutValidation(header.Key, header.Value))
                    {
                        _logger.Warning("Failed to add external header: {Header}", header.Key);
                    }
                }
            }

            message.Headers.Accept.ParseAdd("application/json");
            var stopwatch = Stopwatch.StartNew();
            var response = await SendAsync(message, HttpCompletionOption.ResponseContentRead);
            stopwatch.Stop();
            response.Log(uriPartial, stopwatch.Elapsed.TotalMilliseconds);
            return response;
        }
        finally
        {
            // PostAsync does not own the content. Callers may reuse it for retries or
            // another endpoint, so detach it before disposing the request message.
            message.Content = null;
            message.Dispose();
        }
    }

    /// <summary>
    /// 检查 header 名称是否为 RFC 9110 定义的合法 token。用户误按 JSON 等格式填写时，
    /// 名称会含大括号、引号等分隔符，此时 Remove/Add 等 API 会抛异常，需先行跳过。
    /// </summary>
    private static bool IsValidHeaderName(string name)
    {
        return name.Length > 0 && name.All(c => c is > ' ' and < '\u007f' && !"()<>@,;:\\\"/[]?={}".Contains(c));
    }

    public async Task<bool> DownloadFileAsync(Uri uri, string fileName, string? contentType = "application/octet-stream", CancellationToken token = default)
    {
        var result = await DownloadFileWithResultAsync(uri, fileName, contentType, token: token).ConfigureAwait(false);
        return result.Success;
    }

    public async Task<HttpDownloadResult> DownloadFileWithResultAsync(Uri uri, string fileName, string? contentType = "application/octet-stream", Dictionary<string, string>? extraHeader = null, CancellationToken token = default)
    {
        string fileDir = PathsHelper.BaseDir;
        string fileNameWithTemp = fileName + ".temp";
        string fullFilePath = Path.Combine(fileDir, fileName);
        string fullFilePathWithTemp = Path.Combine(fileDir, fileNameWithTemp);
        _logger.Information("Start to download file from {Uri} and save to {TempPath}", uri, fullFilePathWithTemp);

        HttpStatusCode? statusCode = null;
        try
        {
            var headers = extraHeader == null
                ? new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
                : new Dictionary<string, string>(extraHeader, StringComparer.OrdinalIgnoreCase);
            headers["Accept"] = contentType ?? "application/octet-stream";
            using var response = await GetAsync(uri, extraHeader: headers, httpCompletionOption: HttpCompletionOption.ResponseHeadersRead, token: token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            statusCode = response.StatusCode;
            if (response.StatusCode != HttpStatusCode.OK)
            {
                var (retryAfter, isRateLimited) = await GetRetryInfoAsync(response, token).ConfigureAwait(false);
                return new(false, statusCode, retryAfter, isRateLimited);
            }

            using var idleTimeout = CancellationTokenSource.CreateLinkedTokenSource(token);
            idleTimeout.CancelAfter(_downloadIdleTimeout);
            await using var stream = await response.Content.ReadAsStreamAsync(idleTimeout.Token).ConfigureAwait(false);
            idleTimeout.CancelAfter(Timeout.InfiniteTimeSpan);
            await using (var tempFileStream = new FileStream(fullFilePathWithTemp, FileMode.Create, FileAccess.Write, FileShare.None, 81920, useAsync: true))
            {
                // 记录初始化
                long value = 0;
                int valueInOneSecond = 0;
                long fileMaximum = response.Content.Headers.ContentLength ?? 1;
                DateTime beforeDt = DateTime.Now;

                // Dangerous action
                VersionUpdateDialogViewModel.OutputDownloadProgress();

                byte[] buffer = new byte[81920];
                while (true)
                {
                    // Reset for each network read: large files may take much longer than
                    // the idle timeout as long as the server continues sending data.
                    idleTimeout.CancelAfter(_downloadIdleTimeout);
                    int byteLen = await stream.ReadAsync(buffer, idleTimeout.Token).ConfigureAwait(false);
                    idleTimeout.CancelAfter(Timeout.InfiniteTimeSpan);
                    if (byteLen == 0)
                    {
                        break;
                    }

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
                    await tempFileStream.WriteAsync(buffer.AsMemory(0, byteLen), token).ConfigureAwait(false);
                }
            }

            token.ThrowIfCancellationRequested();
            File.Copy(fullFilePathWithTemp, fullFilePath, true);
            return new(true, statusCode);
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            throw;
        }
        catch (OperationCanceledException)
        {
            _logger.Warning("File download timed out for {Uri}", uri.GetLeftPart(UriPartial.Path));
            return new(false, statusCode);
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to download file from {Uri}", uri.GetLeftPart(UriPartial.Path));
            return new(false, statusCode);
        }
        finally
        {
            if (File.Exists(fullFilePathWithTemp))
            {
                try
                {
                    _logger.Information("Remove download temp file {TempFile}", fullFilePathWithTemp);
                    File.Delete(fullFilePathWithTemp);
                }
                catch (Exception e)
                {
                    _logger.Warning(e, "Failed to remove download temp file {TempFile}", fullFilePathWithTemp);
                }
            }
        }
    }

    internal static async Task<(TimeSpan? RetryAfter, bool IsRateLimited)> GetRetryInfoAsync(HttpResponseMessage response, CancellationToken cancellationToken = default)
    {
        if (response.IsSuccessStatusCode)
        {
            return (null, false);
        }

        var now = DateTimeOffset.UtcNow;
        var delay = response.Headers.RetryAfter?.Delta ?? (response.Headers.RetryAfter?.Date - now);
        bool exhausted = response.Headers.TryGetValues("X-RateLimit-Remaining", out var remaining) &&
            remaining.Any(value => long.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out long count) && count == 0);
        if (exhausted &&
            response.Headers.TryGetValues("X-RateLimit-Reset", out var resetValues) &&
            long.TryParse(resetValues.FirstOrDefault(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var seconds) &&
            seconds >= DateTimeOffset.MinValue.ToUnixTimeSeconds() && seconds <= DateTimeOffset.MaxValue.ToUnixTimeSeconds())
        {
            var resetDelay = DateTimeOffset.FromUnixTimeSeconds(seconds) - now;
            if (!delay.HasValue || resetDelay > delay.Value)
            {
                delay = resetDelay;
            }
        }

        bool rateLimited = response.StatusCode == HttpStatusCode.TooManyRequests ||
            (response.StatusCode == HttpStatusCode.Forbidden && (exhausted || delay.HasValue));
        if (response.StatusCode == HttpStatusCode.Forbidden && !rateLimited)
        {
            // GitHub secondary limits can be reported only in a 403 response body.
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeout.CancelAfter(_downloadIdleTimeout);
            try
            {
                var body = await response.Content.ReadAsStringAsync(timeout.Token).ConfigureAwait(false);
                rateLimited = body.Contains("rate limit", StringComparison.OrdinalIgnoreCase);
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                // Failure to read an error body must not discard the known HTTP status.
            }
            catch (HttpRequestException)
            {
            }
        }

        delay ??= rateLimited ? TimeSpan.FromMinutes(1) : null;
        return (delay is { } value && value < TimeSpan.Zero ? TimeSpan.Zero : delay, rateLimited);
    }

    private static bool TryGetProxy(out WebProxy? proxy)
    {
        proxy = null;
        string address = SettingsViewModel.VersionUpdateSettings.Proxy?.Trim() ?? string.Empty;
        if (string.IsNullOrEmpty(address))
        {
            return true;
        }

        if (!address.Contains("://"))
        {
            address = SettingsViewModel.VersionUpdateSettings.ProxyType + "://" + address;
        }

        if (!Uri.TryCreate(address, UriKind.Absolute, out var uri) || !uri.IsWellFormedOriginalString() ||
            uri.Scheme is not ("http" or "https" or "socks4" or "socks4a" or "socks5") ||
            string.IsNullOrEmpty(uri.Host) || uri.Port <= 0 || address.EndsWith(':') ||
            uri.AbsolutePath is not ("" or "/") || !string.IsNullOrEmpty(uri.Query) || !string.IsNullOrEmpty(uri.Fragment))
        {
            return false;
        }

        proxy = new WebProxy(uri);
        return true;
    }

    private async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, HttpCompletionOption completionOption, CancellationToken token = default)
    {
        HttpClient client;
        lock (_clientLock)
        {
            client = _client;
            _activeRequests.TryGetValue(client, out int active);
            _activeRequests[client] = active + 1;
        }

        try
        {
            return await client.SendAsync(request, completionOption, token).ConfigureAwait(false);
        }
        finally
        {
            bool disposeClient = false;
            lock (_clientLock)
            {
                int active = _activeRequests[client] - 1;
                if (active == 0)
                {
                    _activeRequests.Remove(client);
                    disposeClient = _retiredClients.Remove(client);
                }
                else
                {
                    _activeRequests[client] = active;
                }
            }

            if (disposeClient)
            {
                // Disposing the connection pool closes idle connections. Response streams
                // already returned to callers keep their active connections until closed.
                client.Dispose();
            }
        }
    }

    private HttpClient BuildHttpClient(WebProxy? proxy)
    {
        var handler = new HttpClientHandler {
            AutomaticDecompression = DecompressionMethods.All,
            AllowAutoRedirect = true,
        };

        if (proxy != null)
        {
            _logger.Information("Rebuild HttpClient with proxy {Proxy}", proxy.Address?.GetComponents(UriComponents.SchemeAndServer, UriFormat.SafeUnescaped));
            handler.Proxy = proxy;
            handler.UseProxy = true;
        }

        HttpClient client = new HttpClient(handler);
        client.DefaultRequestHeaders.Add("User-Agent", _userAgent);
        client.Timeout = TimeSpan.FromSeconds(15);
        return client;
    }
}
