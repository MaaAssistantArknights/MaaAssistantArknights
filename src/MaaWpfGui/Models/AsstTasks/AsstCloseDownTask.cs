// <copyright file="AsstCloseDownTask.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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

/// <summary>
/// 关闭明日方舟客户端任务
/// </summary>
public class AsstCloseDownTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.CloseDown;

    /// <summary>
    /// Gets or sets 关闭的客户端类型
    /// </summary>
    [JsonProperty("client_type")]
    public string ClientType { get; set; } = string.Empty;

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (AsstTaskType.CloseDown, JObject.FromObject(this));
}
