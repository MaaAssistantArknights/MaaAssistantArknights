// <copyright file="SubtractConverter.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Windows.Data;

namespace MaaWpfGui.Converters;

/// <summary>
/// 双值减法转换器：返回 max(0, values[0] - values[1])，用于 ｢视口高度 - 固定区实际高度｣ 一类的剩余高度绑定，
/// 固定区随语言换行变高时自动收缩；固定区超过视口时结果为 0，由外层面板滚动兜底。
/// 任一值未就绪时返回 <see cref="double.PositiveInfinity"/>（不限制高度），避免初始化阶段被误压为 0。
/// </summary>
public class SubtractConverter : IMultiValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static SubtractConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        if (values.Length == 2 && values[0] is double minuend && values[1] is double subtrahend)
        {
            return Math.Max(0, minuend - subtrahend);
        }

        return double.PositiveInfinity;
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();
}
