// <copyright file="StartUpTask.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public class StartUpTask : BaseTask, IJsonOnDeserialized
{
    public StartUpTask() => TaskType = TaskType.StartUp;

    public string AccountName { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 启用账号切换。null 表示尚未迁移的旧配置，迁移后不再为 null。
    /// </summary>
    public bool? AccountSwitchEnabled { get; set; }

    public void OnDeserialized()
    {
        // v6.14.0-b2 新增一次性配置迁移：旧配置没有这个字段（json 中不存在，反序列化后为 null），此时按是否已有账号名决定初始值
        AccountSwitchEnabled ??= !string.IsNullOrEmpty(AccountName);
    }
}
