﻿using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using ObservableCollections;
using Serilog;

[assembly: PropertyChanged.FilterType("MaaWpfGui.Configuration.")]

namespace MaaWpfGui.Configuration
{
    public class ConfigFactory
    {
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.new.json");

        // TODO: write backup method. WIP: https://github.com/Cryolitia/MaaAssistantArknights/tree/config
        private static readonly string _configurationBakFile = Path.Combine(Environment.CurrentDirectory, "config/gui.new.json.bak");

        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        private static readonly object _lock = new object();

        public delegate void ConfigurationUpdateEventHandler(string key, object oldValue, object newValue);

        public static event ConfigurationUpdateEventHandler ConfigurationUpdateEvent;

        private static readonly JsonSerializerOptions _options = new JsonSerializerOptions {WriteIndented = true, Converters = {new JsonStringEnumConverter()}};

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
                    _logger.Information("Failed to load configuration file, creating a new one");
                    parsed = new Root();
                }

                if (parsed.CurrentConfig is null)
                {
                    parsed.CurrentConfig = new SpecificConfig();
                }

                parsed.PropertyChanged += OnPropertyChangedFactory("Configurations.");
                parsed.Configurations.CollectionChanged += (in NotifyCollectionChangedEventArgs<KeyValuePair<string, SpecificConfig>> args) =>
                {
                    switch (args.Action)
                    {
                        case NotifyCollectionChangedAction.Add:
                        case NotifyCollectionChangedAction.Replace:
                            if (args.IsSingleItem)
                            {
                                args.NewItem.Value.GUI.PropertyChanged += OnPropertyChangedFactory("Configurations." + args.NewItem.Key, JsonSerializer.Serialize(args.NewItem.Value, _options), null);
                            }
                            else
                            {
                                foreach (var value in args.NewItems)
                                {
                                    value.Value.GUI.PropertyChanged += OnPropertyChangedFactory("Configurations." + value.Key, JsonSerializer.Serialize(value.Value, _options), null);
                                }
                            }

                            break;
                    }

                    OnPropertyChangedFactory("Configurations");
                };

                foreach (var keyValue in parsed.Configurations)
                {
                    keyValue.Value.GUI.PropertyChanged += OnPropertyChangedFactory("Configurations." + keyValue.Key + ".GUIConfig.");
                }

                return parsed;
            }
        });

        private static PropertyChangedEventHandler OnPropertyChangedFactory(string key, object oldValue, object newValue)
        {
            return (o, args) =>
            {
                var after = newValue;
                if (after == null && args is PropertyChangedEventDetailArgs)
                {
                    after = (args as PropertyChangedEventDetailArgs).NewValue;
                }

                OnPropertyChanged(key + args.PropertyName, oldValue, after);
            };
        }

        private static PropertyChangedEventHandler OnPropertyChangedFactory(string key)
        {
            return (o, args) =>
            {
                object after = null;
                if (args is PropertyChangedEventDetailArgs)
                {
                    after = (args as PropertyChangedEventDetailArgs).NewValue;
                }

                OnPropertyChanged(key + args.PropertyName, null, after);
            };
        }

        public static Root Root => _rootConfig.Value;

        public static readonly SpecificConfig CurrentConfig = Root.CurrentConfig;

        public static async void OnPropertyChanged(string key, object oldValue, object newValue)
        {
            var result = await Save();
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

        private static async Task<bool> Save(string file = null)
        {
            return await Task.Run(() =>
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
            });
        }
    }
}
