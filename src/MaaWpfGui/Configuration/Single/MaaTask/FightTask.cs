// <copyright file="FightTask.cs" company="MaaAssistantArknights">
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

/// <summary>
/// 刷理智
/// </summary>
public class FightTask : BaseTask
{
    public FightTask() => TaskType = TaskType.Fight;

    /// <summary>
    /// Gets or sets a value indicating whether 是否使用理智药
    /// </summary>
    public bool UseMedicine { get; set; }

    /// <summary>
    /// Gets or sets 理智药数量
    /// </summary>
    public int MedicineCount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否碎石
    /// </summary>
    public bool UseStone { get; set; }

    /// <summary>
    /// Gets or sets 碎石数量
    /// </summary>
    public int StoneCount { get; set; }

    /// <summary>
    /// Gets or sets 首选关卡
    /// </summary>
    public string Stage1 { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 备选关卡
    /// </summary>
    public string Stage2 { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 备选关卡
    /// </summary>
    public string Stage3 { get; set; } = string.Empty;
}
