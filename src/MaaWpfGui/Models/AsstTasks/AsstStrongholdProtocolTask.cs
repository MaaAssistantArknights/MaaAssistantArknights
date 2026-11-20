// <copyright file="AsstStrongholdProtocolTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 卫戍协议极速模拟。
/// </summary>
public class AsstStrongholdProtocolTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.StrongholdProtocol;

    /// <summary>
    /// Gets or sets 运行模式，badge 刷角标，score 刷分。
    /// </summary>
    public string Mode { get; set; } = "badge";

    /// <summary>
    /// Gets or sets 卫戍协议赛季主题
    /// </summary>
    public StrongholdProtocolSeason Season { get; set; } = StrongholdProtocolSeason.Regulation;

    /// <summary>
    /// Gets or sets 角标目标通关次数（3 金 / 6 彩）
    /// </summary>
    public int Target { get; set; } = 3;

    /// <summary>
    /// Gets or sets 目标通关局数
    /// </summary>
    public int Runs { get; set; } = 60;

    /// <summary>
    /// Gets or sets 总局数上限，0 不限
    /// </summary>
    public int Attempts { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var data = new JObject { ["mode"] = Mode };
        if (Mode == "badge")
        {
            data["theme"] = Season.ToString();
            data["target"] = Target;
            data["runs"] = Runs;
            data["attempts"] = Attempts;
        }

        return (TaskType, data);
    }
}
