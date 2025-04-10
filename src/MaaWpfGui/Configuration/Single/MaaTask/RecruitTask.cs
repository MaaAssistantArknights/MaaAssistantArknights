// <copyright file="RecruitTask.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
    /// Gets or sets a value indicating whether 无招聘许可仍然刷新
    /// </summary>
    public bool ForceRefresh { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 不自动确认1星
    /// </summary>
    public bool NotChooseLevel1 { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认3星
    /// </summary>
    public bool ChooseLevel3 { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认4星
    /// </summary>
    public bool ChooseLevel4 { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 自动确认5星
    /// </summary>
    public bool ChooseLevel5 { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 3星使用01:00
    /// TODO 重命名? / 枚举
    /// </summary>
    public bool Level3Time0100 { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 3星使用07:40
    /// TODO 重命名?
    /// </summary>
    public bool Level3Time0740 { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 无倾向Tag时是否刷新3星
    /// TODO 重命名?
    /// </summary>
    public bool RefreshLevel3 { get; set; } = true;

    /// <summary>
    /// Gets or sets 3星首选Tag
    /// </summary>
    public string? Level3FirstTags { get; set; }

    /// <summary>
    /// Gets or sets 多Tag策略
    /// TODO 改枚举？
    /// </summary>
    public int ExtraTagStrategy { get; set; }
}
