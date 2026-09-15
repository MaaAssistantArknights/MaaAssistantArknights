// <copyright file="RoguelikeStartingOpersConverter.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Single.MaaTask;

namespace MaaWpfGui.Configuration.Converter.Specific;

/// <summary>
/// 肉鸽开局干员迁移：旧版 RoguelikeTask 的 CoreChar/UseSupport 字段合并为 StartingOpers 的第 1 项。
/// 读到旧字段且 StartingOpers 为空时合成第 1 项，属性已删除故旧字段不再参与序列化。
/// </summary>
internal class RoguelikeStartingOpersConverter : JsonConverter<Root>
{
    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        if (reader.TokenType != JsonTokenType.StartObject)
        {
            throw new JsonException("Expected StartObject token");
        }

        // 读取整个 JSON 对象
        using var jsonDoc = JsonDocument.ParseValue(ref reader);
        var root = jsonDoc.RootElement;

        // 先用默认方式反序列化
        var rootObj = JsonSerializer.Deserialize<Root>(root.GetRawText(), GetOptionsWithoutThisConverter(options));

        if (rootObj == null)
        {
            return null;
        }

        if (!root.TryGetProperty("Configurations", out var configurationsElement)
            || configurationsElement.ValueKind != JsonValueKind.Object)
        {
            return rootObj;
        }

        foreach (var configProp in configurationsElement.EnumerateObject())
        {
            if (!rootObj.Configurations.TryGetValue(configProp.Name, out var configObj))
            {
                continue;
            }

            if (!configProp.Value.TryGetProperty("TaskQueue", out var taskQueueElement)
                || taskQueueElement.ValueKind != JsonValueKind.Array)
            {
                continue;
            }

            int taskIndex = 0;
            foreach (var taskElement in taskQueueElement.EnumerateArray())
            {
                if (taskIndex < configObj.TaskQueue.Count
                    && configObj.TaskQueue[taskIndex] is RoguelikeTask task
                    && task.StartingOpers.Count == 0
                    && TryGetLegacyStartingOper(taskElement, out var startingOper))
                {
                    task.StartingOpers = [startingOper];
                }

                taskIndex++;
            }
        }

        return rootObj;
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options)
    {
        // 使用默认序列化
        JsonSerializer.Serialize(writer, value, GetOptionsWithoutThisConverter(options));
    }

    /// <summary>
    /// 从旧版任务 JSON 读取 CoreChar/UseSupport 字段，任一字段存在且取值非默认时才有迁移产物。
    /// </summary>
    private static bool TryGetLegacyStartingOper(JsonElement taskElement, out RoguelikeTask.RoguelikeStartingOper startingOper)
    {
        startingOper = new RoguelikeTask.RoguelikeStartingOper();

        if (taskElement.ValueKind != JsonValueKind.Object)
        {
            return false;
        }

        if (taskElement.TryGetProperty("CoreChar", out var coreChar)
            && coreChar.ValueKind == JsonValueKind.String
            && coreChar.GetString() is { } name)
        {
            startingOper.Name = name;
        }

        if (taskElement.TryGetProperty("UseSupport", out var useSupport) && useSupport.ValueKind == JsonValueKind.True)
        {
            startingOper.UseSupport = true;
        }

        return !string.IsNullOrEmpty(startingOper.Name) || startingOper.UseSupport;
    }

    /// <summary>
    /// 获取不包含当前 Converter 的 JsonSerializerOptions，避免无限递归
    /// </summary>
    private static JsonSerializerOptions GetOptionsWithoutThisConverter(JsonSerializerOptions options)
    {
        var newOptions = new JsonSerializerOptions(options);
        for (int i = newOptions.Converters.Count - 1; i >= 0; i--)
        {
            if (newOptions.Converters[i] is RoguelikeStartingOpersConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }

        return newOptions;
    }
}
