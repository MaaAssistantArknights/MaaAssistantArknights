// <copyright file="ConfigurationHelper.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using Serilog;

namespace MaaWpfGui.Helper
{
    public class ConfigurationHelper
    {
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json");
        private static readonly string _configurationBakFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json.bak");
        private static Dictionary<string, string> _kvs;

        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        public delegate void ConfigurationUpdateEventHandler(string key, string oldValue, string newValue);

        public static event ConfigurationUpdateEventHandler ConfigurationUpdateEvent;

        private static bool Released { get; set; }

        /// <summary>
        /// Get a configuration value
        /// </summary>
        /// <param name="key">The config key</param>
        /// <param name="defaultValue">The default value to return if the key is not existed</param>
        /// <returns>The config value</returns>
        public static string GetValue(string key, string defaultValue)
        {
            var hasValue = _kvs.TryGetValue(key, out var value);

            _logger.Debug("Read configuration key {Key} with default value {DefaultValue}, configuration hit: {HasValue}, configuration value {Value}", key, defaultValue, hasValue, value);

            return hasValue
                ? value
                : defaultValue;
        }

        /// <summary>
        /// Set a configuration value
        /// </summary>
        /// <param name="key">The config key</param>
        /// <param name="value">The config value</param>
        /// <returns>The return value of <see cref="Save"/></returns>
        public static bool SetValue(string key, string value)
        {
            var old = string.Empty;
            if (_kvs.ContainsKey(key))
            {
                old = _kvs[key];
                _kvs[key] = value;
            }
            else
            {
                _kvs.Add(key, value);
            }

            var result = Save();
            if (result)
            {
                ConfigurationUpdateEvent?.Invoke(key, old, value);
                _logger.Debug("Configuration {Key} has been set to {Value}", key, value);
            }
            else
            {
                _logger.Warning("Failed to save configuration {Key} to {Value}", key, value);
            }

            return result;
        }

        /// <summary>
        /// Deletes a configuration
        /// </summary>
        /// <param name="key">The configuration key.</param>
        /// <returns>The return value of <see cref="Save"/>.</returns>
        public static bool DeleteValue(string key)
        {
            var old = string.Empty;
            if (_kvs.ContainsKey(key))
            {
                old = _kvs[key];
            }

            _kvs.Remove(key);
            var result = Save();
            if (result)
            {
                ConfigurationUpdateEvent?.Invoke(key, old, string.Empty);
                _logger.Debug("Configuration {Key} has been deleted", key);
            }
            else
            {
                _logger.Warning("Failed to save configuration file when deleted {Key}", key);
            }

            return result;
        }

        /// <summary>
        /// Load configuration file
        /// </summary>
        /// <returns>True if success, false if failed</returns>
        public static bool Load()
        {
            if (Directory.Exists("config") is false)
            {
                Directory.CreateDirectory("config");
            }

            // Config file migration
            const string OldConfigFile = "gui.json";
            if (File.Exists(OldConfigFile))
            {
                if (File.Exists(_configurationFile))
                {
                    File.Delete(OldConfigFile);
                }

                File.Move(OldConfigFile, _configurationFile);
            }

            var oldBakFiles = Directory.GetFiles(".", "gui.json.bak*");
            if (oldBakFiles.Length != 0)
            {
                _logger.Information("Found old backup files, deleting...");
                foreach (var f in oldBakFiles)
                {
                    File.Delete(f);
                    _logger.Information("Deleted {0}", f);
                }
            }

            // Load configuration file
            var parsed = ParseJsonFile(_configurationFile);
            if (parsed is null)
            {
                if (File.Exists(_configurationBakFile))
                {
                    File.Copy(_configurationBakFile, _configurationFile, true);
                    parsed = ParseJsonFile(_configurationFile);
                }
            }

            if (parsed is null)
            {
                _logger.Information("Failed to load configuration file, creating a new one");
                _kvs = new Dictionary<string, string>();
            }
            else
            {
                _kvs = parsed;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Save configuration file
        /// </summary>
        /// <returns>The result of saving process</returns>
        public static bool Save()
        {
            if (Released)
            {
                _logger.Warning("Attempting to save configuration file after release, this is not allowed");
                return false;
            }

            var jsonStr = JsonSerializer.Serialize(_kvs, new JsonSerializerOptions
            {
                WriteIndented = true,
            });

            try
            {
                File.WriteAllText(_configurationFile, jsonStr);
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to save configuration file.");
                return false;
            }

            return true;
        }

        public static string GetCheckedStorage(string storageKey, string name, string defaultValue)
        {
            return GetValue(storageKey + name + ".IsChecked", defaultValue);
        }

        public static bool SetCheckedStorage(string storageKey, string name, string value)
        {
            return SetValue(storageKey + name + ".IsChecked", value);
        }

        public static string GetFacilityOrder(string facility, string defaultValue)
        {
            return GetValue("Infrast.Order." + facility, defaultValue);
        }

        public static bool SetFacilityOrder(string facility, string value)
        {
            return SetValue("Infrast.Order." + facility, value);
        }

        public static string GetTimer(int i, string defaultValue)
        {
            return GetValue($"Timer.Timer{i + 1}", defaultValue);
        }

        public static bool SetTimer(int i, string value)
        {
            return SetValue($"Timer.Timer{i + 1}", value);
        }

        public static string GetTimerHour(int i, string defaultValue)
        {
            return GetValue($"Timer.Timer{i + 1}Hour", defaultValue);
        }

        public static bool SetTimerHour(int i, string value)
        {
            return SetValue($"Timer.Timer{i + 1}Hour", value);
        }

        public static string GetTimerMin(int i, string defaultValue)
        {
            return GetValue($"Timer.Timer{i + 1}Min", defaultValue);
        }

        public static bool SetTimerMin(int i, string value)
        {
            return SetValue($"Timer.Timer{i + 1}Min", value);
        }

        public static string GetTaskOrder(string task, string defaultValue)
        {
            return GetValue("TaskQueue.Order." + task, defaultValue);
        }

        public static bool SetTaskOrder(string task, string value)
        {
            return SetValue("TaskQueue.Order." + task, value);
        }

        public static void Release()
        {
            Save();
            Released = true;
        }

        private static Dictionary<string, string> ParseJsonFile(string filePath)
        {
            if (File.Exists(filePath) is false)
            {
                return null;
            }

            var fs = File.OpenRead(filePath);

            try
            {
                var obj = JsonSerializer.Deserialize<Dictionary<string, string>>(fs);
                if (obj is null)
                {
                    throw new Exception("Failed to parse json file");
                }

                return obj;
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to deserialize json file: {FilePath}", filePath);
            }
            finally
            {
                fs.Close();
            }

            return null;
        }
    }
}
