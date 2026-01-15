// <copyright file="AwardTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public class AwardTask : BaseTask
{
    public AwardTask() => TaskType = TaskType.Award;

    /// <summary>
    /// Gets or sets a value indicating whether 领取日常奖励
    /// </summary>
    public bool Award { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 领取邮件奖励
    /// </summary>
    public bool Mail { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 领取每日免费一抽
    /// </summary>
    public bool FreeGacha { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 领取幸运墙合成玉
    /// </summary>
    public bool Orundum { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 领挖矿合成玉
    /// </summary>
    public bool Mining { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 5周年特殊月卡
    /// </summary>
    public bool SpecialAccess { get; set; }
}
