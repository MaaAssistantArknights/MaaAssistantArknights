// <copyright file="DailyReportAccount.cs" company="MaaAssistantArknights">
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
using System;
using MaaWpfGui.Models;

namespace MaaWpfGui.Configuration.Global;

/// <summary>
/// 用户手动选择的日报账号，与模拟器连接和 MAA 配置独立。
/// </summary>
public class DailyReportAccount : NotifyPropertyChangedWithValue
{
    /// <summary>
    /// Gets or sets 稳定标识，显示名称变化不改变历史记录的归属。
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString("N");

    /// <summary>
    /// Gets or sets 用户自定义的账号名称，不含登录凭据。
    /// </summary>
    public string Name { get; set; } = string.Empty;
}
