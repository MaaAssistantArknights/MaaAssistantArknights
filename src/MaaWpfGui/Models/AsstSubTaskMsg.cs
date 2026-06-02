// <copyright file="AsstSubTaskMsg.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

public class AsstSubTaskMsg
{
    [JsonProperty("taskchain")]
    public string TaskChain { get; set; } = string.Empty;

    [JsonProperty("taskid")]
    public int TaskId { get; set; } = 0;

    [JsonProperty("class")]
    public string Class { get; set; } = string.Empty;

    [JsonProperty("subtask")]
    public string SubTask { get; set; } = string.Empty;

    [JsonProperty("what")]
    public string? What { get; set; } // 大部分为what

    [JsonProperty("why")]
    public string? Why { get; set; } // 极少量 SubTaskError 会有why

    [JsonProperty("uuid")]
    public string UUID { get; set; } = string.Empty;

    [JsonProperty("details")]
    public JObject? Details { get; set; }
}
