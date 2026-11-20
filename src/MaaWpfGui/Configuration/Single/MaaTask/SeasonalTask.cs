// <copyright file="SeasonalTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 赛季任务：盟约策略模式跑卫戍协议极速模拟，按局数循环通关。
/// </summary>
public class SeasonalTask : BaseTask
{
    /// <summary>
    /// Initializes a new instance of the <see cref="SeasonalTask"/> class.
    /// </summary>
    public SeasonalTask() => TaskType = TaskType.SeasonalTask;

    /// <summary>
    /// Gets or sets 通关循环局数，0 = 跳过任务。
    /// </summary>
    public int Rounds { get; set; } = 3;

    /// <summary>
    /// Gets or sets 大核心盟约（阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMajorBond { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 小核心盟约（非阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMinorBond { get; set; } = "精准";

    /// <summary>
    /// Gets or sets 每回合升级策略：下标 = 回合-1，值为该回合目标商店等级，0 = 该回合不升级。
    /// </summary>
    public List<int> UpgradePlan { get; set; } = [3, 4];
}
