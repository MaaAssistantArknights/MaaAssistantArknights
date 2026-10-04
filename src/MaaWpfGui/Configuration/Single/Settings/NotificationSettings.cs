// <copyright file="NotificationSettings.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Models;
using MaaWpfGui.Services.Notification;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

public class NotificationSettings : NotifyPropertyChangedWithValue, IJsonOnDeserialized
{
    public bool EnableStallTimeout { get; set; } = true;

    public int StallTimeoutMinutes { get; set; } = 30;

    public int ReminderIntervalMinutes { get; set; } = 30;

    [JsonInclude]
    public Channel Overlay { get; private set; } = Channel.CreateDefault(NotificationChannel.Overlay);

    [JsonInclude]
    public Channel External { get; private set; } = Channel.CreateDefault(NotificationChannel.External);

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Overlay.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Overlay) + ".");
        External.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(External) + ".");
    }

    public void OnDeserialized()
    {
        StallTimeoutMinutes = Math.Clamp(StallTimeoutMinutes, 0, 11451);
        ReminderIntervalMinutes = Math.Clamp(ReminderIntervalMinutes, 1, 11451);
        Overlay ??= Channel.CreateDefault(NotificationChannel.Overlay);
        External ??= Channel.CreateDefault(NotificationChannel.External);
    }

    public class Channel : NotifyPropertyChangedWithValue, IJsonOnDeserialized
    {
        public bool UseIndependent { get; set; }

        public bool Enable { get; set; } = true;

        public NotificationFilterMode FilterMode { get; set; }

        public string FilterList { get; set; } = string.Empty;

        public int MaxEntries { get; set; } = 100;

        public int TimeMinutes { get; set; } = 60;

        // Always return a fresh object; customized settings cannot mutate defaults.
        public static Channel CreateDefault(NotificationChannel channel) => new() {
            FilterMode = channel is NotificationChannel.External or NotificationChannel.SystemNotification
                ? NotificationFilterMode.Whitelist
                : NotificationFilterMode.None,
            FilterList = channel switch {
                NotificationChannel.External => TagFilter(NotificationTag.TaskError, NotificationTag.TaskComplete, NotificationTag.Stalled),
                NotificationChannel.SystemNotification => TagFilter(NotificationTag.TaskError, NotificationTag.TaskComplete, NotificationTag.Test),
                _ => TagFilter(Enum.GetValues<NotificationTag>()),
            },
        };

        private static string TagFilter(params NotificationTag[] tags) =>
            string.Join("|", tags.Select(NotificationMessage.FormatTag));

        public void OnDeserialized()
        {
            FilterList ??= string.Empty;
            MaxEntries = Math.Clamp(MaxEntries, 0, 10000);
            TimeMinutes = Math.Clamp(TimeMinutes, 0, 10080);
        }
    }
}
