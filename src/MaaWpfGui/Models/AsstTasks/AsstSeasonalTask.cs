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
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 赛季任务（卫戍协议极速模拟通关循环）。
/// </summary>
public class AsstSeasonalTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.SeasonalTask;

    /// <summary>
    /// Gets or sets 通关循环局数。
    /// </summary>
    public int Rounds { get; set; } = 3;

    /// <summary>
    /// Gets or sets 角标目标通关次数（3 金 / 6 彩）。
    /// </summary>
    public int Target { get; set; } = 3;

    public StrongholdProtocolSettings StrategySettings { get; set; } = new()
    {
        Strategy = StrongholdProtocolStrategy.Covenant,
        CoreMinorBond = string.Empty,
        UpgradePlan = [],
    };

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var data = new JObject
        {
            ["rounds"] = Rounds,
            ["target"] = Target,
        };
        AsstStrongholdProtocolTask.SerializeStrategy(data, StrategySettings);

        return (TaskType, data);
    }
}
