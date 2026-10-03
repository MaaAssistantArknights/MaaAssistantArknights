// <copyright file="NotificationHistory.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single.Settings;

namespace MaaWpfGui.Services.Notification;

// History has its own bounded retention; trimming an overlay never removes context.
public sealed class NotificationHistory
{
    private const int Capacity = 10000;
    private readonly Queue<NotificationEvent> _events = new();

    public void Add(NotificationEvent notification)
    {
        _events.Enqueue(notification);
        var cutoff = notification.Timestamp.AddMinutes(-10080);
        while (_events.Count > Capacity || (_events.TryPeek(out var first) && first.Timestamp < cutoff))
        {
            _events.Dequeue();
        }
    }

    public string Bundle(NotificationEvent current, NotificationSettings.Channel policy)
    {
        if (policy.MaxEntries == 0)
        {
            return current.Message?.Content ?? current.Content;
        }

        var cutoff = policy.TimeMinutes == 0
            ? DateTimeOffset.MinValue
            : current.Timestamp.AddMinutes(-policy.TimeMinutes);

        // The dispatch rule selects the trigger, not its context. Preserve each
        // event's real tag and append the payload once, with its original title.
        var context = _events
            .Where(item => item.Source == current.Source && item.Timestamp >= cutoff && !ReferenceEquals(item, current))
            .TakeLast(Math.Clamp(policy.MaxEntries, 0, Capacity))
            .Select(item => $"[{item.Timestamp:HH:mm:ss}][{item.Color}] {item.Content}");
        return string.Join(Environment.NewLine, context.Append(current.Message?.Content ?? current.Content));
    }

    public void Clear(NotificationSource source)
    {
        var remaining = _events.Where(item => item.Source != source).ToArray();
        _events.Clear();
        foreach (var item in remaining)
        {
            _events.Enqueue(item);
        }
    }
}
