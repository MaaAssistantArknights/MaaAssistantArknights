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
using System.Diagnostics;
using System.Linq;
using ExternalNotificationDeliverySettings = MaaWpfGui.Configuration.Single.Settings.ExternalNotification.DeliverySettings;

namespace MaaWpfGui.Services.Notification;

// External notification context has bounded retention, independent of displayed logs.
public sealed class NotificationHistory
{
    private const int Capacity = 10000;
    private readonly Queue<NotificationEvent> _events = new();
    private readonly NotificationFilter _blacklist = new();

    public void Add(NotificationEvent notification)
    {
        _events.Enqueue(notification);
        while (_events.Count > Capacity)
        {
            _events.Dequeue();
        }
    }

    public string Bundle(NotificationEvent current, ExternalNotificationDeliverySettings policy)
    {
        var body = current.Message?.Content ?? current.Content;
        var maxEntries = Math.Clamp(policy.MaxEntries, 0, Capacity);
        if (!policy.IncludePreviousLogs || maxEntries == 0)
        {
            return body;
        }

        var cutoff = policy.TimeMinutes == 0
            ? DateTimeOffset.MinValue
            : current.Timestamp.AddMinutes(-policy.TimeMinutes);

        // Scan from newest to oldest. Only unfiltered, unexpired logs count
        // toward the attachment limit; the current payload is always kept.
        var previous = new List<NotificationEvent>();
        var started = Stopwatch.GetTimestamp();
        foreach (var item in _events.Reverse())
        {
            if (item.Timestamp < cutoff || ReferenceEquals(item, current))
            {
                continue;
            }

            if (policy.FilterPreviousLogs)
            {
                if (Stopwatch.GetElapsedTime(started) > TimeSpan.FromMilliseconds(100))
                {
                    return body;
                }

                var matches = _blacklist.Matches(policy.Blacklist, item.Content);
                if (matches is null)
                {
                    return body;
                }

                if (matches.Value)
                {
                    continue;
                }
            }

            previous.Add(item);
            if (previous.Count == maxEntries)
            {
                break;
            }
        }

        previous.Reverse();
        var context = previous.Select(item => $"[{item.Timestamp:HH:mm:ss}][{item.Color}] {item.Content}");
        return string.Join(Environment.NewLine, context.Append(body));
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
