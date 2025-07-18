// <copyright file="RoguelikeTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 肉鸽任务
/// </summary>
public class RoguelikeTask : BaseTask
{
    public RoguelikeTask() => TaskType = TaskType.Roguelike;

    /// <summary>
    /// Gets or sets 肉鸽主题
    /// </summary>
    public RoguelikeTheme Theme { get; set; } = RoguelikeTheme.Phantom;

    public int Difficulty { get; set; } = int.MaxValue;

    public RoguelikeMode Mode { get; set; } = RoguelikeMode.Exp;

    /// <summary>
    /// Gets or sets 开局分队
    /// </summary>
    public string Squad { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 烧水时候的分队
    /// </summary>
    public string SquadCollectible { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 开局职业组
    /// </summary>
    public string Roles { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 探索次数
    /// </summary>
    public int StartCount { get; set; } = 999999;

    public string CoreChar { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 启用投资
    /// </summary>
    public bool Investment { get; set; } = true;

    /// <summary>
    /// Gets or sets 投资数量
    /// </summary>
    public int InvestCount { get; set; } = 999;

    /// <summary>
    /// Gets or sets a value indicating whether 投资时启用招募、购物刷分
    /// </summary>
    public bool InvestWithMoreScore { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 烧水开局奖励
    /// </summary>
    public RoguelikeCollectibleAward CollectibleStartAwards { get; set; } = RoguelikeCollectibleAward.HotWater | RoguelikeCollectibleAward.Hope | RoguelikeCollectibleAward.Idea;

    /// <summary>
    /// Gets or sets a value indicating whether 烧水购物
    /// </summary>
    public bool CollectibleShopping { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 凹开局直升
    /// </summary>
    public bool StartWithEliteTwo { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 凹开局直升，不打关
    /// </summary>
    public bool StartWithEliteTwoOnly { get; set; }

    /// <summary>
    /// Gets or sets 期望的坍缩范式
    /// </summary>
    public string ExpectedCollapsalParadigms { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 月度小队自动切换
    /// </summary>
    public bool MonthlySquadAutoIterate { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 月度小队刷通讯
    /// </summary>
    public bool MonthlySquadCheckComms { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 深入调查自动切换
    /// </summary>
    public bool DeepExplorationAutoIterate { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 使用好友助战
    /// </summary>
    public bool UseSupport { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 使用非好友助战
    /// </summary>
    public bool UseSupportNonFriend { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 刷新商店(指路鳞)
    /// </summary>
    public bool RefreshTraderWithDice { get; set; }

    private bool SquadIsProfessional => Mode == RoguelikeMode.Collectible && Theme != RoguelikeTheme.Phantom && Squad is "突击战术分队" or "堡垒战术分队" or "远程战术分队" or "破坏战术分队";

    public bool SquadIsFoldartal => Mode == RoguelikeMode.Collectible && Theme == RoguelikeTheme.Sami && Squad == "生活至上分队";

    /// <summary>
    /// Gets or sets a value indicating whether 启用萨米肉鸽刷一层远见板子
    /// </summary>
    public bool SamiFirstFloorFoldartal { get; set; }

    /// <summary>
    /// Gets or sets 萨米肉鸽刷一层远见板子
    /// </summary>
    public string SamiFirstFloorFoldartals { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 在萨米肉鸽生活队凹开局板子
    /// </summary>
    public bool SamiNewSquad2StartingFoldartal { get; set; }

    /// <summary>
    /// Gets or sets 在萨米肉鸽生活队凹开局板子
    /// </summary>
    public string SamiNewSquad2StartingFoldartals { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 存款满了停
    /// </summary>
    public bool StopWhenDepositFull { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 最终boss停
    /// </summary>
    public bool StopAtFinalBoss { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 等级满了停
    /// </summary>
    public bool StopWhenLevelMax { get; set; }
}

public enum RoguelikeTheme
{
    /// <summary>
    /// 傀影
    /// </summary>
    Phantom,

    /// <summary>
    /// 水月
    /// </summary>
    Mizuki,

    /// <summary>
    /// 萨米
    /// </summary>
    Sami,

    /// <summary>
    /// 萨卡兹
    /// </summary>
    Sarkaz,

    /// <summary>
    /// 界园
    /// </summary>
    JieGarden,
}

public enum RoguelikeMode
{
    /// <summary>
    /// 0 - 刷经验，尽可能稳定地打更多层数，不期而遇采用激进策略
    /// </summary>
    Exp = 0,

    /// <summary>
    /// 1 - 刷源石锭，第一层投资完就退出，不期而遇采用保守策略
    /// </summary>
    Investment = 1,

    /// <summary>
    /// 4 - 刷开局，以获得热水壶或者演讲稿开局或只凹直升，不期而遇采用保守策略
    /// </summary>
    Collectible = 4,

    /// <summary>
    /// 5 - 刷隐藏坍缩范式,以增加坍缩值为最优先目标
    /// </summary>
    CLP_PDS = 5,

    /// <summary>
    /// 月度小队，尽可能稳定地打更多层数，不期而遇采用激进策略
    /// </summary>
    Squad = 6,

    /// <summary>
    /// 深入调查，尽可能稳定地打更多层数，不期而遇采用激进策略
    /// </summary>
    Exploration = 7,
}

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
