// <copyright file="AsstOperProgressTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask.SkillLevel;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstOperProgressTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.OperProgress;

    [JsonProperty("plans")]
    public List<OperProgressTask.Plan> Plans { get; set; } = [];

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var list = new JArray();
        foreach (var p in Plans)
        {
            var planObj = new JObject {
                ["role"] = p.role.ToString(),
                ["name"] = p.name,
            };
            if (p.elite > 0)
            {
                planObj["elite"] = p.elite;
            }
            if (p.skillLevel is BaseLevel @base)
            {
                planObj["skill_level"] = @base.Level;
            }
            else if (p.skillLevel is Specialization specialization)
            {
                planObj["skill_level"] = new JArray { specialization.Skill1, specialization.Skill2, specialization.Skill3 };
            }
            list.Add(planObj);
        }
        return (TaskType, JObject.FromObject(this));
    }
}
