// <copyright file="ExternalNotificationSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.Utilities.ValueType;
using Serilog;
using Stylet;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 外部通知
/// </summary>
public class ExternalNotificationSettingsUserControlModel : PropertyChangedBase
{
    static ExternalNotificationSettingsUserControlModel()
    {
        Instance = new();
    }

    public ExternalNotificationSettingsUserControlModel()
    {
        var config = ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs;
        var list = new List<BaseConfig>();
        foreach (var configItem in config)
        {
            BaseConfig c = configItem switch {
                Smtp smtp => new SmtpConfig(smtp),
                ServerChan serverChan => new ServerChanConfig(serverChan),
                Discord discord => new DiscordConfig(discord),
                DingTalk dingTalk => new DingTalkConfig(dingTalk),
                Telegram telegram => new TelegramConfig(telegram),
                Bark bark => new BarkConfig(bark),
                Qmsg qmsg => new QmsgConfig(qmsg),
                Gotify gotify => new GotifyConfig(gotify),
                CustomWebhook customWebhook => new CustomWebhookConfig(customWebhook),
                _ => throw new NotSupportedException($"Unsupported config type: {configItem.GetType()}"),
            };
            list.Add(c);
        }
        ExternalNotificationConfigs = new ObservableCollection<BaseConfig>(list);
        ExternalNotificationConfigs.CollectionChanged += (o, e) => {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Clear();
            }
            else if (e.Action is NotifyCollectionChangedAction.Remove)
            {
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.RemoveAt(e.OldStartingIndex);
            }
            else if (e.Action is NotifyCollectionChangedAction.Move)
            {
                var item = ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs[e.OldStartingIndex];
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.RemoveAt(e.OldStartingIndex);
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Insert(e.NewStartingIndex, item);
            }
            else if (e.Action is NotifyCollectionChangedAction.Add)
            {
                e.NewItems?.OfType<BaseConfig>().ToList().ForEach(item => {
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(item.ToConfig());
                });
            }

            if (e.Action is NotifyCollectionChangedAction.Add or NotifyCollectionChangedAction.Replace)
            {
                e.NewItems?.OfType<BaseConfig>().ToList().ForEach(item => {
                    item.PropertyChanged += (s, e) => {
                        var index = ExternalNotificationConfigs.IndexOf(item);
                        if (index < 0 || index >= ExternalNotificationConfigs.Count || ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Count != ExternalNotificationConfigs.Count)
                        {
                            Log.Error("ExternalNotificationConfigs index out of range or count mismatch. Index: {Index}, ExternalNotificationConfigs Count: {ExternalNotificationConfigsCount}, Config Count: {ConfigCount}", index, ExternalNotificationConfigs.Count, ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Count);
                            return;
                        }

                        ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs[index] = item.ToConfig();
                    };
                });
            }
            NotifyOfPropertyChange(nameof(ConfigCount));
        };
        foreach (var item in ExternalNotificationConfigs)
        {
            item.PropertyChanged += (s, e) => {
                var index = ExternalNotificationConfigs.IndexOf(item);
                if (index < 0 || index >= ExternalNotificationConfigs.Count || ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Count != ExternalNotificationConfigs.Count)
                {
                    Log.Error("ExternalNotificationConfigs index out of range or count mismatch. Index: {Index}, ExternalNotificationConfigs Count: {ExternalNotificationConfigsCount}, Config Count: {ConfigCount}", index, ExternalNotificationConfigs.Count, ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Count);
                    return;
                }
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs[index] = item.ToConfig();
            };
        }
    }

    public static ExternalNotificationSettingsUserControlModel Instance { get; }

    // UI 绑定的方法
    [UsedImplicitly]
    public static void ExternalNotificationSendTest()
    {
        ExternalNotificationService.Send(LocalizationHelper.GetString("ExternalNotificationSendTestTitle"), LocalizationHelper.GetString("ExternalNotificationSendTestContent"), true);
    }

    public int ConfigCount => ExternalNotificationConfigs.Count;

    public bool ExternalNotificationSendWhenComplete
    {
        get => ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenComplete;
        set {
            ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenComplete = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ExternalNotificationEnableDetails
    {
        get => ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.ShowWhenCompleteWithDetails;
        set {
            ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.ShowWhenCompleteWithDetails = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ExternalNotificationSendWhenError
    {
        get => ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenError;
        set {
            ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenError = value;
            NotifyOfPropertyChange();
        }
    }

    public bool ExternalNotificationSendWhenStalled
    {
        get => ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenStalled;
        set {
            ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenStalled = value;
            NotifyOfPropertyChange();
        }
    }

    private static readonly List<GenericCombinedData<Type>> ExternalNotificationProviders =
        [
            new GenericCombinedData<Type> { Display = "ServerChan", Value = typeof(ServerChanConfig) },
            new GenericCombinedData<Type> { Display = "Telegram", Value = typeof(TelegramConfig) },
            new GenericCombinedData<Type> { Display = "Discord", Value = typeof(DiscordConfig) },
            new GenericCombinedData<Type> { Display = "DingTalk", Value = typeof(DingTalkConfig) },
            new GenericCombinedData<Type> { Display = "SMTP", Value = typeof(SmtpConfig) },
            new GenericCombinedData<Type> { Display = "Bark", Value = typeof(BarkConfig) },
            new GenericCombinedData<Type> { Display = "Qmsg", Value = typeof(QmsgConfig) },
            new GenericCombinedData<Type> { Display = "Gotify", Value = typeof(GotifyConfig) },
            new GenericCombinedData<Type> { Display = "Custom Webhook", Value = typeof(CustomWebhookConfig) }
        ];

    public static List<GenericCombinedData<Type>> ExternalNotificationProviderList => ExternalNotificationProviders;

    public void AddConfig(object sender, RoutedEventArgs e)
    {
        if (e.OriginalSource is not MenuItem item || item.DataContext is not GenericCombinedData<Type> data)
        {
            return;
        }
        if (CreateInstance(data.Value) is BaseConfig config)
        {
            ExternalNotificationConfigs.Add(config);
        }
        else
        {
            throw new ArgumentException($"Invalid Config: {data.Display}");
        }

        static object? CreateInstance(Type type)
        {
            foreach (var ctor in type.GetConstructors())
            {
                var ps = ctor.GetParameters();
                if (ps.All(p => p.HasDefaultValue))
                {
                    var args = ps.Select(p => p.DefaultValue).ToArray();
                    return ctor.Invoke(args);
                }
            }

            return null;
        }
    }

    public void RemoveConfig(BaseConfig config)
    {
        ExternalNotificationConfigs.Remove(config);
    }

    public ObservableCollection<BaseConfig> ExternalNotificationConfigs { get; private set => SetAndNotify(ref field, value); }

    #region External Notification Config

    // FIXME: 不知道为什么 TextBox 在高度变化时会导致 ScrollViewer 的偏移位置变成 0，直接锁到第一个元素去了。在编辑的时候先给它禁用了
    // 不要用 static，s:Action 找不到
    public void CustomWebhookBodyGotFocus() => Instances.SettingsViewModel.AllowScrollOffsetChange = false;

    public void CustomWebhookBodyLostFocus() => Instances.SettingsViewModel.AllowScrollOffsetChange = true;

    #endregion External Notification Config
}
