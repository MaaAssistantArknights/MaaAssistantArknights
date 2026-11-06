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
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.Utilities.ValueType;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 外部通知
/// </summary>
public class ExternalNotificationSettingsUserControlModel : PropertyChangedBase
{
    private readonly HashSet<NotificationChannelViewModel> _subscribedConfigs = [];

    static ExternalNotificationSettingsUserControlModel()
    {
        Instance = new();
    }

    public ExternalNotificationSettingsUserControlModel()
    {
        ExternalNotificationConfigs = new(ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs
            .Select(config => new NotificationChannelViewModel(config)));
        ExternalNotificationConfigs.CollectionChanged += OnConfigsChanged;
        SynchronizeSubscriptions();
        LocalizationHelper.LanguageChanged += RefreshLabels;
        RefreshLabels();
    }

    public static ExternalNotificationSettingsUserControlModel Instance { get; }

    public bool Enable
    {
        get => ConfigFactory.CurrentConfig.Gui.ExternalNotification.Enable;
        set {
            if (Enable == value)
            {
                return;
            }
            ConfigFactory.CurrentConfig.Gui.ExternalNotification.Enable = value;
            Instances.NotificationService?.ResetAllChannels();
            NotifyOfPropertyChange();
            NotifyOfPropertyChange(nameof(HasScheduledNotifications));
        }
    }

    public bool HasScheduledNotifications => Enable && ExternalNotificationConfigs.Any(item => item.DeliverySettings.Enable && item.DeliverySettings.SendBeforeScheduledStart);

    private void RefreshLabels()
    {
        ExternalNotificationProviderList.RefreshLocalization();
        for (int i = 0; i < ExternalNotificationConfigs.Count; i++)
        {
            ExternalNotificationConfigs[i].RefreshLabel(i + 1);
        }
    }

    public LocalizedObservableList<ExternalNotificationChannel> ExternalNotificationProviderList { get; } = new(
        ExternalNotificationChannel.All.Select(channel => new LocalizedEntry<ExternalNotificationChannel>(channel, channel.LocalizationKey)).ToArray());

    public ObservableCollection<NotificationChannelViewModel> ExternalNotificationConfigs { get; }

    public int ConfigCount => ExternalNotificationConfigs.Count;

    [UsedImplicitly]
    public static void ExternalNotificationSendTest() => Instances.NotificationService.TestExternalNotification();

    public void AddConfig(object sender, RoutedEventArgs e)
    {
        if (e.OriginalSource is MenuItem { DataContext: GenericCombinedData<ExternalNotificationChannel> data })
        {
            AddChannel(data.Value);
        }
    }

    public void AddChannel(ExternalNotificationChannel channel) => ExternalNotificationConfigs.Add(
        new NotificationChannelViewModel(new NotificationChannelConfig { Provider = channel.CreateEditor().ToConfig() }));

    public void TestChannel(NotificationChannelViewModel channel) => channel.TestChannel();

    public void RemoveConfig(NotificationChannelViewModel config) => ExternalNotificationConfigs.Remove(config);

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
                    saved.Insert(e.NewStartingIndex + index, ((NotificationChannelViewModel)e.NewItems[index]!).Config);
                }
                break;
            case NotifyCollectionChangedAction.Replace:
                for (var index = 0; index < e.NewItems!.Count; ++index)
                {
                    saved[e.NewStartingIndex + index] = ((NotificationChannelViewModel)e.NewItems[index]!).Config;
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
        NotifyOfPropertyChange(nameof(HasScheduledNotifications));
        RefreshLabels();
        if (ExternalNotificationChannel.All.All(channel =>
            ExternalNotificationConfigs.Any(config => config.Provider.GetType() == channel.EditorType)))
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
            Instances.NotificationService?.ResetChannel(item.Id);
            item.Dispose();
        }
        foreach (var item in ExternalNotificationConfigs.Except(_subscribedConfigs))
        {
            item.PropertyChanged += OnConfigChanged;
            _subscribedConfigs.Add(item);
        }
    }

    private void OnConfigChanged(object? sender, PropertyChangedEventArgs e)
    {
        NotifyOfPropertyChange(nameof(HasScheduledNotifications));
    }
}
