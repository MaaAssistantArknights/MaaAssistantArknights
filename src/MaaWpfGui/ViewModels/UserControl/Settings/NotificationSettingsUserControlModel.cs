// <copyright file="NotificationSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class NotificationSettingsItem : PropertyChangedBase
{
    private readonly string _enableKey;
    private readonly string _enableBlacklistKey;
    private readonly string _enableWhitelistKey;
    private readonly string _filterListKey;
    private readonly string? _maxEntriesKey;
    private readonly string? _timeMinutesKey;
    private readonly string _useIndependentKey;
    private readonly NotificationType _type;

    public NotificationSettingsItem(
        string enableKey,
        string enableBlacklistKey,
        string enableWhitelistKey,
        string filterListKey,
        string? maxEntriesKey,
        string? timeMinutesKey,
        string useIndependentKey,
        NotificationType type)
    {
        _enableKey = enableKey;
        _enableBlacklistKey = enableBlacklistKey;
        _enableWhitelistKey = enableWhitelistKey;
        _filterListKey = filterListKey;
        _maxEntriesKey = maxEntriesKey;
        _timeMinutesKey = timeMinutesKey;
        _useIndependentKey = useIndependentKey;
        _type = type;
    }

    public NotificationType ChannelType => _type;

    public bool ShowLimitFields => _maxEntriesKey != null;

    public bool ShowFilterList => UseIndependent && (EnableBlacklist || EnableWhitelist);

    public bool UseIndependent
    {
        get => Convert.ToBoolean(ConfigurationHelper.GetValue(_useIndependentKey, bool.FalseString));
        set
        {
            var current = Convert.ToBoolean(ConfigurationHelper.GetValue(_useIndependentKey, bool.FalseString));
            if (current == value)
            {
                return;
            }

            if (value)
            {
                InitFromDefaults();
            }

            ConfigurationHelper.SetValue(_useIndependentKey, value.ToString());
            NotifyOfPropertyChange();
            NotifyOfPropertyChange(nameof(ShowFilterList));
            RefreshAllProperties();
        }
    }

    private void InitFromDefaults()
    {
        // 首次启用独立设置时，将用户配置初始化为默认配置的值
        var defaultEnable = NotificationManager.GetDefaultEnableWhitelist(_type) || NotificationManager.GetDefaultEnableBlacklist(_type) || true;
        ConfigurationHelper.SetValue(_enableKey, defaultEnable.ToString());
        ConfigurationHelper.SetValue(_enableBlacklistKey, NotificationManager.GetDefaultEnableBlacklist(_type).ToString());
        ConfigurationHelper.SetValue(_enableWhitelistKey, NotificationManager.GetDefaultEnableWhitelist(_type).ToString());
        ConfigurationHelper.SetValue(_filterListKey, NotificationManager.GetDefaultFilterList(_type));
        if (_maxEntriesKey != null)
        {
            ConfigurationHelper.SetValue(_maxEntriesKey, NotificationManager.GetDefaultMaxEntries(_type).ToString());
        }

        if (_timeMinutesKey != null)
        {
            ConfigurationHelper.SetValue(_timeMinutesKey, NotificationManager.GetDefaultTimeMinutes(_type).ToString());
        }
    }

    public void RestoreToDefault()
    {
        InitFromDefaults();
        RefreshAllProperties();
    }

    private void RefreshAllProperties()
    {
        NotifyOfPropertyChange(nameof(Enable));
        NotifyOfPropertyChange(nameof(EnableBlacklist));
        NotifyOfPropertyChange(nameof(EnableWhitelist));
        NotifyOfPropertyChange(nameof(FilterList));
        NotifyOfPropertyChange(nameof(ShowFilterList));
        if (_maxEntriesKey != null)
        {
            NotifyOfPropertyChange(nameof(MaxEntries));
        }

        if (_timeMinutesKey != null)
        {
            NotifyOfPropertyChange(nameof(TimeMinutes));
        }
    }

    public bool Enable
    {
        get => Convert.ToBoolean(ConfigurationHelper.GetValue(_enableKey, bool.TrueString));
        set
        {
            var current = Convert.ToBoolean(ConfigurationHelper.GetValue(_enableKey, bool.TrueString));
            if (current == value)
            {
                return;
            }

            ConfigurationHelper.SetValue(_enableKey, value.ToString());
            NotifyOfPropertyChange();
        }
    }

    public bool EnableBlacklist
    {
        get => Convert.ToBoolean(ConfigurationHelper.GetValue(_enableBlacklistKey, bool.FalseString));
        set
        {
            var current = Convert.ToBoolean(ConfigurationHelper.GetValue(_enableBlacklistKey, bool.FalseString));
            if (current == value)
            {
                return;
            }

            ConfigurationHelper.SetValue(_enableBlacklistKey, value.ToString());
            NotifyOfPropertyChange();
            NotifyOfPropertyChange(nameof(ShowFilterList));
            if (value)
            {
                EnableWhitelist = false;
            }
        }
    }

    public bool EnableWhitelist
    {
        get => Convert.ToBoolean(ConfigurationHelper.GetValue(_enableWhitelistKey, bool.FalseString));
        set
        {
            var current = Convert.ToBoolean(ConfigurationHelper.GetValue(_enableWhitelistKey, bool.FalseString));
            if (current == value)
            {
                return;
            }

            ConfigurationHelper.SetValue(_enableWhitelistKey, value.ToString());
            NotifyOfPropertyChange();
            NotifyOfPropertyChange(nameof(ShowFilterList));
            if (value)
            {
                EnableBlacklist = false;
            }
        }
    }

    public string FilterList
    {
        get => ConfigurationHelper.GetValue(_filterListKey, string.Empty);
        set
        {
            ConfigurationHelper.SetValue(_filterListKey, value);
            NotifyOfPropertyChange();
        }
    }

    public int MaxEntries
    {
        get => int.TryParse(ConfigurationHelper.GetValue(_maxEntriesKey ?? string.Empty, "100"), out var val) ? val : 100;
        set
        {
            if (_maxEntriesKey == null)
            {
                return;
            }

            ConfigurationHelper.SetValue(_maxEntriesKey, value.ToString());
            NotifyOfPropertyChange();
        }
    }

    public int TimeMinutes
    {
        get => int.TryParse(ConfigurationHelper.GetValue(_timeMinutesKey ?? string.Empty, "60"), out var val) ? val : 60;
        set
        {
            if (_timeMinutesKey == null)
            {
                return;
            }

            ConfigurationHelper.SetValue(_timeMinutesKey, value.ToString());
            NotifyOfPropertyChange();
        }
    }
}

