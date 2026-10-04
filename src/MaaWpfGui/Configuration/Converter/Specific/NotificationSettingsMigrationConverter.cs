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
using System.Linq;
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
                var channel = NotificationSettings.Channel.CreateDefault(NotificationChannel.External);
                MigrateExternal(channel,
                    ReadBoolean(external, "SendWhenComplete", true),
                    ReadBoolean(external, "SendWhenError", true),
                    ReadBoolean(external, "SendWhenStalled", false),
                    ReadBoolean(external, "ShowWhenCompleteWithDetails", false));
                notification["External"] = JsonSerializer.SerializeToNode(channel, options);
            }

            gui["Notification"] = notification;
        }

        MigrateTagFilters((JsonObject)gui["Notification"]!);
        return gui.Deserialize<Gui>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Gui value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    // Both legacy configuration formats use the same boolean-to-policy mapping.
    internal static void MigrateExternal(NotificationSettings.Channel channel,
        bool sendWhenComplete, bool sendWhenError, bool sendWhenStalled, bool includeDetails)
    {
        var tags = new[] {
            (Tag: NotificationTag.TaskComplete, Enabled: sendWhenComplete),
            (Tag: NotificationTag.TaskError, Enabled: sendWhenError),
            (Tag: NotificationTag.Stalled, Enabled: sendWhenStalled),
        }.Where(item => item.Enabled).Select(item => NotificationMessage.FormatTag(item.Tag)).ToArray();
        channel.UseIndependent = true;
        channel.Enable = tags.Length != 0;
        channel.FilterMode = NotificationFilterMode.Whitelist;
        channel.FilterList = string.Join("|", tags);
        channel.MaxEntries = includeDetails ? 100 : 0;
    }

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
