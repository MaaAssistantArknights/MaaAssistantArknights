// <copyright file="DailyReportCalendarState.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 作战日报日历的日期格渲染状态，打开日历时整体快照一次，供日期格模板按日期查询。
/// </summary>
/// <param name="RangeFrom">当前查看时段起点（含），日期格在时段内即高亮。</param>
/// <param name="RangeTo">当前查看时段终点（含）。</param>
/// <param name="Today">游戏日的今日；与系统日期在 0:00 ~ 4:00 间不一致，故不用 Calendar 自带的今日高亮。</param>
/// <param name="RecordedDays">有作战记录的游戏日。</param>
public record DailyReportCalendarState(DateTime RangeFrom, DateTime RangeTo, DateTime Today, IReadOnlySet<DateTime> RecordedDays)
{
    /// <summary>
    /// 日期格在当前查看时段内的位置，决定高亮色带的圆角。
    /// </summary>
    /// <param name="date">日期格的日期。</param>
    /// <returns>不在时段内为 <see cref="RangePosition.None"/>。</returns>
    public RangePosition GetRangePosition(DateTime date)
    {
        date = date.Date;
        if (date < RangeFrom || date > RangeTo)
        {
            return RangePosition.None;
        }

        var isStart = date == RangeFrom;
        var isEnd = date == RangeTo;
        return (isStart, isEnd) switch
        {
            (true, true) => RangePosition.Single,
            (true, false) => RangePosition.Start,
            (false, true) => RangePosition.End,
            _ => RangePosition.Middle,
        };
    }

    /// <summary>
    /// 日期格在高亮色带中的位置。
    /// </summary>
    public enum RangePosition
    {
        /// <summary>不在时段内。</summary>
        None,

        /// <summary>时段仅一天。</summary>
        Single,

        /// <summary>时段首日。</summary>
        Start,

        /// <summary>时段中间。</summary>
        Middle,

        /// <summary>时段末日。</summary>
        End,
    }
}
