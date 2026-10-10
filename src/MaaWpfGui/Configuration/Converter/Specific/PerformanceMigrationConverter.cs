// <copyright file="PerformanceMigrationConverter.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Configuration.Converter.Specific;

// Migrations for the settings-page reorg, applied on the JsonNode level before normal
// deserialization:
// 1. Move the per-profile Performance section into the global Gui tree. GPU preference and
//    software rendering are machine-wide, so only the current profile's value survives;
//    values of other profiles are kept in place and dropped as they may conflict with no
//    merge semantics. Keeping migration here also covers inactive profiles and backup
//    restoration.
// 2. Rename SettingKey ExternalNotificationSettings to NotificationSettings in the persisted
//    order/collapse lists. Release users never saw NotificationSettings (introduced after the
//    last release), so it must inherit the position and collapse state of the removed key
//    instead of being appended at the end.
// Both are no-op when the legacy keys are absent, hence idempotent.
internal sealed class PerformanceMigrationConverter : JsonConverter<Root>
{
    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var node = JsonNode.Parse(ref reader);
        if (node is not JsonObject root)
        {
            return node?.Deserialize<Root>(WithoutThisConverter(options));
        }

        if (root["Gui"] is not JsonObject globalGui)
        {
            root["Gui"] = globalGui = [];
        }

        if (root["Configurations"] is JsonObject configurations)
        {
            string? currentName = null;
            if (root["Current"] is JsonValue current && current.TryGetValue<string>(out var currentNameValue))
            {
                currentName = currentNameValue;
            }

            foreach (var (name, configNode) in configurations)
            {
                if (configNode is not JsonObject config || config["Gui"] is not JsonObject gui ||
                    gui["Performance"] is not JsonObject performance)
                {
                    continue;
                }

                if (name == currentName)
                {
                    gui.Remove("Performance");
                    globalGui["Performance"] = performance;
                }

                // 无法确定当前档案时不迁移，节点留在原位以免设置被静默重置
            }
        }

        if (globalGui["IgnoreBadModulesAndUseSoftwareRendering"] is JsonValue softwareRendering)
        {
            globalGui.Remove("IgnoreBadModulesAndUseSoftwareRendering");
            var performance = globalGui["Performance"] as JsonObject ?? [];
            performance["IgnoreBadModulesAndUseSoftwareRendering"] = softwareRendering;
            globalGui["Performance"] = performance;
        }

        foreach (var listName in new[] { "SettingOrders", "CollapesStates" })
        {
            if (globalGui[listName] is not JsonArray list)
            {
                continue;
            }

            // 列表已含 NotificationSettings（如曾运行过更新版本）时直接移除废弃键，避免重命名产生重复项
            var hasNotificationSettings = list.Any(item => item is JsonValue v && v.TryGetValue<string>(out var s) && s == "NotificationSettings");
            for (var i = list.Count - 1; i >= 0; i--)
            {
                if (list[i] is JsonValue item && item.TryGetValue<string>(out var keyName) && keyName == "ExternalNotificationSettings")
                {
                    if (hasNotificationSettings)
                    {
                        list.RemoveAt(i);
                    }
                    else
                    {
                        list[i] = "NotificationSettings";
                    }
                }
            }
        }

        return root.Deserialize<Root>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private static JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var copy = new JsonSerializerOptions(options);
        for (var index = copy.Converters.Count - 1; index >= 0; --index)
        {
            if (copy.Converters[index] is PerformanceMigrationConverter)
            {
                copy.Converters.RemoveAt(index);
            }
        }

        return copy;
    }
}
