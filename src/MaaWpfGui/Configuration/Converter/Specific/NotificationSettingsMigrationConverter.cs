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
using System.Collections.Generic;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Services.Notification;

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

            if (gui["ExternalNotification"] is JsonObject external
                && (external.ContainsKey("SendWhenComplete") || external.ContainsKey("SendWhenError")
                    || external.ContainsKey("SendWhenStalled") || external.ContainsKey("ShowWhenCompleteWithDetails")))
            {
                var patterns = new List<string>();
                if (ReadBoolean(external, "SendWhenComplete", true))
                {
                    patterns.Add(NotificationMessage.FormatTag(NotificationTag.TaskComplete));
                }

                if (ReadBoolean(external, "SendWhenError", true))
                {
                    patterns.Add(NotificationMessage.FormatTag(NotificationTag.TaskError));
                }

                if (ReadBoolean(external, "SendWhenStalled", false))
                {
                    patterns.Add(NotificationMessage.FormatTag(NotificationTag.Stalled));
                }

                notification["External"] = new JsonObject {
                    ["UseIndependent"] = true,
                    ["Enable"] = patterns.Count != 0,
                    ["FilterMode"] = (int)NotificationFilterMode.Whitelist,
                    ["FilterList"] = string.Join("|", patterns),
                    ["MaxEntries"] = ReadBoolean(external, "ShowWhenCompleteWithDetails", false) ? 100 : 0,
                    ["TimeMinutes"] = 60,
                };
            }

            gui["Notification"] = notification;
        }

        MigrateTagFilters((JsonObject)gui["Notification"]!);
        return gui.Deserialize<Gui>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Gui value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private static void MigrateTagFilters(JsonObject notification)
    {
        foreach (var property in notification)
        {
            if (property.Value is not JsonObject channel
                || channel["FilterList"] is not JsonValue value || !value.TryGetValue<string>(out var patterns))
            {
                continue;
            }

            foreach (var tag in Enum.GetValues<NotificationTag>())
            {
                patterns = patterns.Replace($@"\[{tag}\]", NotificationMessage.FormatTag(tag), StringComparison.Ordinal);
            }

            channel["FilterList"] = patterns;
        }
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
