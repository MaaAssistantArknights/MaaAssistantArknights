// <copyright file="ExternalNotificationDeliverySettingsModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public sealed class ExternalNotificationDeliverySettingsModel : PropertyChangedBase, IDataErrorInfo
{
    public ExternalNotificationDeliverySettingsModel(ExternalNotification.DeliverySettings settings)
    {
        Settings = settings;
        Settings.PropertyChanged += (_, args) => NotifyOfPropertyChange(args.PropertyName ?? string.Empty);
    }

    public ExternalNotification.DeliverySettings Settings { get; }

    public bool Enable
    {
        get => Settings.Enable;
        set => Settings.Enable = value;
    }

    public bool SendWhenComplete
    {
        get => Settings.SendWhenComplete;
        set => Settings.SendWhenComplete = value;
    }

    public bool SendWhenError
    {
        get => Settings.SendWhenError;
        set => Settings.SendWhenError = value;
    }

    public bool SendWhenStalled
    {
        get => Settings.SendWhenStalled;
        set => Settings.SendWhenStalled = value;
    }

    public bool SendBeforeScheduledStart
    {
        get => Settings.SendBeforeScheduledStart;
        set => Settings.SendBeforeScheduledStart = value;
    }

    public bool UseCustomConditions
    {
        get => Settings.UseCustomConditions;
        set => Settings.UseCustomConditions = value;
    }

    public bool SendAfterLogCount
    {
        get => Settings.SendAfterLogCount;
        set => Settings.SendAfterLogCount = value;
    }

    public int NewLogCount
    {
        get => Settings.NewLogCount;
        set => Settings.NewLogCount = Math.Clamp(value, 1, 10000);
    }

    public bool SendWhenContentMatches
    {
        get => Settings.SendWhenContentMatches;
        set => Settings.SendWhenContentMatches = value;
    }

    public string Whitelist
    {
        get => Settings.Whitelist;
        set => Settings.Whitelist = value;
    }

    public bool IncludePreviousLogs
    {
        get => Settings.IncludePreviousLogs;
        set => Settings.IncludePreviousLogs = value;
    }

    public int MaxEntries
    {
        get => Settings.MaxEntries;
        set => Settings.MaxEntries = Math.Clamp(value, 0, 10000);
    }

    public int TimeMinutes
    {
        get => Settings.TimeMinutes;
        set => Settings.TimeMinutes = Math.Clamp(value, 0, 10080);
    }

    public bool FilterPreviousLogs
    {
        get => Settings.FilterPreviousLogs;
        set => Settings.FilterPreviousLogs = value;
    }

    public string Blacklist
    {
        get => Settings.Blacklist;
        set => Settings.Blacklist = value;
    }

    public string Error => string.Empty;

    public string this[string columnName] => columnName switch {
        nameof(Whitelist) when !NotificationFilter.IsValid(Whitelist) => LocalizationHelper.GetString("NotificationSettingsInvalidFilterTip"),
        nameof(Blacklist) when !NotificationFilter.IsValid(Blacklist) => LocalizationHelper.GetString("NotificationSettingsInvalidFilterTip"),
        _ => string.Empty,
    };
}
