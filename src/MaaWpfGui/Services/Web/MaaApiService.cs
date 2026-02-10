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

    public async Task<JObject?> RequestMaaApiWithCache(string api, bool allowFallbackToCache = true)
    {
        return await RequestWithFallback(api, MaaUrls.MaaApi, MaaUrls.MaaApi2, allowFallbackToCache);
    }

    private async Task<JObject?> RequestWithFallback(string api, string primaryBaseUrl, string? fallbackBaseUrl = null, bool allowFallbackToCache = true)
    {
        var json = await TryRequest(api, primaryBaseUrl, allowFallbackToCache);
        if (json != null || string.IsNullOrEmpty(fallbackBaseUrl))
        {
            return json;
        }

        return await TryRequest(api, fallbackBaseUrl, allowFallbackToCache);
    }

    private async Task<JObject?> TryRequest(string api, string baseUrl, bool allowFallbackToCache = true)
    {
        var url = baseUrl + api;
        var cache = Path.Combine(CacheDir, api);

        var response = await ETagCache.FetchResponseWithEtag(url, !File.Exists(cache));
        if (response == null)
        {
            return allowFallbackToCache ? LoadApiCache(api) : null;
        }

        if (response.StatusCode == System.Net.HttpStatusCode.NotModified)
        {
            return LoadApiCache(api);
        }

        if (response.StatusCode != System.Net.HttpStatusCode.OK)
        {
            return allowFallbackToCache ? LoadApiCache(api) : null;
        }

        var body = await HttpResponseHelper.GetStringAsync(response);
        if (string.IsNullOrEmpty(body))
        {
            return LoadApiCache(api);
        }

        try
        {
            var json = (JObject?)JsonConvert.DeserializeObject(body);
            string? directoryPath = Path.GetDirectoryName(cache);

            if (!Directory.Exists(directoryPath))
            {
                Directory.CreateDirectory(directoryPath!);
            }

            await File.WriteAllTextAsync(cache, body);
            ETagCache.Set(response, url);

            return json;
        }
        catch
        {
            return null;
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
