// <copyright file="ETagCache.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.Helper
{
    public class ETagCache
    {
        private static readonly ILogger _logger = Log.ForContext<ETagCache>();

        private static readonly string _cacheFile = Path.Combine(Environment.CurrentDirectory, "cache/etag.json");
        private static Dictionary<string, string> _cache;

        public static void Load()
        {
            if (File.Exists(_cacheFile) is false)
            {
                _cache = new Dictionary<string, string>();
                return;
            }

            try
            {
                var jsonStr = File.ReadAllText(_cacheFile);
                _cache = JsonConvert.DeserializeObject<Dictionary<string, string>>(jsonStr);
            }
            catch (Exception e)
            {
                _logger.Error(e.Message);
            }
            finally
            {
                _cache ??= new Dictionary<string, string>();
            }
        }

        public static void Save()
        {
            var jsonStr = JsonConvert.SerializeObject(_cache);
            File.WriteAllText(_cacheFile, jsonStr);
        }

        public static string Get(string url)
        {
            if (url is null)
            {
                return string.Empty;
            }

            return _cache.TryGetValue(url, out string ret) ? ret : string.Empty;
        }

        public static void Set(string url, string etag)
        {
            _cache[url] = etag;
        }
    }
}
