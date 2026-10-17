// <copyright file="WeComBotAccount.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Models;

namespace MaaWpfGui.Configuration.Global;

/// <summary>
/// 企业微信机器人可操作的账号，与一个 MAA 配置档案一一对应。
/// </summary>
public class WeComBotAccount : NotifyPropertyChangedWithValue
{
    /// <summary>
    /// Gets or sets 聊天里使用的账号称呼，例如「主号」。
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 对应配置档案名，即「切换配置」里的名字。
    /// </summary>
    public string Configuration { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 理智作战关卡，留空表示沿用档案里的设置。
    /// </summary>
    public string Stage { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 参与轮换。
    /// </summary>
    public bool Enable { get; set; } = true;
}
