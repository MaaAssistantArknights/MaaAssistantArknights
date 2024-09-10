// <copyright file="ConfigFactory.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using ObservableCollections;
using Serilog;

[assembly: PropertyChanged.FilterType("MaaWpfGui.Configuration.")]

namespace MaaWpfGui.Configuration
{
    public static class ConfigFactory
    {
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.new.json");

        // TODO: write backup method. WIP: https://github.com/Cryolitia/MaaAssistantArknights/tree/config
        // ReSharper disable once UnusedMember.Local
        private static readonly string _configurationBakFile = Path.Combine(Environment.CurrentDirectory, "config/gui.new.json.bak");

        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        private static readonly object _lock = new();

        private static readonly SemaphoreSlim _semaphore = new(1, 1);

        public delegate void ConfigurationUpdateEventHandler(string key, object oldValue, object newValue);

        // ReSharper disable once EventNeverSubscribedTo.Global
        public static event ConfigurationUpdateEventHandler ConfigurationUpdateEvent;

        private static readonly JsonSerializerOptions _options = new() { WriteIndented = true, Converters = { new JsonStringEnumConverter() }, DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull };

        private static readonly Lazy<Root> _rootConfig = new Lazy<Root>(() =>
        {
            lock (_lock)
            {
                if (Directory.Exists("config") is false)
                {
                    Directory.CreateDirectory("config");
                }

                Root parsed = null;
                if (File.Exists(_configurationFile))
                {
                    try
                    {
                        parsed = JsonSerializer.Deserialize<Root>(File.ReadAllText(_configurationFile), _options);
                    }
                    catch (Exception e)
                    {
                        _logger.Information("Failed to parse configuration file: " + e);
                    }
                }

                if (parsed is null)
                {
                    if (File.Exists(_configurationBakFile))
                    {
                        File.Copy(_configurationBakFile, _configurationFile, true);
                        try
                        {
                            parsed = JsonSerializer.Deserialize<Root>(File.ReadAllText(_configurationFile), _options);
                        }
                        catch (Exception e)
                        {
                            _logger.Information("Failed to parse configuration file: " + e);
                        }
                    }
                }

                if (parsed is null)
                {
                    _logger.Information("Failed to load configuration file, creating a new one");
                    parsed = new Root();
                }

                parsed.CurrentConfig ??= new SpecificConfig();

                parsed.PropertyChanged += OnPropertyChangedFactory("Root.");
                parsed.Configurations.CollectionChanged += (in NotifyCollectionChangedEventArgs<KeyValuePair<string, SpecificConfig>> args) =>
                {
                    switch (args.Action)
                    {
                        case NotifyCollectionChangedAction.Add:
                        case NotifyCollectionChangedAction.Replace:
                            if (args.IsSingleItem)
                            {
                                args.NewItem.Value.GUI.PropertyChanged += OnPropertyChangedFactory("Root.Configurations." + args.NewItem.Key, JsonSerializer.Serialize(args.NewItem.Value, _options), null);
                            }
                            else
                            {
                                foreach (var value in args.NewItems)
                                {
                                    value.Value.GUI.PropertyChanged += OnPropertyChangedFactory("Root.Configurations." + value.Key, JsonSerializer.Serialize(value.Value, _options), null);
                                }
                            }

                            break;
                        case NotifyCollectionChangedAction.Remove:
                        case NotifyCollectionChangedAction.Move:
                        case NotifyCollectionChangedAction.Reset:
                            break;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }

                    OnPropertyChanged("Root.Configurations", null, null);
                };

                parsed.Timers.CollectionChanged += OnCollectionChangedFactory<int, Timer>("Root.Timers.");
                parsed.VersionUpdate.PropertyChanged += OnPropertyChangedFactory();
                parsed.AnnouncementInfo.PropertyChanged += OnPropertyChangedFactory();

                foreach (var keyValue in parsed.Configurations)
                {
                    var key = "Root.Configurations." + keyValue.Key + ".";
                    keyValue.Value.GUI.PropertyChanged += OnPropertyChangedFactory(key);
                    keyValue.Value.DragItemIsChecked.CollectionChanged += OnCollectionChangedFactory<string, bool>(key + nameof(SpecificConfig.DragItemIsChecked) + ".");
                    keyValue.Value.InfrastOrder.CollectionChanged += OnCollectionChangedFactory<string, int>(key + nameof(SpecificConfig.InfrastOrder) + ".");
                    keyValue.Value.TaskQueueOrder.CollectionChanged += OnCollectionChangedFactory<string, int>(key + nameof(SpecificConfig.TaskQueueOrder) + ".");
                }

                return parsed;
            }
        });

