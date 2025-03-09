// <copyright file="AsstInfrastTask.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 基建任务
/// </summary>
public class AsstInfrastTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Infrast;

    /// <summary>
    /// Gets or sets 要换班的设施（有序）
    /// </summary>
    public List<string> Facilitys { get; set; } = [];

    /// <summary>
    /// Gets or sets 无人机用途。可用值包括：
    /// <list type="bullet">
    /// <item><c>_NotUse</c></item>
    /// <item><c>Money</c></item>
    /// <item><c>SyntheticJade</c></item>
    /// <item><c>CombatRecord</c></item>
    /// <item><c>PureGold</c></item>
    /// <item><c>OriginStone</c></item>
    /// <item><c>Chip</c></item>
    /// </list>
    /// </summary>
    public string UsesOfDrones { get; set; } = "_NotUse";

    /// <summary>
    /// Gets or sets a value indicating whether 训练室是否尝试连续专精
    /// </summary>
    public bool ContinueTraining { get; set; }

    /// <summary>
    /// Gets or sets 宿舍进驻心情阈值
    /// </summary>
    public double DormThreshold { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 宿舍是否使用未进驻筛选标签
    /// </summary>
    public bool DormFilterNotStationedEnabled { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 宿舍是否使用蹭信赖功能
    /// </summary>
    public bool DormDormTrustEnabled { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 制造站搓玉是否补货
    /// </summary>
    public bool OriginiumShardAutoReplenishment { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 领取基建会客室留言板奖励
    /// </summary>
    public bool InfrastReceptionMessageBoard { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 是否开启自定义配置
    /// </summary>
    public bool IsCustom { get; set; }

    /// <summary>
    /// Gets or sets 自定义配置文件路径
    /// </summary>
    public string Filename { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 自定义配置计划编号
    /// </summary>
    public int PlanIndex { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var taskParams = new JObject
        {
            ["facility"] = JArray.FromObject(Facilitys),
            ["drones"] = UsesOfDrones,
            ["continue_training"] = ContinueTraining,
            ["threshold"] = DormThreshold,
            ["dorm_notstationed_enabled"] = DormFilterNotStationedEnabled,
            ["dorm_trust_enabled"] = DormDormTrustEnabled,
            ["replenish"] = OriginiumShardAutoReplenishment,
            ["reception_message_board"] = InfrastReceptionMessageBoard,
            ["mode"] = IsCustom ? 10000 : 0,
        };

        if (IsCustom)
        {
            taskParams["filename"] = Filename;
            taskParams["plan_index"] = PlanIndex;
        }

        return (TaskType, taskParams);
    }
}
