// <copyright file="NotificationLogBuffer.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.ViewModels.Items;

namespace MaaWpfGui.Services.Notification;

// Owned by the UI thread, with timestamps stored beside their own channel's items.
public sealed class NotificationLogBuffer
{
    private readonly Queue<DateTimeOffset> _timestamps = new();

    public ObservableCollection<LogItemViewModel> Items { get; } = [];

    public void Add(DateTimeOffset timestamp, LogItemViewModel item)
    {
        _timestamps.Enqueue(timestamp);
        Items.Add(item);
    }

    public void Trim(NotificationSettings.Channel policy, DateTimeOffset now)
    {
        var capacity = policy.MaxEntries == 0 ? 10000 : Math.Clamp(policy.MaxEntries, 1, 10000);
        var cutoff = policy.TimeMinutes == 0 ? DateTimeOffset.MinValue : now.AddMinutes(-policy.TimeMinutes);
        while (_timestamps.Count > capacity || (_timestamps.TryPeek(out var first) && first < cutoff))
        {
            _timestamps.Dequeue();
            Items.RemoveAt(0);
        }
    }

    public void Clear()
    {
        _timestamps.Clear();
        Items.Clear();
    }
}
