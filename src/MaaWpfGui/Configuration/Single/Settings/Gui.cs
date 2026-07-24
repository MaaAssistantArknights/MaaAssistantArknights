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
using System.ComponentModel;
using System.Text.Json.Serialization;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.Configuration.Single.Settings;

public class Gui : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + ".");
        Performance.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + "." + nameof(Performance) + ".");
        ExternalNotification.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + "." + nameof(ExternalNotification) + ".");
        ExternalNotification.Configs.CollectionChanged += Handler.OnCollectionChangedFactory<Base>(prefix + nameof(Gui) + "." + nameof(ExternalNotification) + ".");
        RemoteControl.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + "." + nameof(RemoteControl) + ".");
        RuntimeSettings.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + "." + nameof(RuntimeSettings) + ".");
        ConnectSettings.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(Gui) + "." + nameof(ConnectSettings) + ".");
    }

    [JsonInclude]
    public Performance Performance { get; private set; } = new();

    [JsonInclude]
    public ExternalNotification ExternalNotification { get; private set; } = new();

    [JsonInclude]
    public RemoteControl RemoteControl { get; private set; } = new();

    [JsonInclude]
    public RuntimeSettings RuntimeSettings { get; private set; } = new();

    //[JsonInclude]
    //public ConnectSettings ConnectSettings { get; private set; } = new();

    public string WindowTitlePrefix { get; set; } = string.Empty;
}
