// <copyright file="DepotMaintainPresetCatalog.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Linq;

namespace MaaWpfGui.Models;

/// <summary>
/// Pure editable reserve defaults, independent of UI or account configuration.
/// </summary>
internal static class DepotMaintainPresetCatalog
{
    // Convenience order, not a universal farming-efficiency ranking.
    // Unknown/new materials retain their relative order after known defaults.
    internal static int Priority(string itemId)
    {
        if (itemId == "4001")
        {
            return 0;
        }

        if (itemId is "2001" or "2002" or "2003" or "2004")
        {
            return 1;
        }

        if (itemId is "3301" or "3302" or "3303")
        {
            return 2;
        }

        if (itemId.Length == 4 && itemId.StartsWith("32", StringComparison.Ordinal)
            && itemId[2] is >= '1' and <= '8' && itemId[3] is '1' or '2')
        {
            return 3;
        }

        if (Tier3Materials.Any(row => row.Drops.Contains(itemId)))
        {
            return 4;
        }

        if (itemId == "4006")
        {
            return 5;
        }

        return 6;
    }

    internal static IEnumerable<T> Ordered<T>(IEnumerable<T> plans, Func<T, string> itemId, bool useDefaults)
        => useDefaults ? plans.OrderBy(plan => Priority(itemId(plan))) : plans;

    /// <summary>
    /// Gets final held-stock defaults and reviewed early main-story routes.
    /// This is not a training bill, an efficiency ranking or a recipe expansion.
    /// A fresh array prevents one caller from changing later preset additions.
    /// </summary>
    internal static (string Stage, string[] Drops, int DefaultCount)[] Tier3Materials => [
        ("2-4", ["30013"], 20),   // 固源岩组
        ("2-5", ["30023"], 20),   // 糖组
        ("2-6", ["30033"], 20),   // 聚酸酯组
        ("2-8", ["30043"], 20),   // 异铁组
        ("3-1", ["30053"], 20),   // 酮凝集组
        ("3-4", ["30063"], 20),   // 全新装置
        ("2-9", ["30073"], 20),   // 扭转醇
        ("3-2", ["30083"], 20),   // 轻锰矿
        ("3-3", ["30093"], 20),   // 研磨石
        ("2-10", ["30103"], 20),  // RMA70-12
        ("7-8", ["31013"], 20),   // 凝胶
        ("6-12", ["31023"], 20),  // 炽合金
        ("9-14", ["31033"], 20),  // 晶体元件
        ("9-4", ["31043"], 20),   // 半自然溶剂
        ("9-6", ["31053"], 20),   // 化合切削液
        ("11-3", ["31063"], 20),  // 转质盐组
        ("13-5", ["31073"], 20),  // 褐素纤维
        ("13-15", ["31083"], 20), // 环烃聚质
        ("15-9", ["31093"], 20),  // 类凝结核
        ("17-5", ["31103"], 20),  // 液化高能气体
        ("17-11", ["31113"], 20), // 电极单元
    ];
}
