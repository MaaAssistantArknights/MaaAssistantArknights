// <copyright file="FightTaskStageResetModeConverter.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Configuration.Converter;

internal class FightTaskStageResetModeConverter : JsonConverter<Root>
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

        if (root.TryGetProperty("Configurations", out var configurationsElement) && configurationsElement.ValueKind == JsonValueKind.Object)
        {
            foreach (var configProp in configurationsElement.EnumerateObject())
            {
                var configName = configProp.Name;
                var configValue = configProp.Value;

                if (configValue.TryGetProperty("TaskQueue", out var taskQueueElement) && taskQueueElement.ValueKind == JsonValueKind.Array)
                {
                    int taskIndex = 0;
                    foreach (var taskElement in taskQueueElement.EnumerateArray())
                    {
                        var task = rootObj.Configurations[configName].TaskQueue[taskIndex];
                        if (task is FightTask fightTask)
                        {
                            if (!taskElement.TryGetProperty("StageResetMode", out _))
                            {
                                if (fightTask.HideUnavailableStage)
                                {
                                    fightTask.StageResetMode = FightStageResetMode.Current;
                                }
                                else
                                {
                                    fightTask.StageResetMode = FightStageResetMode.Ignore;
                                }
                            }
                        }
                        taskIndex++;
                    }
                }
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
    /// 获取不包含当前 Converter 的 JsonSerializerOptions，避免无限递归
    /// </summary>
    private static JsonSerializerOptions GetOptionsWithoutThisConverter(JsonSerializerOptions options)
    {
        var newOptions = new JsonSerializerOptions(options);
        for (int i = newOptions.Converters.Count - 1; i >= 0; i--)
        {
            if (newOptions.Converters[i] is FightTaskStageResetModeConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }
        return newOptions;
    }
}
