// <copyright file="ConfigurationConvert.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration;
using MaaWpfGui.Constants;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper
{
    public class ConfigurationConverter
    {
        private static readonly string _configurationOldFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json.old");
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json");
        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        public static bool Convert()
        {
            if (Directory.Exists("config") is false)
            {
                Directory.CreateDirectory("config");
            }

            // Load configuration file
            var parsed = ParseJsonFile(_configurationFile);
            if (parsed is null)
            {
                return false;
            }

            if (parsed["ConfigVersion"] is null)
            {
                // v4 to v5
                try
                {
                    File.Copy(_configurationFile, _configurationOldFile, true);
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to save configuration file.");
                }

                return ConvertV4ToV5();
            }
            else if (parsed["ConfigVersion"].Type == JTokenType.Integer)
            {
                // 暂无意义
                return true;
            }
            else
            {
                return false;
            }
        }

        private static bool ConvertV4ToV5()
        {
            ConfigurationHelper.Load();
            ConfigFactory.CurrentConfig.GUI.Localization = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);
            Task.Run(() => ConfigFactory.Save()).Wait();
            return true;
        }

        private static JObject ParseJsonFile(string filePath)
        {
            if (File.Exists(filePath) is false)
            {
                return null;
            }

            var str = File.ReadAllText(filePath);

            try
            {
                var obj = (JObject)JsonConvert.DeserializeObject(str);
                return obj ?? throw new Exception("Failed to parse json file");
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to deserialize json file: {FilePath}", filePath);
            }

            return null;
        }
    }
}
