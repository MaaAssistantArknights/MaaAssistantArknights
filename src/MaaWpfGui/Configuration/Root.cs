// <copyright file="Root.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Text.Json.Serialization;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single;
using MaaWpfGui.Constants;
using ObservableCollections;

namespace MaaWpfGui.Configuration;

public class Root : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    [JsonInclude]
    public ObservableDictionary<string, SpecificConfig> Configurations { get; private set; } = [];

    [JsonInclude]
    public ObservableDictionary<int, Timer> Timers { get; private set; } = [];

    public int ConfigVersion { get; set; } = 1;

    public string Current { get; set; } = ConfigurationKeys.DefaultConfiguration;

    [JsonInclude]
    public VersionUpdate VersionUpdate { get; private set; } = new();

    [JsonInclude]
    public AnnouncementInfo AnnouncementInfo { get; private set; } = new();

    [JsonInclude]
    public GUI GUI { get; private set; } = new();

    [JsonIgnore]
    public SpecificConfig CurrentConfig
    {
        get
        {
            Configurations.TryGetValue(Current, out var result);
            return result!;
        }

        set => Configurations[Current] = value;
    }

    [UsedImplicitly]
    public void OnPropertyChanged(string propertyName, object before, object after)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
    }
}
