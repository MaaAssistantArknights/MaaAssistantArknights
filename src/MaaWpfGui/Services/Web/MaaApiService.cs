// <copyright file="MaaApiService.cs" company="MaaAssistantArknights">
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

using System;
using System.IO;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Services.Web
{
    public class MaaApiService : IMaaApiService
    {
        private const string CacheDir = "cache/";

        public async Task<JObject> RequestMaaApiWithCache(string api)
        {
            var url = MaaUrls.MaaApi + api;

            var response = await Instances.HttpService.GetStringAsync(new Uri(url));
            if (response == null)
            {
                return LoadApiCache(api);
            }

            try
            {
                var json = (JObject)JsonConvert.DeserializeObject(response);
                var cache = CacheDir + api;
                string directoryPath = Path.GetDirectoryName(cache);

                if (!Directory.Exists(directoryPath))
                {
                    Directory.CreateDirectory(directoryPath!);
                }

                await File.WriteAllTextAsync(cache, response);

                return json;
            }
            catch
            {
                return null;
            }
        }

        public JObject LoadApiCache(string api)
        {
            var cache = CacheDir + api;
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
