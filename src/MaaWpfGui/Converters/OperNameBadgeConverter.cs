// <copyright file="OperNameBadgeConverter.cs" company="MaaAssistantArknights">
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
/// 将干员名转换为「头像 + 干员名」一体的展示元素（<see cref="OperAvatarHelper.CreateOperBadgeByName"/>），
/// 用于下拉列表等需要按条目调用方法生成元素的绑定场景。
/// </summary>
public class OperNameBadgeConverter : IValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static OperNameBadgeConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        var name = value?.ToString() ?? string.Empty;
        var avatarSize = parameter is string text && double.TryParse(text, out var size) ? size : 18;
        return OperAvatarHelper.CreateOperBadgeByName(name, avatarSize);
    }

    /// <inheritdoc/>
    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) => Binding.DoNothing;
}
