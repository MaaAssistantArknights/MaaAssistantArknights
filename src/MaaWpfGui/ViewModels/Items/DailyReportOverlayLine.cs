// <copyright file="DailyReportOverlayLine.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Media;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 柱状图上叠加的一条折线（药剂或源石），点坐标归一化到 0 ~ 1，由 <c>Path Stretch="Fill"</c> 拉伸到图表区域。
/// </summary>
public class DailyReportOverlayLine
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportOverlayLine"/> class.
    /// </summary>
    /// <param name="values">各柱对应的数值。</param>
    /// <param name="axisMax">右轴上限，两条线共用以便直接比较。</param>
    /// <param name="pointCount">实际绘制的点数（前若干个柱子）；时段未过完时其后的柱子不连线。</param>
    public DailyReportOverlayLine(IReadOnlyList<long> values, long axisMax, int pointCount)
    {
        var count = values.Count;
        var figure = new PathFigure { IsClosed = false, IsFilled = false };
        for (var i = 0; i < Math.Min(pointCount, count); i++)
        {
            // 每个点落在对应柱子中线；y 向下为正，数值越大越靠上
            var point = new Point((i + 0.5) / count, 1 - ((double)values[i] / axisMax));
            if (i == 0)
            {
                figure.StartPoint = point;
            }
            else
            {
                figure.Segments.Add(new LineSegment(point, true));
            }
        }

        // Stretch="Fill" 按几何的包围盒拉伸，补上两个不可见的角点，把坐标范围固定为整个 0 ~ 1 区域
        var frame = new PathGeometry();
        frame.Figures.Add(figure);
        frame.Figures.Add(new PathFigure(new Point(0, 0), [], false) { IsFilled = false });
        frame.Figures.Add(new PathFigure(new Point(1, 1), [], false) { IsFilled = false });
        Geometry = frame;
        Geometry.Freeze();

        // 只有一个点时连不成线（如本周周一、本月 1 号），不画也不显示图例
        IsVisible = pointCount >= 2 && values.Take(pointCount).Any(v => v > 0);
    }

    /// <summary>
    /// Gets 折线几何（含固定坐标范围的两个角点）。
    /// </summary>
    public Geometry Geometry { get; }

    /// <summary>
    /// Gets a value indicating whether 当前时段该数值不全为 0（全为 0 时不画这条线）。
    /// </summary>
    public bool IsVisible { get; }
}
