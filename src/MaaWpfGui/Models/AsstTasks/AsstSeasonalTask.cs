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
using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 赛季任务（盟约策略通关循环）。
/// </summary>
public class AsstSeasonalTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.SeasonalTask;

    /// <summary>
    /// Gets or sets 通关循环局数。
    /// </summary>
    public int Rounds { get; set; } = 3;

    /// <summary>
    /// Gets or sets 大核心盟约（阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMajorBond { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 小核心盟约（非阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMinorBond { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 每回合升级计划：下标 = 回合-1，值为该回合目标商店等级，0 = 该回合不升级。
    /// </summary>
    public List<int> UpgradePlan { get; set; } = [];

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var data = new JObject
        {
            ["rounds"] = Rounds,
            ["core_major_bond"] = CoreMajorBond,
            ["core_minor_bond"] = CoreMinorBond,
            ["upgrade_plan"] = JArray.FromObject(UpgradePlan),
        };

        return (TaskType, data);
    }
}
