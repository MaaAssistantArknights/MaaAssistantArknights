// <copyright file="NotificationEvent.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Services.Notification;

// Immutable history preserves original text and tags even when a UI log changes.
public sealed record NotificationEvent(
    DateTimeOffset Timestamp,
    NotificationSource Source,
    string Content,
    string Color,
    NotificationMessage? Message = null,
    string Weight = "Regular",
    bool ShowTime = true)
{
    public string FilterContent => Message is null ? Content : $"{NotificationMessage.FormatTag(Message.Tag)} {Content}";
}
