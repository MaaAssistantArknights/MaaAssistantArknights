// <copyright file="Timer.cs" company="MaaAssistantArknights">
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

using System;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;

namespace MaaWpfGui.Configuration.Global;

public class Timer : PropertyChangedBase
{
    [JsonConstructor]
    public Timer(int id, bool? enable, string config, int hour, int minute)
    {
        Id = id;
        IsEnabled = enable;
        Config = config;
        Hour = hour;
        Minute = minute;
    }

    public Timer(int id, string config)
    {
        Id = id;
        IsEnabled = false;
        Config = config;
        Hour = DateTimeOffset.Now.Hour;
        Minute = DateTimeOffset.Now.Minute;
    }

    public Timer()
        : this(-1, string.Empty)
    {
    }

    public int Id { get; set; }

    private static string _Name => LocalizationHelper.GetString("Timer");

    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    [JsonIgnore]
    public string Name => $"{_Name} {Id + 1}";

    [JsonInclude]
    public bool? IsEnabled { get; set => SetAndNotify(ref field, value); } = false;

    [JsonInclude]
    public string Config { get; set => SetAndNotify(ref field, value); }

    [JsonInclude]
    public int Hour { get; set => SetAndNotify(ref field, value); }

    [JsonInclude]
    public int Minute { get; set => SetAndNotify(ref field, value); }
}
