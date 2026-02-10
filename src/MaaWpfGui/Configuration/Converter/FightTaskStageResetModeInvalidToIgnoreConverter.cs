// <copyright file="FightTaskStageResetModeInvalidToIgnoreConverter.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration.Converter;

internal class FightTaskStageResetModeInvalidToIgnoreConverter : JsonConverter<Root>
{
    private const string TypeDiscriminatorProperty = "$type";
    private const string FightTaskTypeName = "FightTask";
    private const string StageResetModeProperty = "StageResetMode";
    private const string InvalidValue = "Invalid";
    private const string IgnoreValue = "Ignore";

    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        if (reader.TokenType != JsonTokenType.StartObject)
        {
            throw new JsonException("Expected StartObject token");
        }

        using var jsonDoc = JsonDocument.ParseValue(ref reader);
        JsonNode? rootNode = JsonNode.Parse(jsonDoc.RootElement.GetRawText());

        if (rootNode is JsonObject rootObj)
        {
            ReplaceInvalidStageResetModeInTaskQueues(rootObj);
        }

        string modifiedJson = rootNode?.ToJsonString() ?? string.Empty;
        return JsonSerializer.Deserialize<Root>(modifiedJson, GetOptionsWithoutThisConverter(options));
    }

    /// <summary>
    /// 遍历 Configurations.*.TaskQueue，将 $type 为 FightTask 且 StageResetMode 为 "Invalid" 的项改为 "Ignore"
    /// </summary>
    private static void ReplaceInvalidStageResetModeInTaskQueues(JsonObject root)
    {
        if (root["Configurations"] is not JsonObject configurations)
        {
            return;
        }

        foreach (var (_, configNode) in configurations)
        {
            if (configNode is not JsonObject config)
            {
                continue;
            }

            if (config["TaskQueue"] is not JsonArray taskQueue)
            {
                continue;
            }

            foreach (var taskNode in taskQueue)
            {
                if (taskNode is not JsonObject task)
                {
                    continue;
                }

                if (!IsFightTask(task))
                {
                    continue;
                }

                if (task[StageResetModeProperty] is JsonValue modeNode &&
                    modeNode.GetValue<string>() == InvalidValue)
                {
                    task[StageResetModeProperty] = IgnoreValue;
                }

                if (task["StagePlan"] is JsonArray stagePlan)
                {
                    for (int i = 0; i < stagePlan.Count; i++)
                    {
                        if (stagePlan[i] is JsonValue stageNode &&
                            stageNode.GetValue<string>() == "__INVALID__")
                        {
                            stagePlan[i] = "OR-8";
                        }
                    }
                }
            }
        }
    }

    /// <summary>
    /// 判断任务节点是否为 FightTask（依据 $type 字段）
    /// </summary>
    private static bool IsFightTask(JsonObject task)
    {
        if (task[TypeDiscriminatorProperty] is not JsonValue typeNode)
        {
            return false;
        }

        return typeNode.GetValue<string>() == FightTaskTypeName;
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options)
    {
        JsonSerializer.Serialize(writer, value, GetOptionsWithoutThisConverter(options));
    }

    private static JsonSerializerOptions GetOptionsWithoutThisConverter(JsonSerializerOptions options)
    {
        var newOptions = new JsonSerializerOptions(options);
        for (int i = newOptions.Converters.Count - 1; i >= 0; i--)
        {
            if (newOptions.Converters[i] is FightTaskStageResetModeInvalidToIgnoreConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }
        return newOptions;
    }
}
