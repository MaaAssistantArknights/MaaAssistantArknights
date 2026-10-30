// <copyright file="HeatColumnOffsetConverter.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Windows.Data;

namespace MaaWpfGui.Converters;

/// <summary>
/// 多值转换器：values[0] 为列号，values[1] 为总列数，values[2] 为网格实际宽度，返回该列左边缘的横坐标。
/// 用于把标签对齐到等分列宽的 UniformGrid 某一列；参数未就绪时返回 0。
/// </summary>
public class HeatColumnOffsetConverter : IMultiValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static HeatColumnOffsetConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        return values is [int column, int columns, double width, ..] && columns > 0
            ? width * column / columns
            : 0d;
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();
}
