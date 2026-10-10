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
using System.Text.Json.Serialization;
using MaaWpfGui.Models;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

public class NotificationSettings : NotifyPropertyChangedWithValue, IJsonOnDeserialized
{
    public bool EnableStallTimeout { get; set; } = true;

    public int StallTimeoutMinutes { get; set; } = 30;

    public int ReminderIntervalMinutes { get; set; } = 30;

    public void EventBinding(string prefix) => PropertyChanged += Handler.OnPropertyChangedFactory(prefix);

    public void OnDeserialized()
    {
        StallTimeoutMinutes = Math.Clamp(StallTimeoutMinutes, 0, 11451);
        ReminderIntervalMinutes = Math.Clamp(ReminderIntervalMinutes, 1, 11451);
    }
}
