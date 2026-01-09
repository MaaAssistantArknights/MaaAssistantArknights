// <copyright file="AsstParadoxCopilotTask.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstParadoxCopilotTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.ParadoxCopilot;

    /// <summary>
    /// Gets or sets 多作业列表, 导航至关卡 (启用自动战斗序列、取消代理)
    /// </summary>
    [JsonProperty("list")]
    public List<string> MultiTasks { get; set; } = [];

    [JsonProperty("filename")]
    public string? FileName { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var json = new JObject();
        if (FileName is not null)
        {
            json["filename"] = FileName;
        }
        else if (MultiTasks.Count > 0)
        {
            json["list"] = JArray.FromObject(MultiTasks);
        }

        return (TaskType, json);
    }
}
