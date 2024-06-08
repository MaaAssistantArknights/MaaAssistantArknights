// <copyright file="ArkItem.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Diagnostics.CodeAnalysis;
using System.Text.Json.Serialization;

namespace MaaWpfGui.Models
{
    [SuppressMessage("ReSharper", "UnusedMember.Global")]
    public class ArkItem
    {
        [JsonPropertyName("classifyType")]
        public string ClassifyType { get; set; }

        [JsonPropertyName("description")]
        public string Description { get; set; }

        [JsonPropertyName("icon")]
        public string Icon { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("sortId")]
        public int SortId { get; set; }

        [JsonPropertyName("usage")]
        public string Usage { get; set; }
    }
}
