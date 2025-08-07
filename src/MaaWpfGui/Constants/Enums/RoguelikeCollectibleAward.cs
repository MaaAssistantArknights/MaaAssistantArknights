// <copyright file="RoguelikeCollectibleAward.cs" company="MaaAssistantArknights">
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

using System;

namespace MaaWpfGui.Constants.Enums;

[Flags]
public enum RoguelikeCollectibleAward
{
    /// <summary>
    /// 热水壶, Roguelike@LastReward
    /// </summary>
    HotWater = 1 << 0,

    /// <summary>
    /// 护盾, Roguelike@LastReward2
    /// </summary>
    Shield = 1 << 1,

    /// <summary>
    /// 源石锭, Roguelike@LastReward3
    /// </summary>
    Ingot = 1 << 2,

    /// <summary>
    /// 2希望, Roguelike@LastReward4
    /// </summary>
    Hope = 1 << 3,

    /// <summary>
    /// 随机, Roguelike@LastRewardRand
    /// </summary>
    Random = 1 << 4,

    /// <summary>
    /// 钥匙, Mizuki@Roguelike@LastReward5
    /// </summary>
    Key = 1 << 5,

    /// <summary>
    /// 骰子, Mizuki@Roguelike@LastReward6
    /// </summary>
    Dice = 1 << 6,

    /// <summary>
    /// 构想, Sarkaz@Roguelike@LastReward5
    /// </summary>
    Idea = 1 << 7,

    /// <summary>
    /// 票券, JieGarden@Roguelike@LastReward5
    /// </summary>
    Ticket = 1 << 8,
}
