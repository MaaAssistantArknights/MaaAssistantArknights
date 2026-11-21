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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 赛季任务：按指定局数循环通关卫戍协议极速模拟。
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
}
