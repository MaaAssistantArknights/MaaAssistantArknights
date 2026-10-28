// <copyright file="DailyReportRowViewModel.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 作战日报单条记录行：日维度为一条 Fight 链，周/月/年为同一关卡的聚合。
/// </summary>
public class DailyReportRowViewModel
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportRowViewModel"/> class.
    /// </summary>
    /// <param name="stage">关卡编号。</param>
    /// <param name="timeText">时间文本（日维度为起止钟点，聚合维度为空）。</param>
    /// <param name="metaText">场次与资源消耗文本。</param>
    /// <param name="drops">掉落合计。</param>
    /// <param name="times">本行场次，用于计算每场掉落。</param>
    /// <param name="sanity">本行理智消耗，用于计算单件理智。</param>
    public DailyReportRowViewModel(string stage, string timeText, string metaText, Dictionary<string, int> drops, int times, int sanity)
    {
        Stage = stage;
        TimeText = timeText;
        MetaText = metaText;
        Drops = drops.OrderByDescending(d => d.Value)
            .Select(d => new DailyReportDropItemViewModel(d.Key, d.Value, stage, times, sanity))
            .ToList();
    }

    /// <summary>
    /// Gets 关卡编号。
    /// </summary>
    public string Stage { get; }

    /// <summary>
    /// Gets 时间文本。
    /// </summary>
    public string TimeText { get; }

    /// <summary>
    /// Gets 场次与资源消耗文本。
    /// </summary>
    public string MetaText { get; }

    /// <summary>
    /// Gets 掉落列表（按数量降序）。
    /// </summary>
    public IReadOnlyList<DailyReportDropItemViewModel> Drops { get; }
}
