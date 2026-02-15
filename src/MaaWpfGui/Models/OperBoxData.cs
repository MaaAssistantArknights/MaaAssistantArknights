// <copyright file="OperBoxData.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using Newtonsoft.Json;

namespace MaaWpfGui.Models;

public class OperBoxData
{
    public class OperData
    {
        [JsonProperty("id")]
        public string Id { get; set; } = null!; // char_2024_chyue

        /// <summary>
        /// Gets or sets 干员名称
        /// </summary>
        [JsonProperty("name")]
        public string Name { get; set; } = null!; // 重岳

        /// <summary>
        /// Gets or sets 精英化等级
        /// </summary>
        [JsonProperty("elite")]
        public int Elite { get; set; } = 0;

        /// <summary>
        /// Gets or sets 等级
        /// </summary>
        [JsonProperty("level")]
        public int Level { get; set; } = 0;

        /// <summary>
        /// Gets or sets a value indicating whether 是否拥有
        /// </summary>
        [JsonProperty("own")]
        public bool Own { get; set; } = false;

        /// <summary>
        /// Gets or sets 潜能
        /// </summary>
        [JsonProperty("potential")]
        public int Potential { get; set; } = 0;

        /// <summary>
        /// Gets or sets 星级
        /// </summary>
        [JsonProperty("rarity")]
        public int Rarity { get; set; } = 0;
    }
}
