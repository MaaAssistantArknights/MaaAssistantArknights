// <copyright file="ItemImageConverter.cs" company="MaaAssistantArknights">
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
/// 将物品 ID 转换为物品图标（<see cref="ItemListHelper.GetItemImage"/>），
/// 用于材料选择下拉列表等按条目显示图标的绑定场景；无对应图标（含 ｢不选择｣ 空值项）时返回 <c>null</c>，由布局占位对齐。
/// </summary>
public class ItemImageConverter : IValueConverter
{
    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static ItemImageConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        return ItemListHelper.GetItemImage(value?.ToString() ?? string.Empty);
    }

    /// <inheritdoc/>
    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) => Binding.DoNothing;
}
