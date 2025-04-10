// <copyright file="RoguelikeTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
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
    /// Gets or sets 开局职业组
    /// </summary>
    public string Roles { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 探索次数
    /// </summary>
    public int StartCount { get; set; } = 999999;

    public string? CoreChar { get; set; }

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
    /// Gets or sets a value indicating whether 存款满了停
    /// </summary>
    public bool StopWhenDepositFull { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 最终boss停
    /// </summary>
    public bool StopAtFinalBoss { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 凹开局直升
    /// </summary>
    public bool StartWithEliteTwo { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 凹开局直升，不打关
    /// </summary>
    public bool StartWithEliteTwoOnly { get; set; }

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
    public string? SamiFirstFloorFoldartals { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 在萨米肉鸽生活队凹开局板子
    /// </summary>
    public bool SamiNewSquad2StartingFoldartal { get; set; }

    /// <summary>
    /// Gets or sets 在萨米肉鸽生活队凹开局板子
    /// </summary>
    public string? SamiNewSquad2StartingFoldartals { get; set; }
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
}
