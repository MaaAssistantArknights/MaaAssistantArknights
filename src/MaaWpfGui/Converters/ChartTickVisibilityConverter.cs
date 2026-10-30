// <copyright file="ChartTickVisibilityConverter.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using MaaWpfGui.ViewModels.Items;

namespace MaaWpfGui.Converters;

/// <summary>
/// 多值转换器：决定柱状图某根柱子下的横轴刻度是否显示，刻度放不下时按固定间隔稀疏。
/// values[0] 为该柱的 <see cref="DailyReportChartBarViewModel"/>，values[1] 为全部柱子，values[2] 为刻度行的实际宽度。
/// 间隔取能容下最宽刻度的最小值；主刻度（如跨年处的年份）始终显示，与主刻度相距不足一个间隔的普通刻度隐藏以免重叠。
/// 返回 Hidden 而非 Collapsed，避免刻度行高度随显隐变化。
/// </summary>
public class ChartTickVisibilityConverter : IMultiValueConverter
{
    // 与刻度文字的字号一致，用于估算刻度宽度
    private const double TickFontSize = 11;

    // 相邻刻度之间至少留出的空白
    private const double TickGap = 6;

    private static readonly int[] _strides = [1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 24, 30];

    // 每根柱子各求值一次，最宽刻度按柱子集合缓存，避免每根柱子都把全部刻度测量一遍
    private static IReadOnlyList<DailyReportChartBarViewModel>? _measuredBars;
    private static double _widestTick;

    /// <summary>
    /// Gets 共享实例，供 XAML 引用。
    /// </summary>
    public static ChartTickVisibilityConverter Instance { get; } = new();

    /// <inheritdoc/>
    public object Convert(object[] values, Type targetType, object? parameter, CultureInfo culture)
    {
        if (values is not [DailyReportChartBarViewModel bar, IReadOnlyList<DailyReportChartBarViewModel> bars, double width, ..]
            || bars.Count == 0 || width <= 0 || string.IsNullOrEmpty(bar.TickLabel))
        {
            return Visibility.Hidden;
        }

        if (bar.IsMajorTick)
        {
            return Visibility.Visible;
        }

        var stride = GetStride(bars, width / bars.Count);
        if (bar.Index % stride != 0)
        {
            return Visibility.Hidden;
        }

        var nearMajor = bars.Any(b => b.IsMajorTick && b.Index != bar.Index && Math.Abs(b.Index - bar.Index) < stride);
        return nearMajor ? Visibility.Hidden : Visibility.Visible;
    }

    /// <inheritdoc/>
    public object[] ConvertBack(object? value, Type[] targetTypes, object? parameter, CultureInfo culture) => throw new NotSupportedException();

    private static int GetStride(IReadOnlyList<DailyReportChartBarViewModel> bars, double columnWidth)
    {
        var need = GetWidestTick(bars) + TickGap;
        foreach (var stride in _strides)
        {
            if (columnWidth * stride >= need)
            {
                return stride;
            }
        }

        return _strides[^1];
    }

    private static double GetWidestTick(IReadOnlyList<DailyReportChartBarViewModel> bars)
    {
        if (ReferenceEquals(bars, _measuredBars))
        {
            return _widestTick;
        }

        var typeface = new Typeface(SystemFonts.MessageFontFamily, FontStyles.Normal, FontWeights.Normal, FontStretches.Normal);
        _widestTick = bars
            .Where(b => !string.IsNullOrEmpty(b.TickLabel))
            .Select(b => new FormattedText(b.TickLabel, CultureInfo.CurrentUICulture, FlowDirection.LeftToRight, typeface, TickFontSize, Brushes.Black, 1).Width)
            .DefaultIfEmpty(0)
            .Max();
        _measuredBars = bars;
        return _widestTick;
    }
}
