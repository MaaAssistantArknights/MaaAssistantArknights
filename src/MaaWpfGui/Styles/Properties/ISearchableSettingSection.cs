// <copyright file="ISearchableSettingSection.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 可被 <see cref="SearchHighlightBehavior"/> 搜索过滤的设置分区抽象。
/// 真实设置页由 <see cref="ViewModels.Items.SettingItemViewModel"/> 实现，教程沙盒设置页由对应分区模型实现，
/// 行为只依赖这三个成员，不耦合具体分区类型。
/// </summary>
public interface ISearchableSettingSection
{
    /// <summary>
    /// Gets 分区标题显示文本（搜索匹配源之一）。
    /// </summary>
    string Display { get; }

    /// <summary>
    /// Gets or sets a value indicating whether 搜索过滤后本分区仍可见。
    /// </summary>
    bool IsVisibleInSearch { get; set; }

    /// <summary>
    /// Gets 分区在内容 Grid 中的行号（用于把视觉分区容器映射回分区模型）。
    /// </summary>
    int GridRow { get; }
}
