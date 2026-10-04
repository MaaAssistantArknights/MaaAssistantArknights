// <copyright file="ExternalNotificationContentSettingsModel.cs" company="MaaAssistantArknights">
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
using System;
using System.ComponentModel;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public sealed class ExternalNotificationContentSettingsModel : PropertyChangedBase, IDataErrorInfo
{
    private readonly ExternalNotification.ContentSettings _config;
    private readonly ExternalNotification.ContentSettings _defaults = new();

    public ExternalNotificationContentSettingsModel(ExternalNotification.ContentSettings config)
    {
        _config = config;
        _config.PropertyChanged += (_, _) => {
            NotifyOfPropertyChange(nameof(UseIndependent));
            NotifyOfPropertyChange(nameof(Enable));
            NotifyOfPropertyChange(nameof(EnableBlacklist));
            NotifyOfPropertyChange(nameof(EnableWhitelist));
            NotifyOfPropertyChange(nameof(FilterList));
            NotifyOfPropertyChange(nameof(MaxEntries));
            NotifyOfPropertyChange(nameof(TimeMinutes));
        };
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    public ExternalNotification.ContentSettings Effective => UseIndependent ? _config : _defaults;

    public bool UseIndependent
    {
        get => _config.UseIndependent;
        set => _config.UseIndependent = value;
    }

    public bool Enable
    {
        get => _config.Enable;
        set => _config.Enable = value;
    }

    public bool EnableBlacklist
    {
        get => _config.FilterMode == NotificationFilterMode.Blacklist;
        set => _config.FilterMode = value ? NotificationFilterMode.Blacklist
            : EnableBlacklist ? NotificationFilterMode.None : _config.FilterMode;
    }

    public bool EnableWhitelist
    {
        get => _config.FilterMode == NotificationFilterMode.Whitelist;
        set => _config.FilterMode = value ? NotificationFilterMode.Whitelist
            : EnableWhitelist ? NotificationFilterMode.None : _config.FilterMode;
    }

    public string FilterList
    {
        get => _config.FilterList;
        set => _config.FilterList = value;
    }

    public void RestoreDefaultFilterList() => FilterList = _defaults.FilterList;

    public int MaxEntries
    {
        get => _config.MaxEntries;
        set => _config.MaxEntries = Math.Clamp(value, 0, 10000);
    }

    public int TimeMinutes
    {
        get => _config.TimeMinutes;
        set => _config.TimeMinutes = Math.Clamp(value, 0, 10080);
    }

    [PropertyDependsOn(nameof(UseIndependent))]
    [PropertyDependsOn(nameof(EnableBlacklist))]
    [PropertyDependsOn(nameof(EnableWhitelist))]
    public bool ShowFilterList => UseIndependent && (EnableBlacklist || EnableWhitelist);

    [PropertyDependsOn(nameof(FilterList))]
    public bool IsFilterValid => NotificationFilter.IsValid(FilterList);

    public string Error => string.Empty;

    public string this[string columnName] => columnName == nameof(FilterList) && !IsFilterValid
        ? LocalizationHelper.GetString("NotificationSettingsInvalidFilterTip")
        : string.Empty;
}
