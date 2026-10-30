// <copyright file="DailyReportCalendarDayConverter.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using MaaWpfGui.ViewModels.Items;
using RangePosition = MaaWpfGui.ViewModels.Items.DailyReportCalendarState.RangePosition;

namespace MaaWpfGui.Converters;

/// <summary>
/// 作战日报日历日期格转换器：values[0] 为日期格日期，values[1] 为 <see cref="DailyReportCalendarState"/>，
/// ConverterParameter 指定输出项：
/// <c>Record</c> 有记录角标的 Visibility；<c>Today</c> 今日圆环的 Visibility；
/// <c>Range</c> 选中形态：时段仅一天为 <c>Single</c>（画圆），多日为 <c>Band</c>（画色带），不在时段内为 <c>None</c>；
/// <c>RangeCorner</c> 色带的 CornerRadius（首尾日外侧圆角，中间日直角，连成一条）。
/// </summary>
public class DailyReportCalendarDayConverter : IMultiValueConverter
{
    private const double Radius = 16;

    /// <summary>
    /// Gets 共享实例，供 XAML 静态资源引用。
    /// </summary>
    public static DailyReportCalendarDayConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        var kind = parameter as string;
        if (values is not [DateTime date, DailyReportCalendarState state, ..])
        {
            return kind switch
            {
                "Range" => "None",
                "RangeCorner" => new CornerRadius(Radius),
                _ => Visibility.Collapsed,
            };
        }

        return kind switch
        {
            "Record" => state.RecordedDays.Contains(date.Date) ? Visibility.Visible : Visibility.Collapsed,
            "Today" => date.Date == state.Today ? Visibility.Visible : Visibility.Collapsed,
            "Range" => state.GetRangePosition(date) switch
            {
                RangePosition.None => "None",
                RangePosition.Single => "Single",
                _ => "Band",
            },
            "RangeCorner" => state.GetRangePosition(date) switch
            {
                RangePosition.Start => new CornerRadius(Radius, 0, 0, Radius),
                RangePosition.Middle => new CornerRadius(0),
                RangePosition.End => new CornerRadius(0, Radius, Radius, 0),
                _ => new CornerRadius(Radius),
            },
            _ => Visibility.Collapsed,
        };
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();
}
