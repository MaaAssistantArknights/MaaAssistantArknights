// <copyright file="AsstCloseDownTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 关闭明日方舟客户端任务
/// </summary>
public class AsstCloseDownTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.CloseDown;

    /// <summary>
    /// Gets or sets 关闭的客户端类型
    /// </summary>
    public ClientType ClientType { get; set; } = ClientType.Official;

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var (type, task) = (TaskType, JObject.FromObject(this));
        task["client_type"] = ClientType.ToCustomString();

        return (type, task);
    }
}
