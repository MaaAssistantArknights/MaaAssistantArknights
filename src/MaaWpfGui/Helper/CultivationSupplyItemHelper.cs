// <copyright file="CultivationSupplyItemHelper.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

public static class CultivationSupplyItemHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(CultivationSupplyItemHelper));

    // Restrict equivalence to the same class's dual chip. General crafting recipes are not reversible.
    private static readonly Dictionary<string, (string DualChipId, string Stage)> _chipPacks = new() {
        ["3212"] = ("3213", "PR-C-2"),
        ["3222"] = ("3223", "PR-D-2"),
        ["3232"] = ("3233", "PR-A-2"),
        ["3242"] = ("3243", "PR-B-2"),
        ["3252"] = ("3253", "PR-B-2"),
        ["3262"] = ("3263", "PR-A-2"),
        ["3272"] = ("3273", "PR-C-2"),
        ["3282"] = ("3283", "PR-D-2"),
    };

    public static Dictionary<string, int>? ReadInventory(JObject details)
    {
        try
        {
            var data = details["data"];
            if (data?.Type == JTokenType.String)
            {
                data = JObject.Parse(data.Value<string>() ?? string.Empty);
            }

            if (data is not JObject items)
            {
                return null;
            }

            var inventory = new Dictionary<string, int>();
            foreach (var item in items.Properties())
            {
                if (string.IsNullOrEmpty(item.Name) || item.Value.Type != JTokenType.Integer ||
                    !int.TryParse(item.Value.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var count) ||
                    count < 0)
                {
                    return null;
                }

                inventory[item.Name] = count;
            }

            return inventory.Count > 0 ? inventory : null;
        }
        catch (JsonException ex)
        {
            _logger.Warning(ex, "Invalid inventory result for cultivation supply");
            return null;
        }
    }

    public static string? GetStage(string itemId) =>
        _chipPacks.TryGetValue(itemId, out var item) ? item.Stage : null;

    public static int GetEffectiveCount(string itemId, IReadOnlyDictionary<string, int> inventory)
    {
        if (!_chipPacks.TryGetValue(itemId, out var item))
        {
            return 0;
        }

        var raw = inventory.TryGetValue(itemId, out var count) ? Math.Max(0, count) : 0;
        var dual = inventory.TryGetValue(item.DualChipId, out count) ? Math.Max(0, count) : 0;
        return (int)Math.Min(int.MaxValue, (long)raw + (2L * dual));
    }

    public static string? GetDualChipId(string itemId) =>
        _chipPacks.TryGetValue(itemId, out var item) ? item.DualChipId : null;
}
