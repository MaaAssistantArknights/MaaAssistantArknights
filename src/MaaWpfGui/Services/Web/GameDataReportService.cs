// <copyright file="GameDataReportService.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Services.Web
{
    public class GameDataReportService
    {
        private static readonly ILogger _logger = Log.ForContext<GameDataReportService>();
        private const int MaxRetryPerDomain = 3;
        private const int InitialBackoffMs = 3000;

        public static async Task<bool> PostWithRetryAsync(string url, HttpContent content, Dictionary<string, string> headers, string subtask, Action<string>? callback = null)
        {
            _logger.Information("Start PostWithRetryAsync, url: {Url}", url);
            _logger.Information("Request headers: {@Headers}", headers);
            var body = await content.ReadAsStringAsync();
            _logger.Information("Request body: {Body}", body);

            var originalResponse = await TryPostAsync(url, content, headers);
            if (originalResponse != null && (int)originalResponse.StatusCode == 200)
            {
                await ProcessResponseIdAsync(originalResponse, callback);
                return true;
            }

            _logger.Warning("Initial request failed, status code: {StatusCode}", originalResponse?.StatusCode);

            if (subtask != "ReportToPenguinStats" || !url.Contains(MaaUrls.PenguinIoDomain))
            {
                // 一图流的上报失败不提示
                return true;
            }

            foreach (var backupDomain in MaaUrls.PenguinBackupDomains)
            {
                var newUrl = url.Replace(MaaUrls.PenguinIoDomain, backupDomain);
                _logger.Information("Trying backup domain {BackupDomain}, url: {NewUrl}", backupDomain, newUrl);
                var resp = await TryPostAsync(newUrl, content, headers);
                if (resp != null && (int)resp.StatusCode == 200)
                {
                    await ProcessResponseIdAsync(resp, callback);
                    return true;
                }

                _logger.Warning("Request failed on backup domain {BackupDomain}, status code: {StatusCode}", backupDomain, resp?.StatusCode);
            }

            _logger.Error("All retries failed for URL: {Url}", url);
            return false;
        }

        private static async Task<HttpResponseMessage?> TryPostAsync(string targetUrl, HttpContent content, Dictionary<string, string> headers)
        {
            try
            {
                int currentBackoff = InitialBackoffMs;
                for (int attempt = 1; attempt <= MaxRetryPerDomain; attempt++)
                {
                    var response = await Instances.HttpService.PostAsync(new(targetUrl), content, headers);

                    switch ((int)response.StatusCode)
                    {
                        case 200:
                            return response;
                        case >= 500 and < 600:
                            await Task.Delay(currentBackoff);
                            currentBackoff = (int)(currentBackoff * 1.5);
                            continue;
                        default:
                            return response;
                    }
                }
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Error during HTTP POST request to {TargetUrl}", targetUrl);
            }

            return null;
        }

        private static Task ProcessResponseIdAsync(HttpResponseMessage response, Action<string>? callback)
        {
            if (callback is null || !response.Headers.TryGetValues("x-penguin-set-penguinid", out var values))
            {
                return Task.CompletedTask;
            }

            var penguinId = values.FirstOrDefault();
            if (!string.IsNullOrEmpty(penguinId))
            {
                callback.Invoke(penguinId);
            }

            return Task.CompletedTask;
        }
    }
}
