// <copyright file="TolerantEnumConverterFactory.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 容错枚举转换器工厂，为所有枚举类型自动创建 <see cref="TolerantEnumConverter{TEnum}"/>。
/// 替代 <see cref="JsonStringEnumConverter"/>，提供字符串枚举、字典键以及 <c>[Flags]</c> 组合值支持。
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
