// <copyright file="DepotMaintainTask.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public class DepotMaintainTask : BaseTask
{
    public DepotMaintainTask() => TaskType = TaskType.DepotMaintain;

    /// <summary>
    /// 临期药阈值（天）：仅使用 N 天内将过期的理智药。
    /// </summary>
    public const int ExpiringMedicineDays = 2;

    public bool UpdateDepot { get; set; } = true;

    public bool IsStageManually { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 活动期间跳过整个库存保持任务。
    /// </summary>
    public bool SkipDuringActivity { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 资源全开放期间跳过整个库存保持任务。
    /// </summary>
    public bool SkipDuringResourceCollection { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 使用 AUTO 代理倍率（Series = 0）。
    /// 默认关闭（按 1 倍刷取）；开启后单次进入可能因高倍率超过目标库存上限。
    /// </summary>
    public bool UseAutoSeries { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 启用「使用药剂」勾选框。
    /// 默认开启；关闭后各 Plan 不显示药剂行，序列化时强制传 0。
    /// </summary>
    public bool UseMedicine { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 启用「使用源石」勾选框。
    /// 默认开启；关闭后各 Plan 不显示源石行，序列化时强制传 0。
    /// </summary>
    public bool UseStone { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 使用 48 小时内过期的理智药。
    /// 默认关闭；开启后所有 Plan 均使用临期药（固定 2 天阈值）。
    /// </summary>
    public bool UseExpiringMedicine { get; set; }

    public List<Plan> PlanList { get; set; } = [];

    public record class Plan(string Stage = "", string DropId = "", int DropCount = 0, bool UseMedicine = false, int MedicineCount = 0, bool UseStone = false, int StoneCount = 0, [property: JsonIgnore] int TaskId = 0);
}