/// <summary>
/// 通知设置
/// </summary>
public class NotificationSettingsUserControlModel : PropertyChangedBase
{
    static NotificationSettingsUserControlModel()
    {
        Instance = new();
    }

    public static NotificationSettingsUserControlModel Instance { get; }

    public NotificationSettingsItem Overlay { get; } = new(
        enableKey: ConfigurationKeys.NotificationOverlayEnable,
        enableBlacklistKey: ConfigurationKeys.NotificationOverlayEnableBlacklist,
        enableWhitelistKey: ConfigurationKeys.NotificationOverlayEnableWhitelist,
        filterListKey: ConfigurationKeys.NotificationOverlayFilterList,
        maxEntriesKey: ConfigurationKeys.NotificationOverlayMaxEntries,
        timeMinutesKey: ConfigurationKeys.NotificationOverlayTimeMinutes,
        useIndependentKey: ConfigurationKeys.NotificationOverlayUseIndependent,
        type: NotificationType.Overlay);

    public NotificationSettingsItem SystemNotification { get; } = new(
        enableKey: ConfigurationKeys.NotificationSystemNotificationEnable,
        enableBlacklistKey: ConfigurationKeys.NotificationSystemNotificationEnableBlacklist,
        enableWhitelistKey: ConfigurationKeys.NotificationSystemNotificationEnableWhitelist,
        filterListKey: ConfigurationKeys.NotificationSystemNotificationFilterList,
        maxEntriesKey: null,
        timeMinutesKey: null,
        useIndependentKey: ConfigurationKeys.NotificationSystemNotificationUseIndependent,
        type: NotificationType.SystemNotification);

    public NotificationSettingsItem External { get; } = new(
        enableKey: ConfigurationKeys.NotificationExternalEnable,
        enableBlacklistKey: ConfigurationKeys.NotificationExternalEnableBlacklist,
        enableWhitelistKey: ConfigurationKeys.NotificationExternalEnableWhitelist,
        filterListKey: ConfigurationKeys.NotificationExternalFilterList,
        maxEntriesKey: ConfigurationKeys.NotificationExternalMaxEntries,
        timeMinutesKey: ConfigurationKeys.NotificationExternalTimeMinutes,
        useIndependentKey: ConfigurationKeys.NotificationExternalUseIndependent,
        type: NotificationType.External);

    public NotificationSettingsItem TaskQueueLog { get; } = new(
        enableKey: ConfigurationKeys.NotificationTaskQueueLogEnable,
        enableBlacklistKey: ConfigurationKeys.NotificationTaskQueueLogEnableBlacklist,
        enableWhitelistKey: ConfigurationKeys.NotificationTaskQueueLogEnableWhitelist,
        filterListKey: ConfigurationKeys.NotificationTaskQueueLogFilterList,
        maxEntriesKey: null,
        timeMinutesKey: null,
        useIndependentKey: ConfigurationKeys.NotificationTaskQueueLogUseIndependent,
        type: NotificationType.TaskQueueLog);
}
