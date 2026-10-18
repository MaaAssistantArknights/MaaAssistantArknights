// <copyright file="AsstBattleAssistTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstBattleAssistTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.BattleAssist;

    [JsonProperty("auto_use_skills")]
    public bool AutoUseSkills { get; set; }

    [JsonProperty("auto_speed_up")]
    public bool AutoSpeedUp { get; set; }

    [JsonProperty("skill_scan_interval_ms")]
    public int SkillScanIntervalMs { get; set; } = 1500;

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
