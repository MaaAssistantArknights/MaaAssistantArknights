// <copyright file="NotificationTrigger.cs" company="MaaAssistantArknights">
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
using ExternalNotificationDeliverySettings = MaaWpfGui.Configuration.Single.Settings.ExternalNotification.DeliverySettings;

namespace MaaWpfGui.Services.Notification;

// One decision per new log: any enabled condition can trigger the same delivery.
// The service resets this counter when delivery is dispatched, not when a
// provider finishes asynchronously or a displayed log collection is cleared.
public sealed class NotificationTrigger
{
    private readonly NotificationFilter _whitelist = new();
    private long _newLogs;

    public bool ShouldSend(ExternalNotificationDeliverySettings policy, NotificationEvent notification, bool hasChannels)
    {
        if (!policy.Enable || !hasChannels)
        {
            Reset();
            return false;
        }

        ++_newLogs;
        var selectedEvent = notification.Message?.Kind switch {
            NotificationKind.TaskComplete => policy.SendWhenComplete,
            NotificationKind.TaskError => policy.SendWhenError,
            NotificationKind.Stalled => policy.SendWhenStalled,
            _ => false,
        };
        return selectedEvent
            || (policy.SendAfterLogCount && _newLogs >= policy.NewLogCount)
            || (policy.SendWhenContentMatches && _whitelist.Matches(policy.Whitelist, notification.Content) == true);
    }

    public void Reset() => _newLogs = 0;
}
