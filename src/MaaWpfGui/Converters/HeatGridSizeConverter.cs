// <copyright file="HeatGridSizeConverter.cs" company="MaaAssistantArknights">
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
/// 多值转换器：按容器可用尺寸给热力图定正方形格距，使格子横竖间距一致、整体不被拉散。
/// values[0] 为可用宽度，values[1] 为可用高度，values[2] 为列数；行数固定为一周 7 天。
/// 格距取宽、高两个方向能容纳的较小值；参数为 <c>Width</c> 时返回网格总宽（格距 × 列数），为 <c>Cell</c> 时返回格子边长（格距减去格间空隙）。
/// 参数未就绪时返回 0。
/// </summary>
public class HeatGridSizeConverter : IMultiValueConverter
{
    private const int Rows = 7;

    // 格子之间的空隙
    private const double Gap = 3;

    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static HeatGridSizeConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        if (values is not [double width, double height, int columns, ..] || columns <= 0 || width <= 0 || height <= 0)
        {
            return 0d;
        }

        // 取整避免格子落在半像素上发虚
        var pitch = Math.Floor(Math.Min(width / columns, height / Rows));
        return parameter as string == "Cell" ? Math.Max(1, pitch - Gap) : pitch * columns;
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();
}
