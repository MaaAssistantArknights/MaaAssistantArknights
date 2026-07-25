// <copyright file="TimerSettings.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Text.Json.Serialization;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Global;

public class TimerSettings : INotifyPropertyChanged, IJsonOnDeserialized
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(TimerSettings) + ".");
        List.CollectionChanged += (sender, e) => {
            if (e.Action is NotifyCollectionChangedAction.Add or NotifyCollectionChangedAction.Replace)
            {
                e.NewItems?.OfType<Timer>().ToList().ForEach(item => {
                    item.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(TimerSettings) + "." + nameof(List) + ".");
                });
            }
        };
        List.CollectionChanged += Handler.OnCollectionChangedFactory<Timer>(prefix + nameof(TimerSettings) + "." + nameof(List) + ".");
        foreach (Timer timer in List)
        {
            timer.PropertyChanged += Handler.OnPropertyChangedFactory(prefix + nameof(TimerSettings) + "." + nameof(List) + "." + timer.Config + ".");
        }
    }

    public bool ForceScheduledStart { get; set; }

    public bool ShowWindowBeforeForceScheduledStart { get; set; }

    public bool CustomConfig { get; set; }

    public ObservableCollection<Timer> List { get; private set; } = [];

    public void OnDeserialized()
    {
        // 临时补足到8个，支持添加删除后移除此代码
        if (List.Count < 8)
        {
            for (int i = List.Count; i < 8; i++)
            {
                List.Add(new Timer(i, string.Empty));
            }
        }
    }
}
