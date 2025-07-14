// <copyright file="RecruitTask.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 公招
/// </summary>
public class RecruitTask : BaseTask
{
    public RecruitTask() => TaskType = TaskType.Recruit;

    /// <summary>
    /// Gets or sets a value indicating whether 是否使用公招加速卷
    /// </summary>
    [JsonIgnore]
    public bool UseExpedited { get; set; }

    /// <summary>
    /// Gets or sets 单轮最大公招次数
    /// </summary>
    public int MaxTimes { get; set; } = 4;

    /// <summary>
    /// Gets or sets 多Tag策略
    /// TODO 改枚举？
    /// </summary>
    public int ExtraTagMode { get; set; }

    /// <summary>
    /// Gets or sets 3星首选Tag
    /// </summary>
    public List<string> Level3PreferTags { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 无倾向Tag时是否刷新3星
    /// TODO 重命名?
    /// </summary>
    public bool RefreshLevel3 { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 无招聘许可仍然刷新
    /// </summary>
    public bool ForceRefresh { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 不自动确认1星
    /// </summary>
    public bool Level1NotChoose { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认3星
    /// </summary>
    public bool Level3Choose { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认4星
    /// </summary>
    public bool Level4Choose { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认5星
    /// </summary>
    public bool Level5Choose { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 3星时间
    /// </summary>
    public int Level3Time { get; set; } = 540;

    /// <summary>
    /// Gets or sets 4星时间
    /// </summary>
    public int Level4Time { get; set; } = 540;

    /// <summary>
    /// Gets or sets 5星时间
    /// </summary>
    public int Level5Time { get; set; } = 540;
}
