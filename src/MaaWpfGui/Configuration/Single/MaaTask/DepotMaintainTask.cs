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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public class DepotMaintainTask : BaseTask
{
    public DepotMaintainTask() => TaskType = TaskType.DepotMaintain;

    public bool UpdateDepot { get; set; } = true;

    public bool IsStageManually { get; set; }

    public List<Plan> PlanList { get; set; } = [];

    public record Plan(string Stage, string DropId, int DropCount, bool UseMedicine, int MedicineCount, bool UseStone, int StoneCount);
}
