// <copyright file="DailyReportDropItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Windows.Media;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 单个掉落物展示项（图标 + 名称 + 数量）。
/// </summary>
public class DailyReportDropItemViewModel
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportDropItemViewModel"/> class.
    /// </summary>
    /// <param name="itemId">物品 id。</param>
    /// <param name="count">数量。</param>
    /// <param name="stage">所属记录行的关卡。</param>
    /// <param name="times">所属记录行的场次。</param>
    /// <param name="sanity">所属记录行的理智消耗。</param>
    public DailyReportDropItemViewModel(string itemId, int count, string stage, int times, int sanity)
    {
        ItemId = itemId;
        Stage = stage;
        Name = ItemListHelper.GetItemName(itemId) ?? itemId;
        Count = count.FormatNumber(false);
        Image = ItemListHelper.GetItemImage(itemId);
        RateTitle = $"{Name} · {stage}";

        // 场次或理智缺失（旧数据、采集异常）时无从计算，显示占位而非 0 或无穷
        PerBattleText = times > 0 ? DailyReportFormat.Rate((double)count / times) : "-";
        SanityPerItemText = count > 0 && sanity > 0 ? DailyReportFormat.Rate((double)sanity / count) : "-";
    }

    /// <summary>
    /// Gets 物品 id。
    /// </summary>
    public string ItemId { get; }

    /// <summary>
    /// Gets 所属记录行的关卡。
    /// </summary>
    public string Stage { get; }

    /// <summary>
    /// Gets or sets 浮层里的排名摘要（名次、参与排名的关卡数与误差），打开浮层时按当前时段全部记录计算；
    /// 日视图里浮层上方的两项数值只算这一条记录，二者口径不同。无从排名时为空。
    /// </summary>
    public string RankText { get; set; } = string.Empty;

    /// <summary>
    /// Gets 物品名称。
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets 悬停提示（物品名，名称在格内可能被截断）。
    /// </summary>
    public string ToolTip => Name;

    /// <summary>
    /// Gets 掉率浮层标题（物品名 · 关卡）。
    /// </summary>
    public string RateTitle { get; }

    /// <summary>
    /// Gets 每场平均掉落数（掉落数 ÷ 场次）。
    /// </summary>
    public string PerBattleText { get; }

    /// <summary>
    /// Gets 单件平均理智消耗（理智消耗 ÷ 掉落数）。
    /// </summary>
    public string SanityPerItemText { get; }

    /// <summary>
    /// Gets 数量文本。
    /// </summary>
    public string Count { get; }

    /// <summary>
    /// Gets 物品图标；无图标的物品为 null。
    /// </summary>
    public ImageSource? Image { get; }
}
