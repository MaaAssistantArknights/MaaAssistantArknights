// <copyright file="AsstStartUpTask.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

public class AsstStartUpTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.StartUp;

    /// <summary>
    /// Gets or sets 客户端版本
    /// </summary>
    [JsonProperty("client_type")]
    public string ClientType { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether gets or sets 是否自动启动客户端
    /// </summary>
    [JsonProperty("start_game_enabled")]
    public bool StartGame { get; set; }

    /// <summary>
    /// Gets or sets 需要切换到的登录名，留空以禁用
    /// </summary>
    [JsonProperty("account_name", DefaultValueHandling = DefaultValueHandling.Ignore)]
    [DefaultValue("")]
    public string AccountName { get; set; } = string.Empty;

    /// <summary>
    /// 开始唤醒。
    /// </summary>
    /// <param name="clientType">客户端版本。</param>
    /// <param name="enable">是否自动启动客户端。</param>
    /// <param name="accountName">需要切换到的登录名，留空以禁用</param>
    /// <returns>是否成功。</returns>
    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
