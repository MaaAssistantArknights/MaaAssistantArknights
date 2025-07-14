// <copyright file="ETagCache.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.Helper
{
    public class ETagCache
    {
        private static readonly ILogger _logger = Log.ForContext<ETagCache>();

        private static readonly string _etagFile = Path.Combine(Environment.CurrentDirectory, "cache/etag.json");
        private static readonly string _lastModifiedFile = Path.Combine(Environment.CurrentDirectory, "cache/last_modified.json");
        private static Dictionary<string, string> _etagCache = [];
        private static Dictionary<string, DateTimeOffset> _lastModifiedCache = [];

        public static void Load()
        {
            // ETag
            if (File.Exists(_etagFile))
            {
                try
                {
                    var json = File.ReadAllText(_etagFile);
                    _etagCache = JsonConvert.DeserializeObject<Dictionary<string, string>>(json) ?? [];
                }
                catch (Exception e)
                {
                    _logger.Warning(e, "Failed to load ETag cache");
                    _etagCache = [];
                }
            }

            // Last-Modified
            if (File.Exists(_lastModifiedFile))
            {
                try
                {
                    var json = File.ReadAllText(_lastModifiedFile);
                    _lastModifiedCache = JsonConvert.DeserializeObject<Dictionary<string, DateTimeOffset>>(json) ?? [];
                }
                catch (Exception e)
                {
                    _logger.Warning(e, "Failed to load Last-Modified cache");
                    _lastModifiedCache = [];
                }
            }
        }

        public static void Save()
        {
            File.WriteAllText(_etagFile, JsonConvert.SerializeObject(_etagCache, Formatting.Indented));
            File.WriteAllText(_lastModifiedFile, JsonConvert.SerializeObject(_lastModifiedCache, Formatting.Indented));
        }

        public static string GetETag(string? url) =>
            url != null && _etagCache.TryGetValue(url, out var etag) ? etag : string.Empty;

        public static DateTimeOffset? GetLastModified(string? url) =>
            url != null && _lastModifiedCache.TryGetValue(url, out var lm) ? lm : null;

        public static void SetETag(string url, string etag)
        {
            _etagCache[url] = etag;
        }

        public static void SetLastModified(string url, DateTimeOffset? dt)
        {
            if (dt.HasValue)
            {
                _lastModifiedCache[url] = dt.Value;
            }
        }

        // UPDATE: 重定向会导致 uri 变成其他地址，导致存的 ETag 无法匹配原始地址，所以要传入原始地址
        public static void Set(HttpResponseMessage? response, string uri)
        {
            if (response == null || string.IsNullOrEmpty(uri))
            {
                return;
            }

            var etag = response.Headers.ETag?.Tag;
            var lastModified = response.Content?.Headers?.LastModified;

            if (!string.IsNullOrEmpty(etag))
            {
                SetETag(uri, etag);
            }

            if (lastModified.HasValue)
            {
                SetLastModified(uri, lastModified.Value);
            }

            Save();
        }

        public static async Task<HttpResponseMessage?> FetchResponseWithEtag(string url, bool force = false)
        {
            var headers = new Dictionary<string, string>
            {
                { "Accept", "application/octet-stream" },
                { "Connection", "close" },
            };

            if (!force)
            {
                var etag = GetETag(url);
                var lastModified = GetLastModified(url);

                if (!string.IsNullOrEmpty(etag))
                {
                    headers["If-None-Match"] = etag;
                }

                if (lastModified.HasValue)
                {
                    headers["If-Modified-Since"] = lastModified.Value.ToUniversalTime().ToString("R");
                }
            }

            try
            {
                var response = await Instances.HttpService.GetAsync(new Uri(url), headers);
                return response;
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to send GET request to {Uri}", url);
                return null;
            }
        }
    }
}
