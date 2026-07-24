// <copyright file="SpecificConfig.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Configuration.Single.Settings;
using ObservableCollections;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single;

public class SpecificConfig : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Gui.EventBinding(prefix + nameof(Gui) + ".");
        Toolbox.EventBinding(prefix + nameof(Toolbox) + ".");
        Copilot.EventBinding(prefix + nameof(Copilot) + ".");
    }

    [JsonInclude]
    public ObservableDictionary<string, int> InfrastOrder { get; private set; } = [];

    [JsonInclude]
    public ObservableList<BaseTask> TaskQueue { get; private set; } = [];

    [JsonInclude]
    public Gui Gui { get; private set; } = new();

    [JsonInclude]
    public Toolbox Toolbox { get; private set; } = new();

    [JsonInclude]
    public Copilot Copilot { get; private set; } = new();

    [JsonInclude]
    public int TaskSelectedIndex { get; set; } = -1;

    [JsonInclude]
    public ObservableDictionary<string, bool> DragItemIsChecked { get; private set; } = [];

    [UsedImplicitly]
    public void OnPropertyChanged(string propertyName, object before, object after)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
    }
}
