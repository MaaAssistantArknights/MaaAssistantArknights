// <copyright file="TolerantEnumConverter.cs" company="MaaAssistantArknights">
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
using Serilog;

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 容错枚举转换器工厂，为所有枚举类型自动创建 <see cref="TolerantEnumConverter{TEnum}"/>。
/// 替代 <see cref="JsonStringEnumConverter"/>，遇到未知枚举值时返回默认值而非抛出异常。
/// </summary>
internal sealed class TolerantEnumConverterFactory : JsonConverterFactory
{
    /// <inheritdoc/>
    public override bool CanConvert(Type typeToConvert) => typeToConvert.IsEnum;

    /// <inheritdoc/>
    public override JsonConverter CreateConverter(Type typeToConvert, JsonSerializerOptions options)
    {
        var converterType = typeof(TolerantEnumConverter<>).MakeGenericType(typeToConvert);
        return (JsonConverter)Activator.CreateInstance(converterType)!;
    }
}

/// <summary>
/// 容错的枚举 JSON 转换器。
/// 遇到无法识别的字符串或数值时，返回 <c>default(TEnum)</c> 并记录警告日志，
/// 而非抛出 <see cref="JsonException"/>。
/// </summary>
/// <typeparam name="TEnum">枚举类型。</typeparam>
internal sealed class TolerantEnumConverter<TEnum> : JsonConverter<TEnum>
    where TEnum : struct, Enum
{
    private readonly ILogger _logger = Log.ForContext<TolerantEnumConverter<TEnum>>();

    /// <inheritdoc/>
    public override TEnum Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        switch (reader.TokenType)
        {
            case JsonTokenType.String:
                return ParseOrDefault(reader.GetString());

            case JsonTokenType.Number:
                if (reader.TryGetInt64(out long longVal))
                {
                    var converted = (TEnum)Enum.ToObject(typeof(TEnum), longVal);
                    if (!Enum.IsDefined(converted))
                    {
                        _logger.Warning(
                            "Numeric value {Value} is not a defined member of {EnumType}, using default value {Default}",
                            longVal, typeof(TEnum).Name, default(TEnum));
                        return default;
                    }

                    return converted;
                }

                _logger.Warning(
                    "Failed to convert numeric value to enum type {EnumType}, using default value {Default}",
                    typeof(TEnum).Name, default(TEnum));
                return default;

            case JsonTokenType.Null:
                _logger.Warning(
                    "Null value encountered for enum type {EnumType}, using default value {Default}",
                    typeof(TEnum).Name, default(TEnum));
                return default;

            default:
                _logger.Warning(
                    "Unexpected token type {TokenType} for enum type {EnumType}, using default value {Default}",
                    reader.TokenType, typeof(TEnum).Name, default(TEnum));
                return default;
        }
    }

    /// <inheritdoc/>
    public override void Write(Utf8JsonWriter writer, TEnum value, JsonSerializerOptions options)
    {
        writer.WriteStringValue(value.ToString());
    }

    /// <inheritdoc/>
    public override TEnum ReadAsPropertyName(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return ParseOrDefault(reader.GetString());
    }

    /// <inheritdoc/>
    public override void WriteAsPropertyName(Utf8JsonWriter writer, TEnum value, JsonSerializerOptions options)
    {
        writer.WritePropertyName(value.ToString());
    }

    private TEnum ParseOrDefault(string? value)
    {
        if (Enum.TryParse(value, ignoreCase: true, out TEnum result) && Enum.IsDefined(result))
        {
            return result;
        }

        _logger.Warning(
            "Unrecognized enum value \"{Value}\" for {EnumType}, using default value {Default}",
            value, typeof(TEnum).Name, default(TEnum));
        return default;
    }
}
