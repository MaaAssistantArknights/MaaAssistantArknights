// <copyright file="WeComBotSchedule.cs" company="MaaAssistantArknights">
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
/// 企业微信机器人的一条定时清日常时间。
/// </summary>
public class WeComBotSchedule : NotifyPropertyChangedWithValue
{
    /// <summary>
    /// Gets or sets a value indicating whether 本条定时生效。
    /// </summary>
    public bool Enable { get; set; } = true;

    /// <summary>
    /// Gets or sets 小时。
    /// </summary>
    public int Hour { get; set; } = 8;

    /// <summary>
    /// Gets or sets 分钟。
    /// </summary>
    public int Minute { get; set; }
}
