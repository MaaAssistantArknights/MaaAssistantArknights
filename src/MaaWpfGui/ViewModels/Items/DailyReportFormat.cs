// <copyright file="DailyReportFormat.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// 作战日报的比率数字格式。
/// </summary>
public static class DailyReportFormat
{
    /// <summary>
    /// 按数量级保留约三位有效数字：龙门币一类每场上千的值不带小数，单件理智一类远小于 1 的值不被截成 0。
    /// </summary>
    /// <param name="value">比率值。</param>
    /// <returns>格式化文本。</returns>
    public static string Rate(double value)
    {
        var culture = LocalizationHelper.CustomCultureInfo;
        var abs = Math.Abs(value);
        if (abs >= 100)
        {
            return value.ToString("N0", culture);
        }

        if (abs >= 10)
        {
            return value.ToString("0.0", culture);
        }

        if (abs >= 1 || abs == 0)
        {
            return value.ToString("0.00", culture);
        }

        var decimals = Math.Clamp(2 - (int)Math.Floor(Math.Log10(abs)), 2, 6);
        return value.ToString("0." + new string('0', decimals), culture);
    }
}
