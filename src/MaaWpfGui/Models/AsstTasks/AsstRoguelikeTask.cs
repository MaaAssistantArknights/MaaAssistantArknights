// <copyright file="AsstRoguelikeTask.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 肉鸽任务
/// </summary>
public class AsstRoguelikeTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Roguelike;

    /// <summary>
    /// Gets or sets 模式。可用值包括：
    /// <list type="bullet">
    ///     <item>
    ///         <term><c>0</c></term>
    ///         <description>刷蜡烛，尽可能稳定地打更多层数。</description>
    ///     </item>
    ///     <item>
    ///         <term><c>1</c></term>
    ///         <description>刷源石锭，第一层投资完就退出。</description>
    ///     </item>
    ///     <item>
    ///         <term><c>2</c></term>
    ///         <description><b>【即将弃用】</b>两者兼顾，投资过后再退出，没有投资就继续往后打。</description>
    ///     </item>
    ///     <item>
    ///         <term><c>3</c></term>
    ///         <description><b>【开发中】</b>尝试通关，尽可能打的远。</description>
    ///     </item>
    /// </list>
    /// </summary>
    public RoguelikeMode Mode { get; set; }

    /// <summary>
    /// Gets or sets 刷投资的目标难度/其他模式的选择难度
    /// </summary>
    public int Difficulty { get; set; }

    /// <summary>
    /// Gets or sets 开始探索次数
    /// </summary>
    public int Starts { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否投资源石锭
    /// </summary>
    public bool InvestmentEnabled { get; set; }

    /// <summary>
    /// Gets or sets 投资源石锭次数
    /// </summary>
    public int InvestmentCount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 投资时候刷更多分
    /// </summary>
    public bool InvestmentWithMoreScore { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 投资满了自动停止任务
    /// </summary>
    public bool InvestmentStopWhenFull { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 刷开局模式是否购物
    /// </summary>
    public bool CollectibleModeShopping { get; set; }

    /// <summary>
    /// Gets or sets 烧水时使用的分队
    /// </summary>
    public string CollectibleModeSquad { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 开局分队
    /// </summary>
    public string Squad { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 开局职业组
    /// </summary>
    public string Roles { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 开局干员名
    /// </summary>
    public string CoreChar { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 是否凹开局直升
    /// </summary>
    public bool StartWithEliteTwo { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否只凹开局直升，不进行作战
    /// </summary>
    public bool StartWithEliteTwoNonBattle { get; set; }

    /// <summary>
    /// Gets or sets 需要刷的开局
    /// </summary>
    public Dictionary<string, bool> CollectibleModeStartRewards { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 凹第一层远见板子
    /// </summary>
    public bool SamiFirstFloorFoldartal { get; set; }

    /// <summary>
    /// Gets or sets 需要凹的板子
    /// </summary>
    public string SamiStartFloorFoldartal { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 是否在萨米肉鸽生活队凹开局板子
    /// </summary>
    public bool SamiNewSquad2StartingFoldartal { get; set; }

    /// <summary>
    /// Gets or sets 需要凹的板子
    /// </summary>
    public List<string> SamiNewSquad2StartingFoldartals { get; set; } = [];

    /// <summary>
    /// Gets or sets 萨米刷坍缩专用，需要刷的坍缩列表
    /// </summary>
    public List<string> ExpectedCollapsalParadigms { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 是否core_char使用好友助战
    /// </summary>
    public bool UseSupport { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否允许使用非好友助战
    /// </summary>
    public bool UseSupportNonFriend { get; set; }

    /// <summary>
    /// Gets or sets 肉鸽主题
    /// </summary>
    public RoguelikeTheme Theme { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否用骰子刷新商店购买特殊商品，目前支持水月肉鸽的指路鳞
    /// </summary>
    public bool RefreshTraderWithDice { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否启动月度小队自动切换
    /// </summary>
    public bool MonthlySquadAutoIterate { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否将月度小队通信也作为切换依据
    /// </summary>
    public bool MonthlySquadCheckComms { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否启动深入调查自动切换
    /// </summary>
    public bool DeepExplorationAutoIterate { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否在五层BOSS前停下来
    /// </summary>
    public bool StopAtFinalBoss { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否在满级时停止任务
    /// </summary>
    public bool StopAtMaxLevel { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否使用刷钱种子
    /// </summary>
    public bool StartWithSeed { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var taskParams = new JObject
        {
            ["mode"] = (int)Mode,
            ["theme"] = Theme.ToString(),
            ["difficulty"] = Difficulty,
            ["starts_count"] = Starts,
            ["investment_enabled"] = InvestmentEnabled,
        };

        if (InvestmentEnabled)
        {
            taskParams["investment_with_more_score"] = InvestmentWithMoreScore;
            taskParams["investments_count"] = InvestmentCount;
            taskParams["stop_when_investment_full"] = InvestmentStopWhenFull;
        }

        if (Squad.Length > 0)
        {
            taskParams["squad"] = Squad;
        }

        if (Roles.Length > 0)
        {
            taskParams["roles"] = Roles;
        }

        if (CoreChar.Length > 0)
        {
            taskParams["core_char"] = CoreChar;
        }

        if (Mode == RoguelikeMode.Exp)
        {
            taskParams["stop_at_final_boss"] = StopAtFinalBoss;
            taskParams["stop_at_max_level"] = StopAtMaxLevel;
        }
        else if (Mode == RoguelikeMode.Collectible)
        {
            // 刷开局模式
            taskParams["collectible_mode_shopping"] = CollectibleModeShopping;
            taskParams["collectible_mode_squad"] = CollectibleModeSquad;
            taskParams["start_with_elite_two"] = StartWithEliteTwo;
            taskParams["only_start_with_elite_two"] = StartWithEliteTwoNonBattle;
            taskParams["collectible_mode_start_list"] = JObject.FromObject(CollectibleModeStartRewards);
        }

        if (Mode == RoguelikeMode.Squad)
        {
            taskParams["monthly_squad_auto_iterate"] = MonthlySquadAutoIterate;
            taskParams["monthly_squad_check_comms"] = MonthlySquadCheckComms;
        }

        if (Mode == RoguelikeMode.Exploration)
        {
            taskParams["deep_exploration_auto_iterate"] = DeepExplorationAutoIterate;
        }

        if (SamiFirstFloorFoldartal && SamiStartFloorFoldartal.Length > 0)
        {
            taskParams["first_floor_foldartal"] = SamiStartFloorFoldartal;
        }

        if (SamiNewSquad2StartingFoldartal && SamiNewSquad2StartingFoldartals.Count > 0)
        {
            taskParams["start_foldartal_list"] = JArray.FromObject(SamiNewSquad2StartingFoldartals);
        }

        if (Mode == RoguelikeMode.CLP_PDS && ExpectedCollapsalParadigms.Count > 0)
        {
            taskParams["expected_collapsal_paradigms"] = JArray.FromObject(ExpectedCollapsalParadigms);
        }

        taskParams["use_support"] = UseSupport;
        taskParams["use_nonfriend_support"] = UseSupportNonFriend;
        taskParams["refresh_trader_with_dice"] = Theme == RoguelikeTheme.Mizuki && RefreshTraderWithDice;

        return (TaskType, taskParams);
    }
}
