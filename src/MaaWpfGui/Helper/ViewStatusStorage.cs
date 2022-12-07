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
    /// <summary>
    /// 界面设置存储（读写json文件）
    /// </summary>
    public class ViewStatusStorage
    {
        public static List<Config> ConfigList { get; private set; } = new List<Config>();

        public static Config CurrentConfig { get; private set; }

        public static bool isSticky { get; private set; } = false;

        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public static string Get(string key, string default_value)
        {
            if (CurrentConfig == null)
            {
                return default_value;
            }

            return CurrentConfig.Get(key, default_value);
        }

        /// <summary>
        /// Sets a key with a value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public static void Set(string key, string value)
        {
            if (CurrentConfig == null)
            {
                return;
            }

            CurrentConfig.Set(key, value);
        }

        public static bool Delete(string key)
        {
            if (CurrentConfig == null)
            {
                return false;
            }

            return CurrentConfig.Delete(key);
        }

        /// <summary>
        /// Saves configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Save()
        {
            if (CurrentConfig == null)
            {
                return false;
            }

            return CurrentConfig.Save();
        }

        private static Config Load(string configPath)
        {
            var configFullPath = configPath;
            var configBakPath = configPath + ".bak";
            var configFileName = Path.GetFileName(configPath);
            JObject configDetail;
            try
            {
                string jsonStr = File.ReadAllText(configFullPath);

                if (jsonStr.Length <= 2 && File.Exists(configFullPath))
                {
                    jsonStr = File.ReadAllText(configFullPath);
                    try
                    {
                        File.Copy(configBakPath, configFullPath, true);
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
                return null;
            }

            string configName;
            if (configDetail.ContainsKey("name"))
            {
                configName = configDetail["name"].ToString();
            }
            else if (configFileName == "gui.json")
            {
                configName = "Default";
            }
            else
            {
                configName = configFileName;
            }

            return new Config(configName, configPath, configDetail);
        }

        /// <summary>
        /// Loads configuration.
        /// </summary>
        /// <param name="configPath">Specific configuration path.</param>
        /// <param name="withRestore">Whether to restore with backup file.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Load(string configPath = null, bool withRestore = true)
        {
            if (configPath != null)
            {
                isSticky = true;

                if (!File.Exists(configPath))
                {
                    throw new FileNotFoundException("Specific configuration file not found or permission denied.", configPath);
                }

                var config = Load(configPath);
                if (config != null)
                {
                    ConfigList.Add(config);
                }

                Logger.Info(string.Format("Specific configuration loaded.", ConfigList.Count));
            }
            else
            {
                DirectoryInfo dir = new DirectoryInfo(Environment.CurrentDirectory);
                foreach (FileInfo file in dir.GetFiles())
                {
                    if (Regex.IsMatch(file.Name, @"^gui.*\.json$"))
                    {
                        var config = Load(file.FullName);
                        if (config != null)
                        {
                            ConfigList.Add(config);
                        }
                    }
                }

                Logger.Info(string.Format("Found {0} configuration files.", ConfigList.Count));

                if (ConfigList.Count == 0)
                {
                    // init config
                    ConfigList.Add(new Config("Default", Path.Combine(Environment.CurrentDirectory, "gui.json"), new JObject()));
                }
            }

            try
            {
                CurrentConfig = ConfigList.Where((config) => config.ConfigName == "Default").Single();
            }
            catch (Exception e)
            {
                CurrentConfig = ConfigList.First();
            }

            Save();
            BakeUpDaily();

            return true;
        }

        public static void Checkout(string key)
        {
            try
            {
                var config = ConfigList.Where((config) => config.ConfigName == key).Single();
                CurrentConfig = config;
                var newProcess = new Process();
                newProcess.StartInfo.FileName = AppDomain.CurrentDomain.FriendlyName;
                newProcess.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();
                newProcess.StartInfo.Arguments = string.Format("-f \"{0}\"", config.ConfigPath);
                newProcess.Start();
                Application.Current.Shutdown();
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
            }
        }

        public static void Release()
        {
            CurrentConfig.Release();
        }

        /// <summary>
        /// Backs up configuration daily. (#2145)
        /// </summary>
        /// <param name="num">The number of backup files.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool BakeUpDaily(int num = 2)
        {
            return CurrentConfig.BakeUpDaily();
        }
    }

    public class Config
    {
        public readonly string ConfigName;
        public readonly string ConfigPath;

        private string _configBakPath => ConfigPath + ".bak";

        private JObject _config { get; set; }

        public Config(string configName, string configPath, JObject configDetail)
        {
            ConfigName = configName;
            ConfigPath = configPath;
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
