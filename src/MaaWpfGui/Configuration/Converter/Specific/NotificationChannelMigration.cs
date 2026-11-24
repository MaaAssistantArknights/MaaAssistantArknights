// <copyright file="NotificationChannelMigration.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Nodes;
using MaaWpfGui.Configuration.Single.Settings;

namespace MaaWpfGui.Configuration.Converter.Specific;

internal static class NotificationChannelMigration
{
    internal static void Normalize(JsonObject external)
    {
        var shared = external["Delivery"] as JsonObject;
        if (!external.ContainsKey("Enable"))
        {
            external["Enable"] = shared?["Enable"]?.DeepClone() ?? JsonValue.Create(false);
        }

        var ids = new HashSet<Guid>();
        if (external["Configs"] is JsonArray configs)
        {
            for (int index = 0; index < configs.Count; index++)
            {
                if (configs[index] is not JsonObject original)
                {
                    throw new JsonException("Notification channel configuration must be an object.");
                }

                JsonObject channel;
                if (original.ContainsKey("Provider"))
                {
                    channel = original;
                }
                else
                {
                    var delivery = shared?.DeepClone().AsObject()
                        ?? JsonSerializer.SerializeToNode(new ExternalNotification.DeliverySettings())!.AsObject();
                    delivery["Enable"] = true;
                    channel = new JsonObject {
                        ["Provider"] = original.DeepClone(),
                        ["Delivery"] = delivery,
                    };
                    configs[index] = channel;
                }

                var validId = channel["Id"] is JsonValue idValue
                    && idValue.TryGetValue<string>(out var text)
                    && Guid.TryParse(text, out var parsed)
                    && parsed != Guid.Empty;
                var id = validId ? Guid.Parse(channel["Id"]!.GetValue<string>()) : Guid.Empty;
                if (id == Guid.Empty || !ids.Add(id))
                {
                    do
                    {
                        id = Guid.NewGuid();
                    }
                    while (!ids.Add(id));
                    channel["Id"] = id.ToString();
                }

                if (channel["Delivery"] is null)
                {
                    channel["Delivery"] = JsonSerializer.SerializeToNode(new ExternalNotification.DeliverySettings { Enable = true });
                }
            }
        }

        external.Remove("Delivery");
    }
}
