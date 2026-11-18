// <copyright file="MiniGame.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using MaaWpfGui.Models;
using static MaaWpfGui.Configuration.Factory.ConfigFactory;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 工具箱小游戏标签页相关设置
/// </summary>
public class MiniGame : NotifyPropertyChangedWithValue
{
    public void EventBinding(string prefix)
    {
        PropertyChanged += Handler.OnPropertyChangedFactory(prefix);
        Reclamation.EventBinding(prefix + nameof(Reclamation) + ".");
    }

    public string EventShopBlackList { get; set; } = string.Empty;

    public bool EventShopBlackListDataSupplementInstrument { get; set; }

    public bool EventShopBlackListDataSupplementStick { get; set; }

    public bool EventShopBlackListLmd { get; set; }

    public bool EventShopBlackListFurniturePart { get; set; }

    public bool EventShopBlackListOther { get; set; }

    /// <summary>
    /// Gets or sets 卫戍协议运行模式："level" 刷等级（原有实现），"badge" 刷角标。
    /// </summary>
    public string SPAMode { get; set; } = "level";

    /// <summary>
    /// Gets or sets a value indicating whether 刷角标使用低配模式（不需要自选编队）。
    /// </summary>
    public bool SPALowProfile { get; set; }

    /// <summary>
    /// Gets or sets 刷角标时单个策略需要通关的次数：3 = 金色角标，6 = 彩色角标。
    /// </summary>
    public int SPATargetClears { get; set; } = 3;

    /// <summary>
    /// Gets or sets 刷角标最多通关局数。
    /// </summary>
    public int SPAMaxRuns { get; set; } = 20;

    /// <summary>
    /// Gets or sets 刷角标的目标："medal" 刷策略徽章，"task-1".."task-7" 对应七条任务线。
    /// 对应插件参数 <c>spa_farm.goal</c>（JS 的 <c>pr[].id</c>）。
    /// </summary>
    public string SPAGoal { get; set; } = "medal";

    /// <summary>
    /// Gets or sets 刷角标的对局模式："quick" 极速模拟，"standard" 标准模拟。
    /// 对应插件参数 <c>spa_farm.mode</c>；刷角标时若为 "standard"，
    /// 插件会在难度页按「规程等级N」拖拽选择。
    /// </summary>
    public string SPASimMode { get; set; } = "quick";

    [JsonInclude]
    public Reclamation Reclamation { get; private set; } = new();
}
