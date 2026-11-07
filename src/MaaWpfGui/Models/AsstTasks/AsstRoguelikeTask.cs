// <copyright file="AsstRoguelikeTask.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 肉鸽任务
/// </summary>
public class AsstRoguelikeTask : AsstBaseTask
{
    // 满天赋机械师自带结构性原理的开局组合，与黑流策略 mechanic_flight 模块的条件一致
    public const string BlackFlowFlightSquad = "特勤分队";
    public const string BlackFlowFlightRoles = "稳扎稳打";
    public const string BlackFlowFlightCoreChar = "机械师";

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
    /// Gets or sets 开局干员顺位列表，按顺序招募
    /// </summary>
    public List<AsstRoguelikeCoreChar> CoreCharList { get; set; } = [];

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
    /// Gets or sets FindPlaytime 模式的目标常乐节点子类型
    /// </summary>
    public RoguelikeBoskySubNodeType FindPlaytimeTarget { get; set; } = RoguelikeBoskySubNodeType.Ling;

    /// <summary>
    /// Gets or sets 刷襁褓动物模式的目标品种
    /// </summary>
    public RoguelikeBlackFlowCultivationTarget BlackFlowCultivationTarget { get; set; } = RoguelikeBlackFlowCultivationTarget.Cat;

    /// <summary>
    /// Gets or sets 刷等级的目标结局
    /// </summary>
    public RoguelikeBlackFlowEnding BlackFlowEnding { get; set; } = RoguelikeBlackFlowEnding.Ending1;

    /// <summary>
    /// Gets or sets a value indicating whether 刷等级时用机械师快速飞三层
    /// </summary>
    public bool BlackFlowFastFlight { get; set; }

    /// <summary>
    /// Gets or sets 刷开局要求的两只襁褓动物
    /// </summary>
    public RoguelikeBlackFlowStartAnimal BlackFlowStartAnimal1 { get; set; } = RoguelikeBlackFlowStartAnimal.None;

    public RoguelikeBlackFlowStartAnimal BlackFlowStartAnimal2 { get; set; } = RoguelikeBlackFlowStartAnimal.None;

    /// <summary>
    /// Gets or sets 刷开局的期望奖励
    /// </summary>
    public RoguelikeBlackFlowStartReward BlackFlowStartRewards { get; set; } = RoguelikeBlackFlowStartReward.None;

