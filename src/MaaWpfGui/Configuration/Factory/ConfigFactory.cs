// <copyright file="ConfigFactory.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using System.Text.Unicode;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Converter;
using MaaWpfGui.Configuration.Single;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using ObservableCollections;
using Serilog;
using static MaaWpfGui.Helper.PathsHelper;

[assembly: PropertyChanged.FilterType("MaaWpfGui.Configuration.")]

namespace MaaWpfGui.Configuration.Factory;

public static class ConfigFactory
{
    public static readonly string ConfigFile = Path.Combine(ConfigDir, "gui.new.json");

    // TODO: write backup method. WIP: https://github.com/Cryolitia/MaaAssistantArknights/tree/config
    private static readonly string _configBakFile = Path.Combine(ConfigDir, "gui.new.json.bak");

    private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

    private static readonly Lock _lock = new();
    private static Task? _saveTask;
    private const int PendingDelayMs = 200;

    private static bool _isReleasing = false;

    private static readonly Timer _debounceTimer = new(CreateSaveTask, null, Timeout.Infinite, Timeout.Infinite);

    private static readonly SemaphoreSlim _semaphore = new(1, 1);

    public delegate void ConfigurationUpdateEventHandler(string key, object? oldValue, object? newValue);

    // ReSharper disable once EventNeverSubscribedTo.Global
    public static event ConfigurationUpdateEventHandler? ConfigurationUpdateEvent;

    private static readonly JsonSerializerOptions _options = new() { WriteIndented = true, Converters = { new JsonStringEnumConverter(), new FightTaskStageResetModeInvalidToIgnoreConverter(), new FightTaskStageResetModeConverter() }, Encoder = JavaScriptEncoder.Create(UnicodeRanges.All), DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull };

