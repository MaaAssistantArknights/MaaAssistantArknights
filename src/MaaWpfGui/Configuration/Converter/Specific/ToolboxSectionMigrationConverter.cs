// <copyright file="ToolboxSectionMigrationConverter.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Configuration.Converter.Specific;

// Migration for the Toolbox settings reorg, applied on the JsonNode level before normal
// deserialization:
// 1. Move the previously flat Toolbox properties into per-feature sections
//    (Recruitment / OperBox / Gacha / Peep / MiniGame). A flat key is only moved when the
//    target section does not already carry the same key, and the flat key is removed either
//    way.
// 2. Move the queue-held Reclamation task entries into Toolbox.MiniGame.Reclamation, as
//    Reclamation is no longer a queue task type. The first entry supplies the settings;
//    duplicates are dropped. Applies to every profile so inactive profiles and backup
//    restoration keep their values.
// All passes are no-ops when the legacy nodes are absent, hence idempotent.
internal sealed class ToolboxSectionMigrationConverter : JsonConverter<Root>
{
    private static readonly Dictionary<string, string[]> _sectionKeys = new()
    {
        [nameof(Toolbox.Recruitment)] =
        [
            "ChooseLevel3", "ChooseLevel3Time", "ChooseLevel4", "ChooseLevel4Time", "ChooseLevel5", "ChooseLevel6", "AutoSetTime", "ShowPotential",
        ],
        [nameof(Toolbox.OperBox)] = ["OperBoxExportFormat", "OperBoxAvatarMode"],
        [nameof(Toolbox.Gacha)] = ["GachaShowDisclaimerNoMore"],
        [nameof(Toolbox.Peep)] = ["PeepTargetFps"],
        [nameof(Toolbox.MiniGame)] =
        [
            "EventShopBlackList", "EventShopBlackListDataSupplementInstrument", "EventShopBlackListDataSupplementStick",
            "EventShopBlackListLmd", "EventShopBlackListFurniturePart", "EventShopBlackListOther",
        ],
    };

    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        var node = JsonNode.Parse(ref reader);
        if (node is not JsonObject root)
        {
            return node?.Deserialize<Root>(WithoutThisConverter(options));
        }

        if (root[nameof(Root.Configurations)] is JsonObject configurations)
        {
            foreach (var (_, configNode) in configurations)
            {
                if (configNode is not JsonObject config)
                {
                    continue;
                }

                if (config[nameof(Toolbox)] is JsonObject toolbox)
                {
                    MigrateToolbox(toolbox);
                }

                MigrateReclamationTaskQueueEntry(config);
            }
        }

        return root.Deserialize<Root>(WithoutThisConverter(options));
    }

    // 任务队列里的生息演算条目（$type 为 ReclamationTask）搬入 Toolbox.MiniGame.Reclamation；
    // 首个条目供参数，其余重复条目仅移除，避免反序列化时命中已删除的派生类型注册
    private static void MigrateReclamationTaskQueueEntry(JsonObject config)
    {
        if (config[nameof(Single.SpecificConfig.TaskQueue)] is not JsonArray taskQueue)
        {
            return;
        }

        JsonObject? migrated = null;
        for (var i = taskQueue.Count - 1; i >= 0; i--)
        {
            if (taskQueue[i] is not JsonObject entry || entry["$type"]?.GetValue<string>() != "ReclamationTask")
            {
                continue;
            }

            migrated ??= entry;
            taskQueue.RemoveAt(i);
        }

        if (migrated is null)
        {
            return;
        }

        var toolbox = config[nameof(Toolbox)] as JsonObject ?? [];
        var miniGame = toolbox[nameof(Toolbox.MiniGame)] as JsonObject ?? [];
        var reclamation = miniGame[nameof(Toolbox.MiniGame.Reclamation)] as JsonObject ?? [];

        foreach (var (key, value) in migrated)
        {
            if (key == "$type")
            {
                continue;
            }

            if (reclamation[key] is null)
            {
                reclamation[key] = value?.DeepClone();
            }
        }

        miniGame[nameof(Toolbox.MiniGame.Reclamation)] = reclamation;
        toolbox[nameof(Toolbox.MiniGame)] = miniGame;
        config[nameof(Toolbox)] = toolbox;
    }

    private static void MigrateToolbox(JsonObject toolbox)
    {
        foreach (var (sectionName, keys) in _sectionKeys)
        {
            JsonObject? section = null;
            foreach (var key in keys)
            {
                if (toolbox[key] is not { } value)
                {
                    continue;
                }

                section ??= toolbox[sectionName] as JsonObject ?? [];
                if (section[key] is null)
                {
                    section[key] = value.DeepClone();
                }

                // 目标节已有同名键（曾运行过更新版本）时以节内值为准，仅移除平铺残留
                toolbox.Remove(key);
            }

            if (section is not null && toolbox[sectionName] is not JsonObject)
            {
                toolbox[sectionName] = section;
            }
        }
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options) =>
        JsonSerializer.Serialize(writer, value, WithoutThisConverter(options));

    private JsonSerializerOptions WithoutThisConverter(JsonSerializerOptions options)
    {
        var copy = new JsonSerializerOptions(options);
        for (var index = copy.Converters.Count - 1; index >= 0; --index)
        {
            if (copy.Converters[index] is ToolboxSectionMigrationConverter)
            {
                copy.Converters.RemoveAt(index);
            }
        }

        return copy;
    }
}
