// <copyright file="ParadoxTask.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Models.AsstTasks.AsstCopilotTask;

namespace MaaWpfGui.Models.AsstTasks;

public class ParadoxTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Copilot;

    /// <summary>
    /// Gets or sets a value indicating whether 悖论模拟
    /// </summary>
    [JsonProperty("is_paradox")]
    public bool IsParadox { get; set; }

    /// <summary>
    /// Gets or sets 多作业列表, 导航至关卡 (启用自动战斗序列、取消代理), 不得与FileName同时使用
    /// </summary>
    public List<MultiTask> MultiTasks { get; set; } = [];

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        throw new NotImplementedException();
    }
}
