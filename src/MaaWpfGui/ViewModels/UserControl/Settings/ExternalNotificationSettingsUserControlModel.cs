// <copyright file="ExternalNotificationSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.Utilities.ValueType;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 外部通知
/// </summary>
public class ExternalNotificationSettingsUserControlModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<ExternalNotificationSettingsUserControlModel>();
    private readonly HashSet<BaseConfig> _subscribedConfigs = [];

    static ExternalNotificationSettingsUserControlModel()
    {
        Instance = new();
    }

    public ExternalNotificationSettingsUserControlModel()
    {
        DeliverySettings = new(ConfigFactory.CurrentConfig.Gui.ExternalNotification.Delivery);
        ExternalNotificationConfigs = new(ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs
            .Select(config => ExternalNotificationChannel.ForConfig(config).ReadConfig(config)));
        ExternalNotificationConfigs.CollectionChanged += OnConfigsChanged;
        SynchronizeSubscriptions();
        LocalizationHelper.LanguageChanged += ExternalNotificationProviderList.RefreshLocalization;
    }

    public static ExternalNotificationSettingsUserControlModel Instance { get; }

    public ExternalNotificationDeliverySettingsModel DeliverySettings { get; }

    public LocalizedObservableList<ExternalNotificationChannel> ExternalNotificationProviderList { get; } = new(
        ExternalNotificationChannel.All.Select(channel => (channel, channel.LocalizationKey)).ToArray());

    public ObservableCollection<BaseConfig> ExternalNotificationConfigs { get; }

    public int ConfigCount => ExternalNotificationConfigs.Count;

    [UsedImplicitly]
    public static void ExternalNotificationSendTest() => Instances.NotificationService.TestExternalNotification();

    public void AddConfig(object sender, RoutedEventArgs e)
    {
        if (e.OriginalSource is MenuItem { DataContext: GenericCombinedData<ExternalNotificationChannel> data })
        {
            ExternalNotificationConfigs.Add(data.Value.CreateEditor());
        }
    }

    public void RemoveConfig(BaseConfig config) => ExternalNotificationConfigs.Remove(config);

    private void OnConfigsChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        var saved = ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs;
        switch (e.Action)
        {
            case NotifyCollectionChangedAction.Reset:
                saved.Clear();
                break;
            case NotifyCollectionChangedAction.Add:
                for (var index = 0; index < e.NewItems!.Count; ++index)
                {
                    saved.Insert(e.NewStartingIndex + index, ((BaseConfig)e.NewItems[index]!).ToConfig());
                }
                break;
            case NotifyCollectionChangedAction.Replace:
                for (var index = 0; index < e.NewItems!.Count; ++index)
                {
                    saved[e.NewStartingIndex + index] = ((BaseConfig)e.NewItems[index]!).ToConfig();
                }
                break;
            case NotifyCollectionChangedAction.Remove:
                for (var index = 0; index < e.OldItems!.Count; ++index)
                {
                    saved.RemoveAt(e.OldStartingIndex);
                }
                break;
            case NotifyCollectionChangedAction.Move:
                var moved = saved[e.OldStartingIndex];
                saved.RemoveAt(e.OldStartingIndex);
                saved.Insert(e.NewStartingIndex, moved);
                break;
        }

        SynchronizeSubscriptions();
        NotifyOfPropertyChange(nameof(ConfigCount));
        if (ExternalNotificationChannel.All.All(channel =>
            ExternalNotificationConfigs.Any(config => config.GetType() == channel.EditorType)))
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.AllChannelBroadcast);
        }
    }

    private void SynchronizeSubscriptions()
    {
        foreach (var item in _subscribedConfigs.Except(ExternalNotificationConfigs).ToArray())
        {
            item.PropertyChanged -= OnConfigChanged;
            _subscribedConfigs.Remove(item);
        }
        foreach (var item in ExternalNotificationConfigs.Except(_subscribedConfigs))
        {
            item.PropertyChanged += OnConfigChanged;
            _subscribedConfigs.Add(item);
        }
    }

    private void OnConfigChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not BaseConfig item)
        {
            return;
        }
        var index = ExternalNotificationConfigs.IndexOf(item);
        var saved = ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs;
        if (index < 0 || index >= saved.Count || saved.Count != ExternalNotificationConfigs.Count)
        {
            _logger.Error("External notification configuration count mismatch. Index: {Index}, editor count: {EditorCount}, saved count: {SavedCount}",
                index, ExternalNotificationConfigs.Count, saved.Count);
            return;
        }

        saved[index] = item.ToConfig();
    }
}
