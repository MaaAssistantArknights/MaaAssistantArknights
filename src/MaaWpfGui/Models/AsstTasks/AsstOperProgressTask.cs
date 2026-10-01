// <copyright file="AsstOperProgressTask.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

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
                ["role"] = p.Role.ToString(),
                ["name"] = p.Name,
            };
            if (p.Elite > 0)
            {
                planObj["elite"] = p.Elite;
            }
            if (p.SkillLevel > 0)
            {
                planObj["skill_level"] = p.SkillLevel;
            }
            if (p.SkillMastery.ToArray().Any(x => x > 0))
            {
                planObj["skill_mastery"] = JArray.FromObject(p.SkillMastery.ToArray());
            }
            list.Add(planObj);
        }
        var jObject = new JObject {
            ["plans"] = list,
        };
        return (TaskType, jObject);
    }
}
