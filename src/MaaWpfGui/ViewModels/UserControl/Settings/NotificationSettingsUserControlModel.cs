// <copyright file="NotificationSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public sealed class NotificationSettingsUserControlModel : PropertyChangedBase
{
    public NotificationSettingsUserControlModel()
    {
        var config = ConfigFactory.CurrentConfig.Gui.Notification;
        Overlay = new(config.Overlay, NotificationChannel.Overlay);
        External = new(config.External, NotificationChannel.External);
        RunningState.Instance.EnableStallTimeout = StallTimeoutEnabled;
        RunningState.Instance.StallTimeoutMinutes = StallTimeoutMinutes;
        RunningState.Instance.ReminderIntervalMinutes = ReminderIntervalMinutes;
        SettingsViewModel.GuiSettings.PropertyChanged += (_, args) => {
            if (args.PropertyName == nameof(UseNotify))
            {
                NotifyOfPropertyChange(nameof(UseNotify));
            }
        };
    }

    public NotificationSettingsItem Overlay { get; }

    public NotificationSettingsItem External { get; }

    public bool UseNotify
    {
        get => SettingsViewModel.GuiSettings.UseNotify;
        set => SettingsViewModel.GuiSettings.UseNotify = value;
    }

    public bool StallTimeoutEnabled
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.Notification.EnableStallTimeout = value;
            RunningState.Instance.EnableStallTimeout = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.Notification.EnableStallTimeout;

    public int StallTimeoutMinutes
    {
        get; set {
            value = Math.Clamp(value, 0, 11451);
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.Notification.StallTimeoutMinutes = value;
            RunningState.Instance.StallTimeoutMinutes = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.Notification.StallTimeoutMinutes;

    public int ReminderIntervalMinutes
    {
        get; set {
            value = Math.Clamp(value, 1, 11451);
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.Notification.ReminderIntervalMinutes = value;
            RunningState.Instance.ReminderIntervalMinutes = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.Notification.ReminderIntervalMinutes;
}
