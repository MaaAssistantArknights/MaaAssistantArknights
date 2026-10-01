// <copyright file="SkillMasteryConverter.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration.Single.MaaTask;

namespace MaaWpfGui.Configuration.Converter.Specific;

/// <summary>
/// <see cref="OperProgressTask.SkillMastery"/> 的 JSON 转换器。
/// <see cref="System.Runtime.CompilerServices.InlineArrayAttribute"/> 结构体只暴露私有字段，System.Text.Json 无法原生读写
/// （写出会被静默序列化成空对象，读入则抛出 JsonException），故显式按 <c>[技能1, 技能2, 技能3]</c> 数组处理，
/// 与剪贴板导入格式及下发给 MaaCore 的 <c>skill_mastery</c> 字段保持一致。
/// </summary>
internal class SkillMasteryConverter : JsonConverter<OperProgressTask.SkillMastery>
{
    /// <inheritdoc/>
    public override OperProgressTask.SkillMastery Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        switch (reader.TokenType)
        {
            case JsonTokenType.Null:
                return default;

            case JsonTokenType.StartArray:
                {
                    Span<int> levels = stackalloc int[3];
                    var index = 0;
                    while (reader.Read() && reader.TokenType != JsonTokenType.EndArray)
                    {
                        // 超过 3 个的元素直接忽略，不足 3 个的按 0 补齐
                        if (index < levels.Length)
                        {
                            levels[index++] = reader.GetInt32();
                        }
                    }

                    return OperProgressTask.SkillMastery.Of(levels[0], levels[1], levels[2]);
                }

            default:
                throw new JsonException($"Expected array or object for SkillMastery, got {reader.TokenType}.");
        }
    }

    /// <inheritdoc/>
    public override void Write(Utf8JsonWriter writer, OperProgressTask.SkillMastery value, JsonSerializerOptions options)
    {
        writer.WriteStartArray();
        writer.WriteNumberValue(value[0]);
        writer.WriteNumberValue(value[1]);
        writer.WriteNumberValue(value[2]);
        writer.WriteEndArray();
    }

    private static int GetLevel(JsonElement element, string propertyName)
        => element.ValueKind == JsonValueKind.Object
            && element.TryGetProperty(propertyName, out var level)
            && level.ValueKind == JsonValueKind.Number
            ? level.GetInt32()
            : 0;
}
