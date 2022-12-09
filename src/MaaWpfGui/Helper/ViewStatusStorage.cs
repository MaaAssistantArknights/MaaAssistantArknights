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
        public static List<MaaProfile> ProfileList { get; private set; } = new List<MaaProfile>();

        public static MaaProfile CurrentProfile { get; private set; }

        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public static string Get(string key, string default_value)
        {
            if (CurrentProfile == null)
            {
                return default_value;
            }

            return CurrentProfile.Get(key, default_value);
        }

        /// <summary>
        /// Sets a key with a value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public static void Set(string key, string value)
        {
            if (CurrentProfile == null)
            {
                return;
            }

            CurrentProfile.Set(key, value);
        }

        /// <summary>
        /// Deletes a key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns><see langword="true"/> if delete successful. <see langword="false"/> if <paramref name="key"/> is not found or before inited.</returns>
        public static bool Delete(string key)
        {
            if (CurrentProfile == null)
            {
                return false;
            }

            return CurrentProfile.Delete(key);
        }

        /// <summary>
        /// Saves configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Save()
        {
            if (CurrentProfile == null)
            {
                return false;
            }

            return CurrentProfile.Save();
        }

        /// <summary>
        /// Loads spcific configuration.
        /// </summary>
        /// <param name="configPath">Specific configuration path.</param>
        /// <returns>Whether the operation is successful.</returns>
        private static MaaProfile Load(string configPath)
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
            if (configDetail.ContainsKey("Name"))
            {
                configName = configDetail["Name"].ToString();
            }
            else if (configFileName == "gui.json")
            {
                configName = "Default";
            }
            else
            {
                configName = configFileName;
            }

            return new MaaProfile(configName, configPath, configDetail);
        }

        /// <summary>
        /// Loads configuration.
        /// </summary>
        /// <param name="configPath">Specific configuration path.</param>
        /// <param name="withRestore">Whether to restore with backup file.</param>
        /// <returns>Configuration if the operation is successful, otherwise null.</returns>
        public static bool Load(string configPath = null, bool withRestore = true)
        {
            FileInfo configFile = null;
            if (configPath != null)
            {
                configFile = new FileInfo(configPath);
                if (!configFile.Exists)
                {
                    throw new FileNotFoundException("Specific configuration file not found or permission denied.", configPath);
                }

                var config = Load(configPath);
                if (config != null)
                {
                    ProfileList.Add(config);
                    CurrentProfile = config;
                }

                Logger.Info(string.Format("Specific configuration loaded.", ProfileList.Count));
            }

            DirectoryInfo dir = new DirectoryInfo(Environment.CurrentDirectory);
            foreach (FileInfo file in dir.GetFiles())
            {
                if (Regex.IsMatch(file.Name, @"^gui.*\.json$"))
                {
                    // if file equals configPath's file
                    if (configPath != null && file.FullName == configFile.FullName)
                    {
                        continue;
                    }

                    var config = Load(file.FullName);
                    if (config != null)
                    {
                        ProfileList.Add(config);
                    }
                }
            }

            Logger.Info(string.Format("Found {0} configuration files.", ProfileList.Count));

            if (ProfileList.Count == 0)
            {
                // init config
                ProfileList.Add(new MaaProfile("Default", Path.Combine(Environment.CurrentDirectory, "gui.json"), new JObject()));
            }

            try
            {
                CurrentProfile ??= ProfileList.Where((config) => config.ConfigName == "Default").Single();
            }
            catch (Exception e)
            {
                CurrentProfile = ProfileList.First();
            }

            Save();
            BakeUpDaily();

            return true;
        }

        public static bool Create(string configName, string baseConfigName = null)
        {
            if (ProfileList.Where((config) => config.ConfigName == configName).Count() > 0)
            {
                return false;
            }

            JObject baseConfig = null;
            if (baseConfigName != null)
            {
                try
                {
                    baseConfig = ProfileList.Where((config) => config.ConfigName == baseConfigName).Single().ConfigDetail;
                }
                catch (Exception e)
                {
                    baseConfig = new JObject();
                }
            }

            var configPath = Path.Combine(Environment.CurrentDirectory, "gui-" + configName + ".json");
            var config = new MaaProfile(configName, configPath, baseConfig ?? new JObject());
            ProfileList.Add(config);
            CurrentProfile = config;
            Save();

            return true;
        }

        public static void Checkout(string key, bool newWindow = false)
        {
            try
            {
                var config = ProfileList.Where((config) => config.ConfigName == key).Single();
                var result = Process.Start(new ProcessStartInfo(System.Windows.Forms.Application.ExecutablePath, string.Format("-f \"{0}\"", config.ConfigPath)));
                if (!newWindow)
                {
                    Application.Current.Shutdown();
                }

                Logger.Info(string.Format("Load Profile {0}", key));
            }
            catch (Exception e)
            {
                Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
            }
        }

        public static void Release()
        {
            CurrentProfile.Release();
        }

        /// <summary>
        /// Backs up configuration daily. (#2145)
        /// </summary>
        /// <param name="num">The number of backup files.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool BakeUpDaily(int num = 2)
        {
            return CurrentProfile.BakeUpDaily();
        }
    }
}
