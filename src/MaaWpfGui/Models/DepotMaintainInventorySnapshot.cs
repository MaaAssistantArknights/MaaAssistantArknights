// <copyright file="DepotMaintainInventorySnapshot.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

/// <summary>
/// Exact display snapshot, deliberately separate from legacy Depot quantities.
/// Missing, withdrawn and unverified items remain unknown, including after reload.
/// Core alone validates actual drops; the GUI never adds drops to unknown stock.
/// </summary>
public sealed class DepotMaintainInventorySnapshot
{
    private Dictionary<string, long> _counts = [];
    private bool _complete;

    public void Clear()
    {
        _counts.Clear();
        _complete = false;
    }

    public long? Count(string itemId) => _complete && _counts.TryGetValue(itemId, out var value) ? value : null;

    public bool Replace(JToken? data, bool complete)
    {
        // Invalidate first: a bad or partial replacement cannot leave old exact stock.
        Clear();
        if (data?.Type == JTokenType.String)
        {
            var text = data.Value<string>();
            if (text is null || text.Length > 1024 * 1024)
            {
                return false;
            }
            try
            {
                data = JObject.Parse(text, new JsonLoadSettings { DuplicatePropertyNameHandling = DuplicatePropertyNameHandling.Error });
            }
            catch (JsonException)
            {
                return false;
            }
        }
        if (data is not JObject values || values.Count > 4096)
        {
            return false;
        }
        Dictionary<string, long> parsed = [];
        foreach (var property in values.Properties())
        {
            if (string.IsNullOrEmpty(property.Name) || property.Value.Type != JTokenType.Integer ||
                !long.TryParse(property.Value.ToString(Formatting.None), NumberStyles.None, CultureInfo.InvariantCulture, out var count))
            {
                return false;
            }
            parsed.Add(property.Name, count);
        }
        _counts = parsed;
        _complete = complete;
        return true;
    }

    public JObject? Export() => _complete ? JObject.FromObject(_counts) : null;
}
