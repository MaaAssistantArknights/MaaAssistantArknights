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
using System.Collections.Generic;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 容错的枚举 JSON 转换器。
/// 遇到无法识别的字符串或数值时，抛出带路径信息的 <see cref="JsonException"/>，
/// 由上层根级容错逻辑决定如何恢复到属性默认值。
/// </summary>
/// <typeparam name="TEnum">枚举类型。</typeparam>
internal sealed class TolerantEnumConverter<TEnum> : JsonConverter<TEnum>
    where TEnum : struct, Enum
{
    private static readonly bool _isFlagsEnum = typeof(TEnum).IsDefined(typeof(FlagsAttribute), inherit: false);
    private static readonly TypeCode _underlyingTypeCode = Type.GetTypeCode(Enum.GetUnderlyingType(typeof(TEnum)));
    private static readonly HashSet<ulong> _definedValues = GetDefinedValues();
    private static readonly ulong _validFlagsMask = GetValidFlagsMask();

    /// <inheritdoc/>
    public override TEnum Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        switch (reader.TokenType)
        {
            case JsonTokenType.String:
                return ParseOrThrow(reader.GetString());

            case JsonTokenType.Number:
                if (reader.TryGetInt64(out long longVal))
                {
                    var converted = (TEnum)Enum.ToObject(typeof(TEnum), longVal);
                    if (!IsValidValue(converted))
                    {
                        throw CreateValueConversionException(longVal.ToString());
                    }

                    return converted;
                }

                throw CreateValueConversionException();

            case JsonTokenType.Null:
                throw CreateValueConversionException();

            default:
                throw CreateValueConversionException();
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
        return ParseOrThrow(reader.GetString());
    }

    /// <inheritdoc/>
    public override void WriteAsPropertyName(Utf8JsonWriter writer, TEnum value, JsonSerializerOptions options)
    {
        writer.WritePropertyName(value.ToString());
    }

    private static TEnum ParseOrThrow(string? value)
    {
        if (Enum.TryParse(value, ignoreCase: true, out TEnum result) && IsValidValue(result))
        {
            return result;
        }

        throw CreateValueConversionException(value);
    }

    private static bool IsValidValue(TEnum value)
    {
        var rawValue = ToUInt64(value);
        if (_definedValues.Contains(rawValue))
        {
            return true;
        }

        if (!_isFlagsEnum)
        {
            return false;
        }

        return rawValue != 0 && (rawValue & ~_validFlagsMask) == 0;
    }

    private static HashSet<ulong> GetDefinedValues()
    {
        HashSet<ulong> values = [];
        foreach (var value in Enum.GetValues<TEnum>())
        {
            values.Add(ToUInt64(value));
        }

        return values;
    }

    private static ulong GetValidFlagsMask()
    {
        ulong mask = 0;
        foreach (var value in Enum.GetValues<TEnum>())
        {
            mask |= ToUInt64(value);
        }

        return mask;
    }

    private static ulong ToUInt64(TEnum value)
    {
        return _underlyingTypeCode switch
        {
            TypeCode.SByte or TypeCode.Int16 or TypeCode.Int32 or TypeCode.Int64 => unchecked((ulong)Convert.ToInt64(value)),
            TypeCode.Byte or TypeCode.UInt16 or TypeCode.UInt32 or TypeCode.UInt64 => Convert.ToUInt64(value),
            _ => throw new InvalidOperationException($"Unsupported enum underlying type: {_underlyingTypeCode}"),
        };
    }

    private static JsonException CreateValueConversionException(string? value = null)
    {
        return string.IsNullOrEmpty(value)
            ? new JsonException($"The JSON value could not be converted to {typeof(TEnum)}.")
            : new JsonException($"The JSON value '{value}' could not be converted to {typeof(TEnum)}.");
    }
}
