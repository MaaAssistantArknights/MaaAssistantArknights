// <copyright file="FitsWidthToVisibilityConverter.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Data;

namespace MaaWpfGui.Converters;

/// <summary>
/// 多值转换器：values[0] 为内容实际宽度，values[1] 为可用宽度，
/// 内容不超过可用宽度时返回 <see cref="Visibility.Visible"/>，否则 <see cref="Visibility.Hidden"/>。
/// 返回 Hidden 而非 Collapsed：Collapsed 会让内容宽度归零，下次测量时又判为可显示，导致显隐来回振荡。
/// </summary>
public class FitsWidthToVisibilityConverter : IMultiValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static FitsWidthToVisibilityConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        return values is [double content, double available, ..] && content <= available
            ? Visibility.Visible
            : Visibility.Hidden;
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();
}
