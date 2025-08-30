// <copyright file="OperatorRole.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums;

/// <summary>
/// 干员职业类型枚举。
/// </summary>
public enum OperatorRole
{
    /// <summary>
    /// 未知, 默认值
    /// </summary>
    Unknown,

    /// <summary>
    /// 近卫/Guard
    /// </summary>
    Warrior,

    /// <summary>
    /// 先锋/Vanguard
    /// </summary>
    Pioneer,

    /// <summary>
    /// 医疗/Medic
    /// </summary>
    Medic,

    /// <summary>
    /// 重装/Defender/坦克
    /// </summary>
    Tank,

    /// <summary>
    /// 狙击/Sniper
    /// </summary>
    Sniper,

    /// <summary>
    /// 术师/术士/法师/Caster
    /// </summary>
    Caster,

    /// <summary>
    /// 辅助/支援/Supporter
    /// </summary>
    Support,

    /// <summary>
    /// 特种/Specialist
    /// </summary>
    Special,

    /// <summary>
    /// 无人机/召唤物/Drone/Summon
    /// </summary>
    Drone,

    /// <summary>
    /// 召唤物 (from asst::BattleDataConfig, MAA 内部分类使用 Drone
    /// </summary>
    Token = Drone,

    /// <summary>
    /// 装置
    /// </summary>
    Trap,
}
