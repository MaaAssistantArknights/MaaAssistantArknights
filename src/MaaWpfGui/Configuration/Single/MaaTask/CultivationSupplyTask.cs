// <copyright file="CultivationSupplyTask.cs" company="MaaAssistantArknights">
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

public class CultivationSupplyTask : BaseTask
{
    public CultivationSupplyTask() => TaskType = TaskType.CultivationSupply;

    public List<Plan> Plans { get; set; } = [];

    public string SourceTaskId { get; set; } = string.Empty;

    public string SourceFingerprint { get; set; } = string.Empty;

    public bool IsStale { get; set; }

    public bool UseAutoSeries { get; set; }

    public bool UseMedicine { get; set; }

    public int MedicineCount { get; set; }

    public bool UseExpiringMedicine { get; set; }

    // TargetCount is the total remaining cultivation demand in chip-pack equivalents.
    // Neither raw chip packs nor existing dual chips have been subtracted from it.
    public record Plan(string ItemId, int TargetCount, string Stage);
}
