// <copyright file="InfrastTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;
public class InfrastTask : BaseTask
{
    public InfrastTask() => TaskType = TaskType.Infrast;

    /// <summary>
    /// Gets or sets 基建切换模式
    /// </summary>
    public InfrastMode Mode { get; set; } = InfrastMode.Normal;

    /// <summary>
    /// Gets or sets 基建无人机用法
    /// </summary>
    public string UsesOfDrones { get; set; } = "Money";

    /// <summary>
    /// Gets or sets 基建心情阈值
    /// </summary>
    public int DormThreshold { get; set; } = 30;

    /// <summary>
    /// Gets or sets a value indicating whether 宿舍空位补信赖
    /// </summary>
    public bool DormTrustEnabled { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 制造站搓玉是否补货
    /// </summary>
    public bool OriginiumShardAutoReplenishment { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 不将已进驻干员放入宿舍
    /// </summary>
    public bool DormFilterNotStationed { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 会客室板子领取信用
    /// </summary>
    public bool ReceptionMessageBoard { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 继续专精
    /// </summary>
    public bool ContinueTraining { get; set; } = false;

    /// <summary>
    /// Gets or sets 自定义配置文件路径
    /// </summary>
    public string Filename { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 自定义配置计划编号
    /// </summary>
    public int PlanIndex { get; set; }
}

public enum InfrastMode
{
    /// <summary>
    /// 普通
    /// </summary>
    Normal,

    /// <summary>
    /// 自定义
    /// </summary>
    Custom = 10000,

    /// <summary>
    /// 轮换
    /// </summary>
    Rotation = 20000,
}
