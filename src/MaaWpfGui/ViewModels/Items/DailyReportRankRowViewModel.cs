// <copyright file="DailyReportRankRowViewModel.cs" company="MaaAssistantArknights">
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
using System.Windows;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 单件理智排名中的一个关卡：某种物品在该关卡平均每获得一个消耗的理智。
/// </summary>
public class DailyReportRankRowViewModel
{
    /// <summary>
    /// 掉落数低于此值时样本不足，排名淡化显示并排到末尾。
    /// </summary>
    public const int MinReliableCount = 10;

    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportRankRowViewModel"/> class.
    /// </summary>
    /// <param name="stage">关卡。</param>
    /// <param name="count">该物品在该关卡的掉落数。</param>
    /// <param name="times">该关卡的场次。</param>
    /// <param name="sanity">该关卡的理智消耗。</param>
    public DailyReportRankRowViewModel(string stage, int count, int times, int sanity)
    {
        Stage = stage;
        Count = count;
        SanityPerItem = (double)sanity / count;

        // 掉落数近似泊松分布，单件理智的相对误差约 1/√n，取 95% 置信度
        RelativeError = 1.96 / Math.Sqrt(count);
        ValueText = DailyReportFormat.Rate(SanityPerItem);
        ErrorText = "±" + RelativeError.ToString("P0", LocalizationHelper.CustomCultureInfo);
        ToolTip = LocalizationHelper.GetStringFormat(
            "TaskQueue.DailyReport.Rank.Tip",
            DailyReportFormat.Rate(SanityPerItem * Math.Max(0, 1 - RelativeError)),
            DailyReportFormat.Rate(SanityPerItem * (1 + RelativeError)),
            count.ToString("N0", LocalizationHelper.CustomCultureInfo),
            times.ToString("N0", LocalizationHelper.CustomCultureInfo));
    }

    /// <summary>
    /// Gets 关卡。
    /// </summary>
    public string Stage { get; }

    /// <summary>
    /// Gets 该物品在该关卡的掉落数。
    /// </summary>
    public int Count { get; }

    /// <summary>
    /// Gets 单件平均理智消耗。
    /// </summary>
    public double SanityPerItem { get; }

    /// <summary>
    /// Gets 单件理智 95% 置信区间的相对半宽。
    /// </summary>
    public double RelativeError { get; }

    /// <summary>
    /// Gets a value indicating whether 掉落数足够（不少于 <see cref="MinReliableCount"/>）。
    /// </summary>
    public bool IsReliable => Count >= MinReliableCount;

    /// <summary>
    /// Gets 单件理智文本。
    /// </summary>
    public string ValueText { get; }

    /// <summary>
    /// Gets 误差文本（相对值）。
    /// </summary>
    public string ErrorText { get; }

    /// <summary>
    /// Gets 悬停提示：置信区间、掉落数与场次。
    /// </summary>
    public string ToolTip { get; }

    /// <summary>
    /// Gets or sets a value indicating whether 该行是当前关注的关卡（从按关卡浮层跳转时的来源关卡）。
    /// </summary>
    public bool IsHighlighted { get; set; }

    /// <summary>
    /// Gets or sets 横条长度比例（0 ~ 1）：最划算关卡的单件理智与本关之比，排名生成后、显示前设置一次。
    /// </summary>
    public double BarRatio { get; set; }

    /// <summary>
    /// Gets 横条所占的星号列宽。
    /// </summary>
    public GridLength BarWidth => new(Math.Clamp(BarRatio, 0.01, 1), GridUnitType.Star);

    /// <summary>
    /// Gets 横条右侧空白所占的星号列宽。
    /// </summary>
    public GridLength RestWidth => new(1 - Math.Clamp(BarRatio, 0.01, 1), GridUnitType.Star);
}
