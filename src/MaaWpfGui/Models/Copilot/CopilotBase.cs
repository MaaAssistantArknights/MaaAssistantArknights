// <copyright file="CopilotBase.cs" company="MaaAssistantArknights">
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

#nullable enable
using Newtonsoft.Json;

namespace MaaWpfGui.Models.Copilot;

public class CopilotBase
{
    /// <summary>
    /// Gets or sets 最低要求 maa 版本号，必选。保留字段，暂未实现。
    /// </summary>
    [JsonProperty("minimum_required")]
    public string MinimumRequired { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 描述信息。
    /// </summary>
    [JsonProperty("doc")]
    public Doc? Documentation { get; set; }

    public class Doc
    {
        [JsonProperty("title")]
        public string? Title { get; set; }

        [JsonProperty("title_color")]
        public string? TitleColor { get; set; }

        [JsonProperty("details")]
        public string? Details { get; set; }

        [JsonProperty("details_color")]
        public string? DetailsColor { get; set; }
    }
}
