// <copyright file="Gui.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using PropertyChanged;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;
using static MaaWpfGui.Models.PostActionSetting;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// Wpf相关设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class Gui
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Performance.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Performance) + ".");
        ExternalNotification.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(ExternalNotification) + ".");
        ExternalNotification.Configs.CollectionChanged += Handler.OnCollectionChangedFactory<Base>(prefix + nameof(ExternalNotification) + ".");
        RemoteControl.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(RemoteControl) + ".");
        RuntimeSettings.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(RuntimeSettings) + ".");
        ConnectSettings.EventBinding(prefix + nameof(ConnectSettings) + ".");
        StartUpSettings.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(StartUpSettings) + ".");
    }

    [JsonInclude]
    public ConnectSettings ConnectSettings { get; private set; } = new();

    [JsonInclude]
    public Performance Performance { get; private set; } = new();

    [JsonInclude]
    public ExternalNotification ExternalNotification { get; private set; } = new();

    [JsonInclude]
    public RemoteControl RemoteControl { get; private set; } = new();

    [JsonInclude]
    public RuntimeSettings RuntimeSettings { get; private set; } = new();

    [JsonInclude]
    public StartUpSettings StartUpSettings { get; private set; } = new();

    public string WindowTitlePrefix { get; set; } = string.Empty;

    public PostActions PostActions { get; set; } = PostActions.None;

    public bool AchievementPopupDisabled { get; set; }

    public bool AchievementPopupAutoClose { get; set; }
}
