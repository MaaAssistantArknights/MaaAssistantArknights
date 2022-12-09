// <copyright file="ViewStatusStorage.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui
{

    public class MaaProfile
    {
        public string ConfigName { get; set; }

        public string ConfigPath { get; set; }

        public bool Started { get; set; } = false;

        private string _configBakPath => ConfigPath + ".bak";

        public JObject ConfigDetail { get; private set; }

        public MaaProfile(string configName, string configPath, JObject configDetail)
        {
            ConfigName = configName;
            ConfigPath = configPath;
            ConfigDetail = configDetail;
            Set("Name", configName);
        }


        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public string Get(string key, string default_value)
        {
            if (ConfigDetail.ContainsKey(key))
            {
                return ConfigDetail[key].ToString();
            }
            else
            {
                return default_value;
            }
        }

        /// <summary>
        /// Sets a key with a value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public void Set(string key, string value)
        {
            ConfigDetail[key] = value;
            Save();
        }

        public bool Delete(string key)
        {
            try
            {
                ConfigDetail.Remove(key);
                Save();
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Saves configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public bool Save()
        {
            if (_released)
            {
                return false;
            }

            try
            {
                var jsonStr = ConfigDetail.ToString();
                if (jsonStr.Length > 2)
                {
                    using (StreamWriter sw = new StreamWriter(ConfigPath))
                    {
                        sw.Write(jsonStr);
                    }

                    if (new FileInfo(ConfigPath).Length > 2)
                    {
                        File.Copy(ConfigPath, _configBakPath, true);
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        private bool _released = false;

        public void Release()
        {
            Save();
            _released = true;
        }

        /// <summary>
        /// Backs up configuration daily. (#2145)
        /// </summary>
        /// <param name="num">The number of backup files.</param>
        /// <returns>Whether the operation is successful.</returns>
        public bool BakeUpDaily(int num = 2)
        {
            if (File.Exists(_configBakPath) && DateTime.Now.Date != new FileInfo(_configBakPath).LastWriteTime.Date && num > 0)
            {
                try
                {
                    for (; num > 1; num--)
                    {
                        if (File.Exists(string.Concat(_configBakPath, num - 1)))
                        {
                            File.Copy(string.Concat(_configBakPath, num - 1), string.Concat(_configBakPath, num), true);
                        }
                    }

                    File.Copy(_configBakPath, string.Concat(_configBakPath, num), true);
                }
                catch (Exception)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
