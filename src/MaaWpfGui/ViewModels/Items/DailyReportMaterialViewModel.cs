// <copyright file="DailyReportMaterialViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 按材料视图的单个物品格：某种物品在当前时段的总获取量，来源关卡放在悬停提示。
/// </summary>
public class DailyReportMaterialViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportMaterialViewModel"/> class.
    /// </summary>
    /// <param name="itemId">物品 id。</param>
    /// <param name="sources">各来源关卡的获取量。</param>
    /// <param name="isSelected">是否为柱状图当前展示的物品。</param>
    public DailyReportMaterialViewModel(string itemId, Dictionary<string, int> sources, bool isSelected)
    {
        ItemId = itemId;
        IsSelected = isSelected;
        Name = ItemListHelper.GetItemName(itemId) ?? itemId;
        Image = ItemListHelper.GetItemImage(itemId);
        Count = sources.Values.Sum().FormatNumber(false);

        var sourceLines = sources
            .OrderByDescending(s => s.Value)
            .ThenBy(s => s.Key, StringComparer.Ordinal)
            .Select(s => $"{s.Key} ×{s.Value.FormatNumber(false)}");
        ToolTip = string.Join("\n", sourceLines.Prepend(Name));
    }

    /// <summary>
    /// Gets 物品 id。
    /// </summary>
    public string ItemId { get; }

    /// <summary>
    /// Gets 物品名称。
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets 物品图标；无图标的物品为 null。
    /// </summary>
    public ImageSource? Image { get; }

    /// <summary>
    /// Gets 总获取量文本。
    /// </summary>
    public string Count { get; }

    /// <summary>
    /// Gets 悬停提示：首行物品名，其后每行一个来源关卡及获取量（按获取量降序）。
    /// </summary>
    public string ToolTip { get; }

    /// <summary>
    /// Gets or sets a value indicating whether 该物品为柱状图当前展示的物品。
    /// </summary>
    public bool IsSelected { get; set => SetAndNotify(ref field, value); }
}
