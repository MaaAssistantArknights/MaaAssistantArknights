// <copyright file="ArkItem.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;

namespace MaaWpfGui.Models;

public class ArkItem
{
    [JsonPropertyName("classifyType")]
    public string ClassifyType { get; set; } = string.Empty;

    [JsonPropertyName("description")]
    public string Description { get; set; } = string.Empty;

    [JsonPropertyName("icon")]
    public string Icon { get; set; } = string.Empty;

    [JsonPropertyName("name")]
    public string Name { get; set; } = string.Empty;

    [JsonPropertyName("sortId")]
    public int SortId { get; set; }

    [JsonPropertyName("usage")]
    public string Usage { get; set; } = string.Empty;

    /// <summary>
    /// Gets the synthesis formula as a map of material id to required count, e.g. dual chips (1 catalyst + 2 chip packs).
    /// Absent for items that are not craftable.
    /// </summary>
    [JsonPropertyName("formula")]
    public Dictionary<string, int>? Formula { get; set; }
}
