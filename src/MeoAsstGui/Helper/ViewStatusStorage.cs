// <copyright file="ViewStatusStorage.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Documents;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MeoAsstGui
{
    /// <summary>
    /// 界面设置存储（读写json文件）
    /// </summary>
    public class ViewStatusStorage
    {
        public static List<Config> ConfigList { get; private set; } = new List<Config>();

        private static Config _currentConfig = null;

        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public static string Get(string key, string default_value)
        {
            return _currentConfig.Get(key, default_value);
        }


        /// <summary>
        /// Sets a key with a value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public static void Set(string key, string value)
        {
            _currentConfig.Set(key, value);
        }

        public static bool Delete(string key)
        {
            return _currentConfig.Delete(key);
        }

        /// <summary>
        /// Saves configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Save()
        {
            return _currentConfig.Save();
        }

        /// <summary>
        /// Loads configuration.
        /// </summary>
        /// <param name="withRestore">Whether to restore with backup file.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Load(bool withRestore = true)
        {
            DirectoryInfo dir = new DirectoryInfo(Environment.CurrentDirectory);
            foreach (FileInfo file in dir.GetFiles())
            {
                if (Regex.IsMatch(file.Name, @"^gui.*\.json$"))
                {
                    var configPath = file.FullName;
                    var configBakPath = file.FullName + ".bak";
                    var configDetail = new JObject();
                    try
                    {
                        string jsonStr = File.ReadAllText(configPath);

                        if (jsonStr.Length <= 2 && File.Exists(configPath))
                        {
                            jsonStr = File.ReadAllText(configPath);
                            try
                            {
                                File.Copy(configBakPath, configPath, true);
                            }
                            catch (Exception e)
                            {
                                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                            }
                        }

                        // 文件存在但为空，会读出来一个null，感觉c#这库有bug，如果是null 就赋值一个空JObject
                        configDetail = (JObject)JsonConvert.DeserializeObject(jsonStr) ?? new JObject();
                    }
                    catch (Exception e)
                    {
                        Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                        configDetail = new JObject();
                        return false;
                    }

                    ConfigList.Add(new Config(file.Name, file.FullName, configDetail));
                }
            }

            Logger.Info(string.Format("Found {0} config files.", ConfigList.Count));
            if (ConfigList.Count > 0)
            {
                _currentConfig = ConfigList.First();
                BakeUpDaily();
            }
            return true;
        }

        public static Config CurrentConfig => _currentConfig;

        public static void Checkout(string key)
        {
            try
            {
                var config = ConfigList.Where((config) => config.ConfigName == key).Single();
                _currentConfig = config;
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
            }
        }

        public static void Release()
        {
            _currentConfig.Release();
        }

        /// <summary>
        /// Backs up configuration daily. (#2145)
        /// </summary>
        /// <param name="num">The number of backup files.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool BakeUpDaily(int num = 2)
        {
            return _currentConfig.BakeUpDaily();
        }
    }

    public class Config
    {
        public readonly string ConfigName;
        public readonly string _configPath;

        private string _configBakPath => _configPath + ".bak";

        private JObject _config { get; set; }

        public Config(string configName, string configPath, JObject configDetail)
        {
            ConfigName = configName;
            _configPath = configPath;
            _config = configDetail;
        }


        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public string Get(string key, string default_value)
        {
            if (_config.ContainsKey(key))
            {
                return _config[key].ToString();
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
            _config[key] = value;
            Save();
        }

        public bool Delete(string key)
        {
            try
            {
                _config.Remove(key);
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
                var jsonStr = _config.ToString();
                if (jsonStr.Length > 2)
                {
                    using (StreamWriter sw = new StreamWriter(_configPath))
                    {
                        sw.Write(jsonStr);
                    }

                    if (new FileInfo(_configPath).Length > 2)
                    {
                        File.Copy(_configPath, _configBakPath, true);
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
