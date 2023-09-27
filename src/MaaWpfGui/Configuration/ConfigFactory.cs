using System;
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
    public static class ConfigFactory
    {
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.new.json");

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
                    var key = "Configurations." + keyValue.Key;
                    keyValue.Value.GUI.PropertyChanged += OnPropertyChangedFactory(key + ".GUI.");
                    keyValue.Value.GUI.PropertyChanged += OnPropertyChangedFactory(key + ".Infrast.");
                    keyValue.Value.DragItemIsChecked.CollectionChanged += OnCollectionChangedFactory<bool>(key + ".DragItemIsChecked.");
                    keyValue.Value.InfrastOrder.CollectionChanged += OnCollectionChangedFactory<int>(key + ".InfrastOrder");
                    keyValue.Value.TaskQueueOrder.CollectionChanged += OnCollectionChangedFactory<int>(key + ".TaskQueue.Order");
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

        private static PropertyChangedEventHandler OnPropertyChangedFactory(string key)
        {
            return (o, args) =>
            {
                object after = null;
                if (args is PropertyChangedEventDetailArgs detailArgs)
                {
                    after = detailArgs.NewValue;
                }

                OnPropertyChanged(key + args.PropertyName, null, after);
            };
        }

        private static NotifyCollectionChangedEventHandler<KeyValuePair<string, T>> OnCollectionChangedFactory<T> (string key)
        {
            return (in NotifyCollectionChangedEventArgs<KeyValuePair<string, T>> args) =>
            {
                OnPropertyChanged(key + args.NewItem.Key, null, args.NewItem.Value);
            };
        }

        public static Root Root => _rootConfig.Value;

        public static SpecificConfig CurrentConfig => Root.CurrentConfig;

        private static async void OnPropertyChanged(string key, object oldValue, object newValue)
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

        private static void Release()
        {
            lock (_lock)
            {
                Save().Wait();
                Save(_configurationBakFile).Wait();
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
    }
}
