// <copyright file="DailyReportHeatCellViewModel.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 年度热力图的单个日期格。
/// </summary>
/// <param name="Date">游戏日；年初补齐到周一的占位格为 null。</param>
/// <param name="Level">颜色档位：0 为当天无数据，1 ~ 4 按当年非零值的四分位递增。</param>
/// <param name="ToolTip">悬停提示（日期与数值）；占位格与今日之后的格子为 null，不弹提示。</param>
public record DailyReportHeatCellViewModel(DateOnly? Date, int Level, string? ToolTip)
{
    /// <summary>
    /// Gets 格子填充的不透明度（1 ~ 4 档）；无数据格由模板改用底色并强制不透明，不使用此值。
    /// </summary>
    public double Opacity => Level switch
    {
        1 => 0.2,
        2 => 0.4,
        3 => 0.65,
        _ => 1,
    };

    /// <summary>
    /// Gets a value indicating whether 该格是年初补齐用的占位格（不绘制）。
    /// </summary>
    public bool IsPlaceholder => Date is null;
}
