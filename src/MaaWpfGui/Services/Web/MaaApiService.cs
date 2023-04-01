// <copyright file="MaaApiService.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Services.Web
{
    public class MaaApiService : IMaaApiService
    {
        private readonly IHttpService _httpService;

        private const string CacheDir = "cache/gui/";
        private const string MaaApi = "https://ota.maa.plus/MaaAssistantArknights/api/";

        public MaaApiService(IHttpService httpService)
        {
            _httpService = httpService;
        }

        public async Task<JObject> RequestMaaApiWithCache(string api)
        {
            if (!Directory.Exists(CacheDir))
            {
                Directory.CreateDirectory(CacheDir);
            }

            var url = MaaApi + api;

            var response = await _httpService.GetStringAsync(new Uri(url));
            if (response == null)
            {
                return LoadApiCache(api);
            }

            try
            {
                var json = (JObject)JsonConvert.DeserializeObject(response);
                var cache = CacheDir + api.Replace('/', '_');
                File.WriteAllText(cache, response);

                return json;
            }
            catch
            {
                return null;
            }
        }

        public JObject LoadApiCache(string api)
        {
            var cache = CacheDir + api.Replace('/', '_');
            if (!File.Exists(cache))
            {
                return null;
            }

            try
            {
                return (JObject)JsonConvert.DeserializeObject(File.ReadAllText(cache));
            }
            catch
            {
                return null;
            }
        }
    }
}
