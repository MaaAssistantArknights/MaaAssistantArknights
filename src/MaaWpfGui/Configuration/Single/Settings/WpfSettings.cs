// <copyright file="WpfSettings.cs" company="MaaAssistantArknights">
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
using Serilog;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.Configuration.Single.Settings;

public class WpfSettings : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(WpfSettings) + ".");
        Performance.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(WpfSettings) + "." + nameof(Performance) + ".");
        ExternalNotification.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(WpfSettings) + "." + nameof(ExternalNotification) + ".");
        ExternalNotification.Configs.CollectionChanged += Handler.OnCollectionChangedFactory<Base>(prefix + nameof(WpfSettings) + "." + nameof(ExternalNotification) + ".");
        RemoteControl.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(WpfSettings) + "." + nameof(RemoteControl) + ".");
        RuntimeSettings.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(WpfSettings) + "." + nameof(RuntimeSettings) + ".");
    }

    public Performance Performance { get; set; } = new();

    public ExternalNotification ExternalNotification { get; set; } = new();

    public RemoteControl RemoteControl { get; set; } = new();

    public RuntimeSettings RuntimeSettings { get; set; } = new();
}
