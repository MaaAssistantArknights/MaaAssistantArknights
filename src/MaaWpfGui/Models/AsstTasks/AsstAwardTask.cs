// <copyright file="AsstAwardTask.cs" company="MaaAssistantArknights">
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
/// 领取奖励任务
/// </summary>
public class AsstAwardTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Award;

    /// <summary>
    /// Gets or sets a value indicating whether 是否领取每日/每周任务奖励
    /// </summary>
    [JsonProperty("award")]
    public bool Award { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否领取所有邮件奖励
    /// </summary>
    [JsonProperty("mail")]
    public bool Mail { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否进行每日免费单抽
    /// </summary>
    [JsonProperty("recruit")]
    public bool FreeGacha { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否领取幸运墙合成玉奖励
    /// </summary>
    [JsonProperty("orundum")]
    public bool Orundum { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否领取限时开采许可合成玉奖励
    /// </summary>
    [JsonProperty("mining")]
    public bool Mining { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否领取五周年赠送月卡奖励
    /// </summary>
    [JsonProperty("specialaccess")]
    public bool SpecialAccess { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
