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

public class SeasonalTask : BaseTask
{
    public SeasonalTask() => TaskType = TaskType.SeasonalTask;

    /// <summary>
    /// Gets or sets 卫戍协议（极速模拟）轮数，从进入对局到返回主界面记一轮，打满后结束；0 表示跳过本任务。
    /// </summary>
    public int SpaRounds { get; set; } = 3;
}
