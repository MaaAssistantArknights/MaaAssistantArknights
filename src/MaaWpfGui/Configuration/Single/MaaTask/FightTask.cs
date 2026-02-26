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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Utilities;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 理智作战
/// </summary>
public class FightTask : BaseTask, IJsonOnDeserialized
{
    public FightTask() => TaskType = TaskType.Fight;

    /// <summary>
    /// Gets or sets a value indicating whether 是否使用理智药
    /// </summary>
    public bool? UseMedicine { get; set; } = false;

    /// <summary>
    /// Gets or sets 理智药数量
    /// </summary>
    public int MedicineCount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否碎石
    /// </summary>
    public bool? UseStone { get; set; } = false;

    /// <summary>
    /// Gets or sets 碎石数量
    /// </summary>
    public int StoneCount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否启用指定掉落
    /// </summary>
    public bool? EnableTargetDrop { get; set; } = false;

    /// <summary>
    /// Gets or sets 掉落物品id
    /// </summary>
    public string DropId { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 掉落数量
    /// </summary>
    public int DropCount { get; set; }

    /// <summary>
    /// Gets or sets 是否启用次数限制
    /// </summary>
    public bool? EnableTimesLimit { get; set; } = false;

    /// <summary>
    /// Gets or sets 最大战斗次数
    /// </summary>
    public int TimesLimit { get; set; } = int.MaxValue;

    /// <summary>
    /// Gets or sets 代理倍率
    /// </summary>
    public int Series { get; set; }

    /// <summary>
    /// Gets or sets 关卡列表, 从上往下选择第一个可用关卡
    /// </summary>
    public List<string> StagePlan { get; set; } = [string.Empty];

    /// <summary>
    /// Gets or sets a value indicating whether 博朗台模式
    /// </summary>
    public bool IsDrGrandet { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 使用临期药
    /// </summary>
    public bool UseExpiringMedicine { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 剿灭关卡自定
    /// </summary>
    public bool UseCustomAnnihilation { get; set; }

    /// <summary>
    /// Gets or sets 剿灭关卡
    /// </summary>
    public string AnnihilationStage { get; set; } = "Annihilation";

    /// <summary>
    /// Gets or sets a value indicating whether 隐藏不可用关卡
    /// </summary>
    public bool HideUnavailableStage { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 手动输入关卡
    /// </summary>
    public bool IsStageManually { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 使用备选关卡
    /// </summary>
    public bool UseOptionalStage { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 允许保存碎石状态
    /// </summary>
    public bool UseStoneAllowSave { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 隐藏代理倍率
    /// </summary>
    public bool HideSeries { get; set; }

    public FightStageResetMode StageResetMode { get; set; } = FightStageResetMode.Current;

    /// <summary>
    /// Gets or sets a value indicating whether 是否启用周计划
    /// </summary>
    public bool UseWeeklySchedule { get; set; }

    /// <summary>
    /// Gets or sets 周计划。当 <see cref="UseWeeklySchedule"/> 为 false 时不参与序列化。
    /// </summary>
    [JsonPredict(nameof(UseWeeklySchedule))]
    public Dictionary<DayOfWeek, bool> WeeklySchedule { get; set; } = Enum.GetValues<DayOfWeek>().ToDictionary(i => i, _ => true);

    public void OnDeserialized()
    {
        if (UseStoneAllowSave == false)
        {
            UseStone = false;
        }
        if (!UseOptionalStage)
        {
            StagePlan = StagePlan.Count == 1 ? StagePlan : [.. StagePlan.Append(string.Empty).Take(1)];
        }
    }
}
