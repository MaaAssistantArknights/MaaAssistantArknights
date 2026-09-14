// <copyright file="ThirdPartyMigrationConverter.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.Settings;

namespace MaaWpfGui.Configuration.Converter.Specific;

/// <summary>
/// 三方服务设置迁移：把 Gui.RuntimeSettings 下的上报与一图流 OpenAPI 字段搬到 Gui.ThirdParty。
/// 旧字段已从 RuntimeSettings 删除，默认序列化不会写回，迁移结果落盘后旧字段自然消失。
/// </summary>
internal class ThirdPartyMigrationConverter : JsonConverter<Root>
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
            if (!rootObj.Configurations.TryGetValue(configProp.Name, out var configObj)
                || !configProp.Value.TryGetProperty("Gui", out var guiElement)
                || !guiElement.TryGetProperty("RuntimeSettings", out var runtimeSettingsElement)
                || runtimeSettingsElement.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            var thirdParty = configObj.Gui.ThirdParty;
            foreach (var property in runtimeSettingsElement.EnumerateObject())
            {
                switch (property.Name)
                {
                    case nameof(ThirdParty.ReportToPenguin) when property.Value.ValueKind is JsonValueKind.True or JsonValueKind.False:
                        thirdParty.ReportToPenguin = property.Value.GetBoolean();
                        break;
                    case nameof(ThirdParty.PenguinId) when property.Value.ValueKind == JsonValueKind.String:
                        thirdParty.PenguinId = property.Value.GetString() ?? string.Empty;
                        break;
                    case nameof(ThirdParty.ReportToYituliu) when property.Value.ValueKind is JsonValueKind.True or JsonValueKind.False:
                        thirdParty.ReportToYituliu = property.Value.GetBoolean();
                        break;
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
            if (newOptions.Converters[i] is ThirdPartyMigrationConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }

        return newOptions;
    }
}
