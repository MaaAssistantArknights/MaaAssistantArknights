// <copyright file="CombatSession.cs" company="MaaAssistantArknights">
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
/// 一条 Fight 链的作战事实记录，对应一场理智作战或库存保持的单条计划。
/// 只记录发生过的量，不记录结束原因、任务来源等叙事信息；零场次的链不产生记录。
/// </summary>
public class CombatSession
{
    /// <summary>
    /// Gets or sets 关卡编号，如 1-7、PR-A-1。
    /// </summary>
    [JsonProperty("stage")]
    public string Stage { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 链开始时刻（UTC），按游戏日归档。
    /// </summary>
    [JsonProperty("startTime")]
    public DateTimeOffset StartTime { get; set; }

    /// <summary>
    /// Gets or sets 链结束时刻（UTC）。运行中的记录与崩溃遗留的记录为 null，后者在加载时补齐。
    /// </summary>
    [JsonProperty("endTime")]
    public DateTimeOffset? EndTime { get; set; }

    /// <summary>
    /// Gets or sets 本链总场次：每局结算按该局代理倍率累加，即实际作战数。
    /// </summary>
    [JsonProperty("times")]
    public int Times { get; set; }

    /// <summary>
    /// Gets or sets 本链理智总消耗：每局结算累加开始前读到的消耗（已含代理倍率）。
    /// </summary>
    [JsonProperty("sanity")]
    public int Sanity { get; set; }

    /// <summary>
    /// Gets or sets 常规理智药使用数。
    /// </summary>
    [JsonProperty("medicinesUsed")]
    public int MedicinesUsed { get; set; }

    /// <summary>
    /// Gets or sets 临期理智药使用数。
    /// </summary>
    [JsonProperty("expiringMedicinesUsed")]
    public int ExpiringMedicinesUsed { get; set; }

    /// <summary>
    /// Gets or sets 源石使用数。
    /// </summary>
    [JsonProperty("stonesUsed")]
    public int StonesUsed { get; set; }

    /// <summary>
    /// Gets or sets 掉落合计（itemId 到本链累计数量），取最后一次 StageDrops 上报的任务内累计值。
    /// </summary>
    [JsonProperty("drops")]
    public Dictionary<string, int> Drops { get; set; } = [];
}
