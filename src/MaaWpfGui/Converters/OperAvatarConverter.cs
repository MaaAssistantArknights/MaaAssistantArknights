// <copyright file="OperAvatarConverter.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;

namespace MaaWpfGui.Converters;

/// <summary>
/// 将干员名（含别名解析）转换为头像（<see cref="OperAvatarHelper.GetOperAvatarByName"/>），
/// 用于干员选择框收起态的头像浮层等按条目显示头像的绑定场景；空名或解析不到干员时返回 <c>null</c>，由布局占位对齐。
/// </summary>
public class OperAvatarConverter : IValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static OperAvatarConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        var name = value?.ToString();
        return string.IsNullOrEmpty(name) ? null : OperAvatarHelper.GetOperAvatarByName(name);
    }

    /// <inheritdoc/>
    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) => Binding.DoNothing;
}
