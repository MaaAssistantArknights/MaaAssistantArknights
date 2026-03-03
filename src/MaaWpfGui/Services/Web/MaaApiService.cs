// <copyright file="MaaApiService.cs" company="MaaAssistantArknights">
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

using System.IO;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Services.Web;

public class MaaApiService : IMaaApiService
{
    private static readonly string CacheDir = PathsHelper.CacheDir;

    public async Task<(bool Cached, JObject? Response)> RequestMaaApiWithCache(string api, bool allowFallbackToCache = true)
    {
        return await RequestWithFallback(api, MaaUrls.MaaApi, MaaUrls.MaaApi2, allowFallbackToCache);
    }

    private async Task<(bool Cached, JObject? Response)> RequestWithFallback(string api, string primaryBaseUrl, string? fallbackBaseUrl = null, bool allowFallbackToCache = true)
    {
        var result = await TryRequest(api, primaryBaseUrl, allowFallbackToCache);
        if (result.Response != null || string.IsNullOrEmpty(fallbackBaseUrl))
        {
            return result;
        }

        return await TryRequest(api, fallbackBaseUrl, allowFallbackToCache);
    }

    private async Task<(bool Cached, JObject? Response)> TryRequest(string api, string baseUrl, bool allowFallbackToCache = true)
    {
        var url = baseUrl + api;
        var cachePath = Path.Combine(CacheDir, api);
        var response = await ETagCache.FetchResponseWithEtag(url, !File.Exists(cachePath));
        if (response?.StatusCode == System.Net.HttpStatusCode.NotModified)
        {
            var cache = LoadApiCache(api);
            return (cache != null, cache);
        }

        if (response == null || response.StatusCode != System.Net.HttpStatusCode.OK)
        {
            if (!allowFallbackToCache)
            {
                return (false, null);
            }

            var cache = LoadApiCache(api);
            return (cache != null, cache);
        }

        var body = await HttpResponseHelper.GetStringAsync(response);
        if (string.IsNullOrEmpty(body))
        {
            var cache = LoadApiCache(api);
            return (cache != null, cache);
        }

        try
        {
            var json = (JObject?)JsonConvert.DeserializeObject(body);
            string? directoryPath = Path.GetDirectoryName(cachePath);

            if (!Directory.Exists(directoryPath))
            {
                Directory.CreateDirectory(directoryPath!);
            }

            await File.WriteAllTextAsync(cachePath, body);
            ETagCache.Set(response, url);

            return (false, json);
        }
        catch
        {
            return (false, null);
        }
    }

    public JObject? LoadApiCache(string api)
    {
        var cache = Path.Combine(CacheDir, api);
        if (!File.Exists(cache))
        {
            return null;
        }

        try
        {
            return (JObject?)JsonConvert.DeserializeObject(File.ReadAllText(cache));
        }
        catch
        {
            return null;
        }
    }
}