        private static PropertyChangedEventHandler OnPropertyChangedFactory(string key, object oldValue, object newValue)
        {
            return (o, args) =>
            {
                var after = newValue;
                if (after == null && args is PropertyChangedEventDetailArgs detailArgs)
                {
                    after = detailArgs.NewValue;
                }

                OnPropertyChanged(key + args.PropertyName, oldValue, after);
            };
        }

        private static PropertyChangedEventHandler OnPropertyChangedFactory(string key = "")
        {
            return (o, args) =>
            {
                object after = null;
                if (args is PropertyChangedEventDetailArgs detailArgs)
                {
                    after = detailArgs.NewValue;
                }

                OnPropertyChanged(key + o.GetType().Name + "." + args.PropertyName, null, after);
            };
        }

        private static NotifyCollectionChangedEventHandler<KeyValuePair<T1, T2>> OnCollectionChangedFactory<T1, T2>(string key)
        {
            return (in NotifyCollectionChangedEventArgs<KeyValuePair<T1, T2>> args) =>
            {
                OnPropertyChanged(key + args.NewItem.Key, null, args.NewItem.Value);
            };
        }

        // ReSharper disable once MemberCanBePrivate.Global
        public static Root Root => _rootConfig.Value;

        public static readonly SpecificConfig CurrentConfig = Root.CurrentConfig;

        private static async void OnPropertyChanged(string key, object oldValue, object newValue)
        {
            var result = await SaveAsync();
            if (result)
            {
                ConfigurationUpdateEvent?.Invoke(key, oldValue, newValue);
                _logger.Debug("Configuration {Key} has been set to {Value}", key, newValue);
            }
            else
            {
                _logger.Warning("Failed to save configuration {Key} to {Value}", key, newValue);
            }
        }

        private static bool Save(string file = null)
        {
            lock (_lock)
            {
                try
                {
                    File.WriteAllText(file ?? _configurationFile, JsonSerializer.Serialize(Root, _options));
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to save configuration file.");
                    return false;
                }

                return true;
            }
        }

        private static async Task<bool> SaveAsync(string file = null)
        {
            await _semaphore.WaitAsync();
            try
            {
                var filePath = file ?? _configurationFile;
                var jsonString = JsonSerializer.Serialize(Root, _options);
                await File.WriteAllTextAsync(filePath, jsonString);
                return true;
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to save configuration file.");
                return false;
            }
            finally
            {
                _semaphore.Release();
            }
        }

        public static void Release()
        {
            lock (_lock)
            {
                Save();
                Save(_configurationBakFile);
            }
        }

        public static bool SwitchConfig(string configName)
        {
            if (Root.Configurations.ContainsKey(configName) is false)
            {
                _logger.Warning("Configuration {ConfigName} does not exist", configName);
                return false;
            }

            Root.Current = configName;
            return true;
        }

        public static bool AddConfiguration(string configName, string copyFrom = null)
        {
            if (string.IsNullOrEmpty(configName))
            {
                return false;
            }

            if (Root.Configurations.ContainsKey(configName))
            {
                _logger.Warning("Configuration {ConfigName} already exists", configName);
                return false;
            }

            if (copyFrom is null)
            {
                Root.Configurations[configName] = new SpecificConfig();
            }
            else
            {
                if (Root.Configurations.ContainsKey(copyFrom) is false)
                {
                    _logger.Warning("Configuration {ConfigName} does not exist", copyFrom);
                    return false;
                }

                Root.Configurations[configName] = JsonSerializer.Deserialize<SpecificConfig>(JsonSerializer.Serialize(Root.Configurations[copyFrom], _options), _options);
            }

            return true;
        }

        public static bool DeleteConfiguration(string configName)
        {
            if (Root.Configurations.ContainsKey(configName) is false)
            {
                _logger.Warning("Configuration {ConfigName} does not exist", configName);
                return false;
            }

            if (Root.Current == configName)
            {
                _logger.Warning("Configuration {ConfigName} is current configuration, cannot delete", configName);
                return false;
            }

            Root.Configurations.Remove(configName);
            return true;
        }

        public static List<string> ConfigList
        {
            get
            {
                List<string> lists = new List<string>(Root.Configurations.Count);
                using IEnumerator<KeyValuePair<string, SpecificConfig>> enumerator = Root.Configurations.GetEnumerator();
                while (enumerator.MoveNext())
                {
                    lists.Add(enumerator.Current.Key);
                }

                return lists;
            }
        }
    }
}