    /// <summary>
    /// Gets or sets a value indicating whether 刷开局的准备局固定用满天赋机械师开局快速飞三层
    /// </summary>
    public bool BlackFlowStartRunFastFlight { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 刷开局的目标局凹第一层实托邦
    /// </summary>
    public bool BlackFlowStartUtopia { get; set; }

    /// <summary>
    /// Gets or sets 凹第一层实托邦时目标局的支援选取顺序
    /// </summary>
    public List<RoguelikeBlackFlowStartReward> BlackFlowStartRewardPriority { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 是否在五层BOSS前停下来
    /// </summary>
    public bool StopAtFinalBoss { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否在满级时停止任务
    /// </summary>
    public bool StopAtMaxLevel { get; set; }

    /// <summary>
    /// Gets or sets 开局时使用的种子
    /// </summary>
    public string? StartWithSeed { get; set; }

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

        // 快速飞三层：分队、职业组和开局干员换成满天赋机械师的固定开局，沿用 Core 刷等级的默认策略。
        // 不勾选就按目标结局打，开局配置即使是这套组合也不飞。
        var expFlight = Theme == RoguelikeTheme.BlackFlow && Mode == RoguelikeMode.Exp && BlackFlowFastFlight;
        if (Theme == RoguelikeTheme.BlackFlow && Mode == RoguelikeMode.Exp && !BlackFlowFastFlight)
        {
            taskParams["blackflow_strategy"] = BlackFlowEnding switch
            {
                RoguelikeBlackFlowEnding.Ending2 => "ending2",
                RoguelikeBlackFlowEnding.Ending3 => "ending3",
                _ => "ending1",
            };
        }

        // 准备局快速飞三层：准备局的分队、职业组和开局干员换成满天赋机械师的固定开局，只招机械师，其余开局招募由 Core 放弃。
        // 凹第一层实托邦的目标局要开局进第一层，用常规设置的职业组和开局干员，所以准备局的另外传。
        var startRunFlight = Theme == RoguelikeTheme.BlackFlow && Mode == RoguelikeMode.BlackFlowStartRun && BlackFlowStartRunFastFlight;
        if (Theme == RoguelikeTheme.BlackFlow && Mode == RoguelikeMode.BlackFlowStartRun)
        {
            var animals = new[] { BlackFlowStartAnimal1, BlackFlowStartAnimal2 }
                .Where(animal => animal != RoguelikeBlackFlowStartAnimal.None).ToArray();
            var hasSerpent = animals.Contains(RoguelikeBlackFlowStartAnimal.FeatheredSerpent);
            var rewards = hasSerpent ? BlackFlowStartRewards : BlackFlowStartRewards & ~RoguelikeBlackFlowStartReward.SerpentRewards;
            taskParams["blackflow_start_animals"] = new JArray(animals.Select(animal => animal switch
            {
                RoguelikeBlackFlowStartAnimal.Cat => "swaddled_cat",
                RoguelikeBlackFlowStartAnimal.Dog => "swaddled_dog",
                RoguelikeBlackFlowStartAnimal.Cerberus => "swaddled_cerberus",
                RoguelikeBlackFlowStartAnimal.FeatheredSerpent => "swaddled_feathered_serpent",
                _ => throw new System.ArgumentOutOfRangeException(nameof(animal)),
            }));
            taskParams["blackflow_start_rewards"] = (int)rewards;
            taskParams["collectible_mode_squad"] = startRunFlight ? BlackFlowFlightSquad : CollectibleModeSquad;
            if (startRunFlight)
            {
                taskParams["collectible_mode_roles"] = BlackFlowFlightRoles;
                taskParams["collectible_mode_core_char_list"] = new JArray(new JObject
                {
                    ["name"] = BlackFlowFlightCoreChar,
                    ["use_support"] = false,
                });
            }

            if (BlackFlowStartUtopia)
            {
                taskParams["blackflow_start_utopia"] = true;
                taskParams["blackflow_start_reward_priority"] = new JArray(BlackFlowStartRewardPriority
                    .Select(reward => reward switch
                    {
                        RoguelikeBlackFlowStartReward.UnnumberedItem => "unnumbered_item",
                        RoguelikeBlackFlowStartReward.SurveyAdvance => "survey_advance",
                        RoguelikeBlackFlowStartReward.SpaceLease => "space_lease",
                        RoguelikeBlackFlowStartReward.RegressionCompensation => "regression_compensation",
                        RoguelikeBlackFlowStartReward.ForestRide => "forest_ride",
                        RoguelikeBlackFlowStartReward.NestParasitism => "nest_parasitism",
                        RoguelikeBlackFlowStartReward.Pegasus => "pegasus",
                        RoguelikeBlackFlowStartReward.Hippogriff => "hippogriff",
                        RoguelikeBlackFlowStartReward.Dragon => "dragon",
                        RoguelikeBlackFlowStartReward.Hydra => "hydra",
                        RoguelikeBlackFlowStartReward.BaiZe => "bai_ze",
                        RoguelikeBlackFlowStartReward.GoldenCrow => "golden_crow",
                        _ => throw new System.ArgumentOutOfRangeException(nameof(reward)),
                    }));
            }
        }

        if (InvestmentEnabled)
        {
            taskParams["investment_with_more_score"] = InvestmentWithMoreScore;
            taskParams["investments_count"] = InvestmentCount;
            taskParams["stop_when_investment_full"] = InvestmentStopWhenFull;
        }

        if (Mode == RoguelikeMode.Squad)
        {
            taskParams["squad"] = "指挥分队";
        }
        else if (expFlight)
        {
            taskParams["squad"] = BlackFlowFlightSquad;
        }
        else if (Squad.Length > 0)
        {
            taskParams["squad"] = Squad;
        }

        if (expFlight)
        {
            taskParams["roles"] = BlackFlowFlightRoles;
        }
        else if (Roles.Length > 0)
        {
            taskParams["roles"] = Roles;
        }

        List<AsstRoguelikeCoreChar> coreCharList = expFlight
            ? [new() { Name = BlackFlowFlightCoreChar, UseSupport = false }]
            : CoreCharList;
        if (coreCharList.Count > 0)
        {
            taskParams["core_char_list"] = new JArray(coreCharList.Select(i => new JObject
            {
                ["name"] = i.Name,
                ["use_support"] = i.UseSupport,
            }));
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

        if (Mode == RoguelikeMode.FindPlaytime)
        {
            taskParams["find_playTime_target"] = (int)FindPlaytimeTarget;
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

        taskParams["use_nonfriend_support"] = UseSupportNonFriend;
        taskParams["refresh_trader_with_dice"] = Theme == RoguelikeTheme.Mizuki && RefreshTraderWithDice;
        if (!string.IsNullOrEmpty(StartWithSeed))
        {
            taskParams["start_with_seed"] = StartWithSeed;
        }

        return (TaskType, taskParams);
    }

    /// <summary>
    /// 开局干员顺位项
    /// </summary>
    public class AsstRoguelikeCoreChar
    {
        /// <summary>
        /// Gets or sets 干员名
        /// </summary>
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets a value indicating whether 该位干员是否使用助战
        /// </summary>
        public bool UseSupport { get; set; }
    }
}
