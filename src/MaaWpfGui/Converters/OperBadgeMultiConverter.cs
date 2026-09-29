// <copyright file="OperBadgeMultiConverter.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;

namespace MaaWpfGui.Converters;

/// <summary>
/// 将 (干员 ID, 展示文本) 两个绑定值合成为「头像 + 干员名」一体的展示元素
/// （<see cref="OperAvatarHelper.CreateOperBadge"/>），用于下拉列表等条目同时持有干员 ID 与展示文本的场景。
/// </summary>
public class OperBadgeMultiConverter : IMultiValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static OperBadgeMultiConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
    {
        var operId = values.Length > 0 ? values[0]?.ToString() ?? string.Empty : string.Empty;
        var display = values.Length > 1 ? values[1]?.ToString() ?? string.Empty : string.Empty;
        return OperAvatarHelper.CreateOperBadge(operId, display);
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture) => [];
}
