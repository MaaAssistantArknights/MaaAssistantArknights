// <copyright file="GpuOptionItem.cs" company="MaaAssistantArknights">
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
using Stylet;

namespace MaaWpfGui.Helper;

/// <summary>
/// GPU 选项的 ComboBox 视图项。用 <see cref="Display"/>（本地化字符串）承载显示文本，
/// 用 <see cref="Value"/>（<see cref="GpuOption"/>）承载业务值。
/// 这样语言切换时只需刷新 <see cref="Display"/>，选中项引用保持稳定，
/// 避开 ComboBox 选中框依赖 <see cref="object.Equals(object)"/> 判等导致的刷新陷阱。
/// </summary>
public class GpuOptionItem(GpuOption option) : PropertyChangedBase
{
    public GpuOption Value { get; } = option;

    /// <summary>
    /// 透传底层 <see cref="GpuOption.IsDeprecated"/>，供 XAML 触发器绑定。
    /// </summary>
    public bool IsDeprecated => Value.IsDeprecated;

    private string _display = option?.ToString() ?? "NULL";

    public string Display
    {
        get => _display;
        set => SetAndNotify(ref _display, value);
    }

    /// <summary>
    /// 重新从底层 <see cref="GpuOption"/> 读取显示文本（语言切换时调用）。
    /// </summary>
    public void RefreshDisplay() => Display = Value?.ToString() ?? "NULL";
}
