// <copyright file="MiniGameTask.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Utilities;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 牛杂
/// </summary>
public class MiniGameTask : BaseTask
{
    public MiniGameTask() => TaskType = TaskType.MiniGame;

    /// <summary>
    /// Gets or sets 要运行的牛杂名（tasks.json 中的 key，与工具箱页面的条目一致）
    /// </summary>
    public string MiniGameName { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 隐秘战线结局
    /// </summary>
    public string SecretFrontEnding { get; set; } = "A";

    /// <summary>
    /// Gets or sets 隐秘战线优先系列事件，为空表示不指定
    /// </summary>
    public string SecretFrontEvent { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 自动提升潜能：中坚信物不足时是否消耗普通信物
    /// </summary>
    public bool UseNormalToken { get; set; }

    /// <summary>
    /// Gets or sets 执行计划模式
    /// </summary>
    public MiniGameScheduleMode ScheduleMode { get; set; } = MiniGameScheduleMode.None;

    /// <summary>
    /// Gets or sets 周计划。仅在 <see cref="ScheduleMode"/> 为 <see cref="MiniGameScheduleMode.Weekly"/> 时生效，
    /// 未启用时不参与序列化。
    /// </summary>
    [JsonPredict(nameof(ScheduleMode), MiniGameScheduleMode.Weekly)]
    public Dictionary<DayOfWeek, bool> WeeklySchedule { get; set; } = Enum.GetValues<DayOfWeek>().ToDictionary(i => i, _ => true);

    /// <summary>
    /// Gets or sets 月计划。仅在 <see cref="ScheduleMode"/> 为 <see cref="MiniGameScheduleMode.Monthly"/> 时生效，
    /// 未启用时不参与序列化。
    /// </summary>
    [JsonPredict(nameof(ScheduleMode), MiniGameScheduleMode.Monthly)]
    public Dictionary<int, bool> MonthlySchedule { get; set; } = Enumerable.Range(1, 31).ToDictionary(i => i, _ => true);
}
