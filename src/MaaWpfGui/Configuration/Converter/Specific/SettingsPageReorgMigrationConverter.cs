// <copyright file="SettingsPageReorgMigrationConverter.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Configuration.Converter.Specific;

// Migrations for the settings-page reorg, applied on the JsonNode level before normal
// deserialization:
// 1. Move the per-profile Performance section into the global Gui tree. GPU preference and
//    software rendering are machine-wide, so only the current profile's value survives;
//    values of other profiles are kept in place and dropped as they may conflict with no
//    merge semantics. Keeping migration here also covers inactive profiles and backup
//    restoration.
// 2. In the persisted order list, rename SettingKey ExternalNotificationSettings to
//    NotificationSettings so the merged section inherits its position. Release users never
//    saw NotificationSettings (introduced after the last release), so without the rename it
//    would be appended at the end. The collapse list is not migrated: collapse state does
//    not carry over to the merged section.
// 3. Drop unknown or removed SettingKey entries from both lists here, so the tolerant
//    deserialization fallback (exception per invalid key) is not triggered on first launch.
// All are no-op when the legacy keys are absent, hence idempotent.
internal sealed class SettingsPageReorgMigrationConverter : JsonConverter<Root>
{
    private const string RemovedExternalNotificationSettingsKey = "ExternalNotificationSettings";

    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var node = JsonNode.Parse(ref reader);
        if (node is not JsonObject root)
        {
            return node?.Deserialize<Root>(WithoutThisConverter(options));
        }

        if (root[nameof(Root.Gui)] is not JsonObject globalGui)
        {
            root[nameof(Root.Gui)] = globalGui = [];
        }

        if (root[nameof(Root.Configurations)] is JsonObject configurations)
        {
            string? currentName = null;
            if (root[nameof(Root.Current)] is JsonValue current && current.TryGetValue<string>(out var currentNameValue))
            {
                currentName = currentNameValue;
            }

            foreach (var (name, configNode) in configurations)
            {
                if (configNode is not JsonObject config || config[nameof(Root.Gui)] is not JsonObject gui ||
                    gui[nameof(Gui.Performance)] is not JsonObject performance)
                {
                    continue;
                }

                if (name == currentName)
                {
                    gui.Remove(nameof(Gui.Performance));
                    globalGui[nameof(Gui.Performance)] = performance;
                }

                // 无法确定当前档案时不迁移，节点留在原位以免设置被静默重置
            }
        }

        if (globalGui[nameof(Gui.Performance.IgnoreBadModulesAndUseSoftwareRendering)] is JsonValue softwareRendering)
        {
            globalGui.Remove(nameof(Gui.Performance.IgnoreBadModulesAndUseSoftwareRendering));
            var performance = globalGui[nameof(Gui.Performance)] as JsonObject ?? [];
            performance[nameof(Gui.Performance.IgnoreBadModulesAndUseSoftwareRendering)] = softwareRendering;
            globalGui[nameof(Gui.Performance)] = performance;
        }

        foreach (var listName in new[] { nameof(Gui.SettingOrders), nameof(Gui.CollapesStates) })
        {
            if (globalGui[listName] is not JsonArray list)
            {
                continue;
            }

            // 列表已含 NotificationSettings（如曾运行过更新版本）时直接移除废弃键，避免重命名产生重复项；
            // 大小写变体同样计入（含手改的小写形态），防止其绕过防重复检查
            var hasNotificationSettings = list.Any(item => item is JsonValue itemValue &&
                itemValue.TryGetValue<string>(out var itemString) &&
                string.Equals(itemString, nameof(SettingKey.NotificationSettings), StringComparison.OrdinalIgnoreCase));

            // 大小写宽容度对齐 TolerantEnumConverter 的 ignoreCase 解析：只删下游必死的键，不误删大小写变体
            for (var i = list.Count - 1; i >= 0; i--)
            {
                var entry = list[i] is JsonValue entryValue && entryValue.TryGetValue<string>(out var entryString)
                    ? entryString
                    : null;

                if (string.Equals(entry, RemovedExternalNotificationSettingsKey, StringComparison.OrdinalIgnoreCase) &&
                    listName == nameof(Gui.SettingOrders))
                {
                    if (hasNotificationSettings)
                    {
                        list.RemoveAt(i);
                    }
                    else
                    {
                        list[i] = nameof(SettingKey.NotificationSettings);
                    }

                    continue;
                }

                if (entry is null || !Enum.TryParse(entry, ignoreCase: true, out SettingKey settingKey) ||
                    !Enum.IsDefined(settingKey))
                {
                    list.RemoveAt(i);
                }
            }
        }

        return root.Deserialize<Root>(WithoutThisConverter(options));
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var copy = new JsonSerializerOptions(options);
        for (var index = copy.Converters.Count - 1; index >= 0; --index)
        {
            if (copy.Converters[index] is SettingsPageReorgMigrationConverter)
            {
                copy.Converters.RemoveAt(index);
            }
        }

        return copy;
    }
}
