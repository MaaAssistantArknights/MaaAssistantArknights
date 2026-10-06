// <copyright file="NotificationSettingsMigrationConverter.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Single.Settings;

namespace MaaWpfGui.Configuration.Converter.Specific;

// Convert per-profile notification options once, before normal deserialization.
// Keeping migration here also covers inactive profiles and backup restoration.
internal sealed class NotificationSettingsMigrationConverter : JsonConverter<Gui>
{
    public override Gui? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var node = JsonNode.Parse(ref reader);
        if (node is not JsonObject gui)
        {
            return node?.Deserialize<Gui>(WithoutThisConverter(options));
        }

        if (gui["Notification"] is not JsonObject)
        {
            var notification = new JsonObject();
            if (gui["RuntimeSettings"] is JsonObject runtime)
            {
                Copy(runtime, notification, "EnableStallTimeout", "EnableStallTimeout");
                Copy(runtime, notification, "StallTimeoutMinutes", "StallTimeoutMinutes");
                Copy(runtime, notification, "StallTimeoutReminderIntervalMinutes", "ReminderIntervalMinutes");
            }

            gui["Notification"] = notification;
        }

        if (gui["ExternalNotification"] is not JsonObject external)
        {
            external = new();
            gui["ExternalNotification"] = external;
        }

        // Preserve current settings; migrate only the pre-branch boolean options.
        if (external["Delivery"] is not JsonObject)
        {
            var delivery = new ExternalNotification.DeliverySettings {
                Enable = external["Configs"] is JsonArray { Count: > 0 },
                SendBeforeScheduledStart = ReadBoolean(external, "SendBeforeScheduledStart", false),
            };
            if (external.ContainsKey("SendWhenComplete") || external.ContainsKey("SendWhenError")
                || external.ContainsKey("SendWhenStalled") || external.ContainsKey("ShowWhenCompleteWithDetails"))
            {
                MigrateExternal(delivery,
                    ReadBoolean(external, "SendWhenComplete", true),
                    ReadBoolean(external, "SendWhenError", true),
                    ReadBoolean(external, "SendWhenStalled", false),
                    ReadBoolean(external, "ShowWhenCompleteWithDetails", false));
            }

            external["Delivery"] = JsonSerializer.SerializeToNode(delivery, options);
        }

        return gui.Deserialize<Gui>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Gui value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    // The pre-branch configuration and its gui.json importer share this mapping.
    internal static void MigrateExternal(ExternalNotification.DeliverySettings delivery,
        bool sendWhenComplete, bool sendWhenError, bool sendWhenStalled, bool includeDetails)
    {
        delivery.SendWhenComplete = sendWhenComplete;
        delivery.SendWhenError = sendWhenError;
        delivery.SendWhenStalled = sendWhenStalled;
        delivery.IncludePreviousLogs = includeDetails;
        delivery.MaxEntries = includeDetails ? 100 : 2;
    }

    private static void Copy(JsonObject source, JsonObject target, string oldName, string newName)
    {
        if (source.TryGetPropertyValue(oldName, out var value))
        {
            target[newName] = value?.DeepClone();
        }
    }

    private static bool ReadBoolean(JsonObject source, string name, bool fallback) =>
        source[name] is JsonValue value && value.TryGetValue<bool>(out var result) ? result : fallback;

    private static JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var copy = new JsonSerializerOptions(options);
        for (var index = copy.Converters.Count - 1; index >= 0; --index)
        {
            if (copy.Converters[index] is NotificationSettingsMigrationConverter)
            {
                copy.Converters.RemoveAt(index);
            }
        }

        return copy;
    }
}