    // TODO: 参考 ConfigurationHelper ，拆几个函数出来
    private static readonly Lazy<Root> _rootConfig = new(() => {
        lock (_lock)
        {
            if (Directory.Exists(ConfigDir) is false)
            {
                Directory.CreateDirectory(ConfigDir);
            }

            Root? parsed = null;
            if (File.Exists(ConfigFile))
            {
                try
                {
                    parsed = JsonSerializer.Deserialize<Root>(File.ReadAllText(ConfigFile), _options);
                    if (parsed is null)
                    {
                        _logger.Warning("Failed to load configuration file, copying configuration file to error file");
                        File.Copy(ConfigFile, ConfigFile + ".err", true);
                    }
                }
                catch (Exception e)
                {
                    _logger.Information(e, "Failed to parse configuration file.");
                }
            }

            if (parsed is null && File.Exists(_configBakFile))
            {
                _logger.Information("trying to use backup file");
                try
                {
                    parsed = JsonSerializer.Deserialize<Root>(File.ReadAllText(_configBakFile), _options);
                    if (parsed is not null)
                    {
                        _logger.Information("Backup file loaded successfully, copying backup file to configuration file");
                        File.Copy(_configBakFile, ConfigFile, true);
                    }
                    else
                    {
                        _logger.Warning("Failed to load backup file, copying backup file to error file");
                        File.Copy(_configBakFile, _configBakFile + ".err", true);
                    }
                }
                catch (Exception e)
                {
                    _logger.Information(e, "Failed to parse configuration file.");
                }
            }

            if (parsed is null)
            {
                _logger.Information("Failed to load configuration file, creating a new one");
                parsed = new Root();
            }

            parsed.PropertyChanged += OnPropertyChangedFactory("Root.");
            parsed.Configurations.CollectionChanged += (in NotifyCollectionChangedEventArgs<KeyValuePair<string, SpecificConfig>> args) => {
                switch (args.Action)
                {
                    case NotifyCollectionChangedAction.Add:
                    case NotifyCollectionChangedAction.Replace:
                        if (args.IsSingleItem)
                        {
                            SpecificConfigBind(args.NewItem.Key, args.NewItem.Value);
                        }
                        else
                        {
                            foreach (var value in args.NewItems)
                            {
                                SpecificConfigBind(value.Key, value.Value);
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

            parsed.Timers.CollectionChanged += OnCollectionChangedFactory<int, Global.Timer>("Root.Timers.");
            parsed.VersionUpdate.PropertyChanged += OnPropertyChangedFactory();
            parsed.AnnouncementInfo.PropertyChanged += OnPropertyChangedFactory();
            parsed.GUI.PropertyChanged += OnPropertyChangedFactory();

            foreach (var keyValue in parsed.Configurations)
            {
                SpecificConfigBind(keyValue.Key, keyValue.Value);
            }

            if (Save(_configBakFile, parsed))
            {
                _logger.Information("{File} saved", _configBakFile);
            }
            else
            {
                _logger.Warning("{File} save failed", _configBakFile);
            }

            if (ParseJsonFile(ConfigurationHelper.ConfigFile) is JsonObject oldConfigJson && oldConfigJson["Configurations"] is JsonObject configurationsObj)
            {
                if (oldConfigJson["Current"]?.GetValue<string>() is string oldCurrent && parsed.Current != oldCurrent)
                {
                    _logger.Warning("Current configuration in old configuration is {OldCurrent}, but in new configuration is {NewCurrent}, switching to old current", oldCurrent, parsed.Current);
                    parsed.Current = oldCurrent;
                }
                var configNames = configurationsObj.Select(i => i.Key);
                foreach (var name in parsed.Configurations.Select(i => i.Key).Except(configNames))
                {
                    parsed.Configurations.Remove(name);
                    _logger.Information("Configuration {ConfigName} does not exist in old configuration, remove it", name);
                }
            }

            if (parsed.Configurations.All(i => i.Key != parsed.Current))
            {
                parsed.Configurations.Add(parsed.Current, new SpecificConfig());
            }

            return parsed;

            void SpecificConfigBind(string name, SpecificConfig config)
            {
                var key = "Root.Configurations." + name + ".";
                config.PropertyChanged += OnPropertyChangedFactory(key);
                config.DragItemIsChecked.CollectionChanged += OnCollectionChangedFactory<string, bool>(key + nameof(SpecificConfig.DragItemIsChecked) + ".");
                config.InfrastOrder.CollectionChanged += OnCollectionChangedFactory<string, int>(key + nameof(SpecificConfig.InfrastOrder) + ".");

                config.TaskQueue.CollectionChanged += (in NotifyCollectionChangedEventArgs<BaseTask> args) => {
                    switch (args.Action)
                    {
                        case NotifyCollectionChangedAction.Add:
                        case NotifyCollectionChangedAction.Replace:
                            if (args.IsSingleItem)
                            {
                                args.NewItem.PropertyChanged += OnPropertyChangedFactory(key + args.NewItem.GetType().Name + ".");
                            }
                            else
                            {
                                foreach (var value in args.NewItems)
                                {
                                    value.PropertyChanged += OnPropertyChangedFactory(key + value.GetType().Name + ".");
                                }
                            }
                            OnPropertyChanged(parsed.Current + ".TaskQueue", null, null);
                            break;
                        case NotifyCollectionChangedAction.Remove:
                        case NotifyCollectionChangedAction.Move:
                        case NotifyCollectionChangedAction.Reset:
                            OnPropertyChanged(parsed.Current + ".TaskQueue", null, null);
                            break;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }
                };
                foreach (var task in config.TaskQueue)
                {
                    task.PropertyChanged += OnPropertyChangedFactory($"{key}.{task.Name}.");
                }
            }

            JsonObject? ParseJsonFile(string filePath)
            {
                if (File.Exists(filePath) is false)
                {
                    return null;
                }

                var str = File.ReadAllText(filePath);
                try
                {
                    var obj = JsonSerializer.Deserialize<JsonObject>(str);
                    return obj ?? throw new Exception("Failed to parse json file");
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "Failed to deserialize json file: {FilePath}", filePath);
                }

                return null;
            }
        }
    });

    private static PropertyChangedEventHandler OnPropertyChangedFactory(string key, object? oldValue, object? newValue)
    {
        return (o, args) => {
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
        return (o, args) => {
            object? after = null;
            if (args is PropertyChangedEventDetailArgs detailArgs)
            {
                after = detailArgs.NewValue;
            }

            OnPropertyChanged(key + o?.GetType().Name + "." + args.PropertyName, null, after);
        };
    }

    private static NotifyCollectionChangedEventHandler<KeyValuePair<T1, T2>> OnCollectionChangedFactory<T1, T2>(string key)
    {
        return (in NotifyCollectionChangedEventArgs<KeyValuePair<T1, T2>> args) => {
            OnPropertyChanged(key + args.NewItem.Key, null, args.NewItem.Value);
        };
    }

    // ReSharper disable once MemberCanBePrivate.Global
    public static Root Root => _rootConfig.Value;

    public static SpecificConfig CurrentConfig => Root.CurrentConfig;

    private static void OnPropertyChanged(string key, object? oldValue, object? newValue)
    {
        _debounceTimer.Change(PendingDelayMs, Timeout.Infinite);

        ConfigurationUpdateEvent?.Invoke(key, oldValue, newValue);
        _logger.Debug("Configuration {Key} has been set to `{NewValue}`, save scheduled", key, newValue);
    }

    private static void CreateSaveTask(object? state)
    {
        if (_isReleasing)
        {
            _logger.Error("Application is releasing, skip create save task");
            return;
        }

        // 创建保存任务（只在 Timer 触发时才创建）
        _saveTask = Task.Run(async () => {
            try
            {
                var result = await SaveAsync();
                if (result)
                {
                    _logger.Debug("Configuration saved");
                }
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to save configuration");
            }
        });
    }

    private static bool Save(string? file = null, Root? root = null)
    {
        lock (_lock)
        {
            try
            {
                File.WriteAllText(file ?? ConfigFile, JsonSerializer.Serialize(root ?? Root, _options));
            }
            catch (Exception e)
            {
                _logger.Error(e, "Failed to save configuration file.");
                return false;
            }

            return true;
        }
    }

    private static async Task<bool> SaveAsync(string? file = null)
    {
        await _semaphore.WaitAsync();
        try
        {
            var filePath = file ?? ConfigFile;
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
        _isReleasing = true;
        if (_saveTask is not null)
        {
            _logger.Information("Waiting for save task to complete");
            _saveTask.Wait();
        }
        lock (_lock)
        {
            if (Save())
            {
                _logger.Information("{File} saved", ConfigFile);
            }
            else
            {
                _logger.Warning("{File} save failed", ConfigFile);
            }
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

    public static bool AddConfiguration(string configName, string? copyFrom = null)
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
            Root.Configurations[configName] = new();
        }
        else
        {
            if (Root.Configurations.ContainsKey(copyFrom) is false)
            {
                _logger.Warning("Configuration {ConfigName} does not exist", copyFrom);
                return false;
            }

            Root.Configurations[configName] = JsonSerializer.Deserialize<SpecificConfig>(JsonSerializer.Serialize(Root.Configurations[copyFrom], _options), _options) ?? new();
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
        get {
            var lists = new List<string>(Root.Configurations.Count);
            using var enumerator = Root.Configurations.GetEnumerator();
            while (enumerator.MoveNext())
            {
                lists.Add(enumerator.Current.Key);
            }

            return lists;
        }
    }
}
