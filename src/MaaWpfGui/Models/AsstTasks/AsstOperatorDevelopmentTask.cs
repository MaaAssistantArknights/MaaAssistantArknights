// <copyright file="AsstOperatorDevelopmentTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstOperatorDevelopmentTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.OperatorDevelopment;

    public JArray Plans { get; set; } = [];

    public override (AsstTaskType TaskType, JObject Params) Serialize() =>
        (TaskType, new JObject { ["plans"] = Plans.DeepClone() });
}
