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
/// 卫戍协议赛季主题。
/// </summary>
public enum StrongholdProtocolSeason
{
    /// <summary>规程（第 3 期）</summary>
    Regulation = 0,

    /// <summary>盟约（第 2 期，当前未开放）</summary>
    Alliance = 1,
}

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

    /// <summary>
    /// 卫戍协议赛季主题，默认规程（第 3 期）；盟约（第 2 期）当前未开放。
    /// </summary>
    public StrongholdProtocolSeason StrongholdProtocolSeason { get; set; } = default;

    /// <summary>
    /// 卫戍协议极速模拟运行模式：badge = 刷角标（完整通关），score = 放生速通（刷分）。
    /// </summary>
    public string StrongholdProtocolMode { get; set; } = "badge";

    /// <summary>
    /// 卫戍协议角标目标通关次数：3 = 金色角标，6 = 彩色角标。
    /// </summary>
    public int StrongholdProtocolTarget { get; set; } = 3;

    /// <summary>
    /// 卫戍协议目标通关局数。
    /// </summary>
    public int StrongholdProtocolRuns { get; set; } = 60;

    public bool EventShopBlackListDataSupplementInstrument { get; set; }

    public bool EventShopBlackListDataSupplementStick { get; set; }

    public bool EventShopBlackListLmd { get; set; }

    public bool EventShopBlackListFurniturePart { get; set; }

    public bool EventShopBlackListOther { get; set; }

    [JsonInclude]
    public Reclamation Reclamation { get; private set; } = new();
}
