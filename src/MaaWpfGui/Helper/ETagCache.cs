// <copyright file="ETagCache.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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

        private static readonly string _cacheFile = Path.Combine(Environment.CurrentDirectory, "cache/etag.json");
        private static Dictionary<string, string> _cache = [];

        public static void Load()
        {
            if (File.Exists(_cacheFile) is false)
            {
                _cache = [];
                return;
            }

            try
            {
                var jsonStr = File.ReadAllText(_cacheFile);
                _cache = JsonConvert.DeserializeObject<Dictionary<string, string>>(jsonStr) ?? [];
            }
            catch (Exception e)
            {
                _logger.Error(e.Message);
            }
        }

        public static void Save()
        {
            var jsonStr = JsonConvert.SerializeObject(_cache);
            File.WriteAllText(_cacheFile, jsonStr);
        }

        // ReSharper disable once MemberCanBePrivate.Global
        public static string Get(string? url)
        {
            if (url is null)
            {
                return string.Empty;
            }

            return _cache.TryGetValue(url, out string? ret) ? ret : string.Empty;
        }

        // ReSharper disable once MemberCanBePrivate.Global
        public static void Set(string url, string etag)
        {
            _cache[url] = etag;
            Save();
        }

        public static void Set(HttpResponseMessage? response)
        {
            var etag = response?.Headers.ETag?.Tag;
            var uri = response?.RequestMessage?.RequestUri?.ToString();
            if (string.IsNullOrEmpty(uri) || string.IsNullOrEmpty(etag))
            {
                return;
            }

            Set(uri, etag);
        }

        public static async Task<HttpResponseMessage?> FetchResponseWithEtag(string url, bool force = false)
        {
            var etag = force ? string.Empty : Get(url);
            Dictionary<string, string> headers = new Dictionary<string, string>
            {
                { "Accept", "application/octet-stream" },
                { "Connection", "close" },
            };

            if (!string.IsNullOrEmpty(etag))
            {
                headers["If-None-Match"] = etag;
            }

            var response = await Instances.HttpService.GetAsync(new Uri(url), headers, httpCompletionOption: HttpCompletionOption.ResponseHeadersRead);
            return response;
        }
    }
}
