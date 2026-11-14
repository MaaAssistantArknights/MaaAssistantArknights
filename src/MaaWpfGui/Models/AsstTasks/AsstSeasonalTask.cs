// <copyright file="AsstSeasonalTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 赛季任务（当前含卫戍协议，争锋频道暂未实现）
/// </summary>
public class AsstSeasonalTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.SeasonalTask;

    /// <summary>
    /// Gets or sets 卫戍协议（极速模拟）轮数，打满后结束；0 表示跳过。
    /// </summary>
    [JsonProperty("spa_rounds")]
    public int SpaRounds { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
