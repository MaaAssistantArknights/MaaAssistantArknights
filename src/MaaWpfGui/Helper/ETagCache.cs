using System;
using System.Collections.Generic;
using System.IO;

namespace MaaWpfGui.Helper
{
    public class ETagCache
    {
        private static readonly string _cacheFile = Path.Combine(Environment.CurrentDirectory, "cache/etag.json");
        private static Dictionary<string, string> _cache;

        public ETagCache() {
            Load();
        }

        private static void Load()
        {
            if (File.Exists(_cacheFile) is false)
            {
                _cache = new Dictionary<string, string>();
                return;
            }

            var jsonStr = File.ReadAllText(_cacheFile);
            _cache = System.Text.Json.JsonSerializer.Deserialize<Dictionary<string, string>>(jsonStr);
        }

        public static void Save()
        {
            var jsonStr = System.Text.Json.JsonSerializer.Serialize(_cache);
            File.WriteAllText(_cacheFile, jsonStr);
        }

        public static string Get(string url)
        {
            if (_cache.ContainsKey(url))
            {
                return _cache[url];
            }

            return string.Empty;
        }

        public static void Set(string url, string etag)
        {
            _cache[url] = etag;
            Save();
        }
    }
}
