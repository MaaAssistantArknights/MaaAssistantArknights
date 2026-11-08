// <copyright file="DailyReportChartBarViewModel.cs" company="MaaAssistantArknights">
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
using System.Windows;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 柱状图单根柱子。
/// </summary>
/// <param name="Index">柱子在图表中的序号，从 0 开始，用于刻度稀疏时按间隔取舍。</param>
/// <param name="Ratio">柱高占图表最大值的比例，0 ~ 1。</param>
/// <param name="ToolTip">悬停提示（时间段与数值；叠加药剂与源石时一并列出）。</param>
/// <param name="TickLabel">柱子正下方的横轴刻度文本，每根柱子都有；放不下时由视图按间隔稀疏。</param>
/// <param name="IsMajorTick">是否为主刻度（如跨年处的年份），稀疏时始终保留。</param>
/// <param name="ValueText">柱顶数值（缩写）；数值为 0 或叠加折线时为空，避免与折线互相遮挡。</param>
public record DailyReportChartBarViewModel(int Index, double Ratio, string ToolTip, string TickLabel, bool IsMajorTick, string ValueText)
{
    /// <summary>
    /// Gets 柱体所占的星号行高，与 <see cref="EmptyHeight"/> 共同按比例分配柱子所在列的高度。
    /// </summary>
    public GridLength FilledHeight => new(Ratio, GridUnitType.Star);

    /// <summary>
    /// Gets 柱体上方空白所占的星号行高。
    /// </summary>
    public GridLength EmptyHeight => new(1 - Ratio, GridUnitType.Star);
}
