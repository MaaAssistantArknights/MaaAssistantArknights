// <copyright file="GlobalGuiRenameConverter.cs" company="MaaAssistantArknights">
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
using Serilog;

namespace MaaWpfGui.Configuration.Converter.Specific;

public class GlobalGuiRenameConverter : JsonConverter<Root>
{
    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        if (reader.TokenType != JsonTokenType.StartObject)
        {
            throw new JsonException("Expected StartObject token");
        }

        using var jsonDoc = JsonDocument.ParseValue(ref reader);
        try
        {
            // 将 "GUI" 重命名为 "Gui"
            var jsonObject = JsonNode.Parse(jsonDoc.RootElement.GetRawText()) as JsonObject;
            if (jsonObject?.TryGetPropertyValue("GUI", out var guiNode) is true)
            {
                if (!jsonObject.ContainsKey("Gui"))
                {
                    jsonObject["Gui"] = guiNode?.DeepClone();
                }
                jsonObject.Remove("GUI");
            }
            return JsonSerializer.Deserialize<Root>(jsonObject?.ToJsonString() ?? jsonDoc.RootElement.GetRawText(), GetOptionsWithoutThisConverter(options));
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to deserialize Root object with GlobalGuiRenameConverter.");
            return JsonSerializer.Deserialize<Root>(jsonDoc.RootElement.GetRawText(), GetOptionsWithoutThisConverter(options));
        }
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
            if (newOptions.Converters[i] is GlobalGuiRenameConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }
        return newOptions;
    }
}
