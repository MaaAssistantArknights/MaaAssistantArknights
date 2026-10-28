// <copyright file="DailyReportDayData.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using Newtonsoft.Json;

namespace MaaWpfGui.Models.DailyReport;

/// <summary>
/// 单日作战日报数据，对应 data/DailyReport/&lt;yyyy-MM-dd&gt;.json。
/// 文件名即游戏日日期 key，日文件是唯一事实源，周/月/年视图由展示层现场聚合。
/// </summary>
public class DailyReportDayData
{
    /// <summary>
    /// Gets or sets 数据结构版本。
    /// </summary>
    [JsonProperty("version")]
    public int Version { get; set; } = 1;

    /// <summary>
    /// Gets or sets 当日全部作战记录。
    /// </summary>
    [JsonProperty("sessions")]
    public List<CombatSession> Sessions { get; set; } = [];
}
