// <copyright file="AsstFightTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstFightTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Fight;

    /// <summary>
    /// Gets or sets 关卡id
    /// </summary>
    [JsonProperty("stage")]
    public string Stage { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 最大吃药数量
    /// </summary>
    [JsonProperty("medicine")]
    public int Medicine { get; set; }

    /// <summary>
    /// Gets or sets 临期药品数量
    /// </summary>
    [JsonProperty("expiring_medicine")]
    public int ExpiringMedicine { get; set; }

    /// <summary>
    /// Gets or sets 最大碎石数量
    /// </summary>
    [JsonProperty("stone")]
    public int Stone { get; set; }

    /// <summary>
    /// Gets or sets 最大次数
    /// </summary>
    [JsonProperty("times")]
    public int MaxTimes { get; set; } = 1;

    /// <summary>
    /// Gets or sets 连战次数
    /// </summary>
    [JsonProperty("series")]
    public int Series { get; set; } = 1;

    /// <summary>
    /// Gets or sets a value indicating whether 葛朗台
    /// </summary>
    [JsonProperty("DrGrandet")]
    public bool IsDrGrandet { get; set; }

    /// <summary>
    /// Gets or sets 掉落物品id, 数量
    /// </summary>
    public Dictionary<string, int> Drops { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 是否回报企鹅物流，默认 false
    /// </summary>
    [JsonProperty("report_to_penguin")]
    public bool ReportToPenguin { get; set; }

    /// <summary>
    /// Gets or sets 企鹅物流回报id, 可选，默认为空。仅在 <see cref="ReportToPenguin"/> 为 true 时有效
    /// </summary>
    public string PenguinId { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 是否回报一图流，默认 false
    /// </summary>
    [JsonProperty("report_to_yituliu")]
    public bool ReportToYituliu { get; set; }

    /// <summary>
    ///  Gets or sets 一图流回报id，可选，默认为空。仅在 <see cref="ReportToYituliu"/> 为 true 时有效
    /// </summary>
    public string YituliuId { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 服务器，可选，默认 "CN", 会影响上传。选项："CN" | "US" | "JP" | "KR"
    /// </summary>
    [JsonProperty("server")]
    public string ServerType { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 客户端类型
    /// </summary>
    [JsonProperty("client_type")]
    public string ClientType { get; set; } = string.Empty;

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var param = JObject.FromObject(this);
        if (ReportToPenguin && !string.IsNullOrWhiteSpace(PenguinId))
        {
            param["penguin_id"] = PenguinId;
        }

        if (ReportToYituliu && !string.IsNullOrWhiteSpace(YituliuId))
        {
            param["yituliu_id"] = YituliuId;
        }

        if (Drops.Count > 0)
        {
            param["drops"] = JObject.FromObject(Drops);
        }

        return (TaskType, param);
    }
}
