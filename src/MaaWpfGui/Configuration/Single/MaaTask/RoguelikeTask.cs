// <copyright file="RoguelikeTask.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 肉鸽任务
/// </summary>
public class RoguelikeTask : BaseTask, IJsonOnDeserialized
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

    /// <summary>
    /// Gets or sets 开局干员顺位列表，按顺序招募，最多 3 位
    /// </summary>
    public List<RoguelikeStartingOper> StartingOpers { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 启用第 2、3 位开局干员
    /// </summary>
    public bool UseAdditionalStartingOpers { get; set; }

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
    /// Gets or sets 刷开局要求的第一只襁褓动物
    /// </summary>
    public RoguelikeBlackFlowStartAnimal BlackFlowStartAnimal1 { get; set; } = RoguelikeBlackFlowStartAnimal.None;

    /// <summary>
    /// Gets or sets 刷开局要求的第二只襁褓动物
    /// </summary>
    public RoguelikeBlackFlowStartAnimal BlackFlowStartAnimal2 { get; set; } = RoguelikeBlackFlowStartAnimal.None;

    /// <summary>
    /// Gets or sets 刷开局的期望奖励，含羽蛇奖励
    /// </summary>
    public RoguelikeBlackFlowStartReward BlackFlowStartRewards { get; set; } = RoguelikeBlackFlowStartReward.None;

    /// <summary>
    /// Gets or sets a value indicating whether 刷开局的准备局固定用满天赋机械师开局快速飞三层
    /// </summary>
    public bool BlackFlowStartRunFastFlight { get; set; }

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

    public bool StartWithSeed { get; set; }

    public string Seed { get; set; } = string.Empty;

    public void OnDeserialized()
    {
        // 刷襁褓动物已并入刷开局：原来的培育目标改为刷开局的第一只动物。
        // 放在读配置时做，没打开过设置页就直接启动的旧任务也能迁移到。
        // 迁移只用到本类现有的字段，所以照 StartUpTask 写在 OnDeserialized 里；
        // RoguelikeStartingOpersConverter 那种 Root 转换器用于旧字段已删除、只能从原始 JSON 读取的情况。
        if (Theme == RoguelikeTheme.BlackFlow && Mode == RoguelikeMode.BlackFlowBabyAnimal)
        {
            BlackFlowStartAnimal1 = BlackFlowCultivationTarget switch
            {
                RoguelikeBlackFlowCultivationTarget.Dog => RoguelikeBlackFlowStartAnimal.Dog,
                RoguelikeBlackFlowCultivationTarget.Cerberus => RoguelikeBlackFlowStartAnimal.Cerberus,
                RoguelikeBlackFlowCultivationTarget.FeatheredSerpent => RoguelikeBlackFlowStartAnimal.FeatheredSerpent,
                _ => RoguelikeBlackFlowStartAnimal.Cat,
            };
            BlackFlowStartAnimal2 = RoguelikeBlackFlowStartAnimal.None;
            Mode = RoguelikeMode.BlackFlowStartRun;
        }
    }

    /// <summary>
    /// 开局干员顺位
    /// </summary>
    public class RoguelikeStartingOper
    {
        public string Name { get; set; } = string.Empty;

        public bool UseSupport { get; set; }
    }
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

    /// <summary>
    /// 黑流树海
    /// </summary>
    BlackFlow,
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

    /// <summary>
    /// 20001 - 刷常乐节点，第一层进洞，找不到需要的节点就重开
    /// </summary>
    FindPlaytime = 20001,

    /// <summary>
    /// 30001 - 刷襁褓动物（主题专用 GUI 模式）
    /// </summary>
    BlackFlowBabyAnimal = 30001,

    /// <summary>
    /// 30003 - 刷开局（主题专用 GUI 模式）
    /// </summary>
    BlackFlowStartRun = 30003,
}

/// <summary>
/// 黑流树海刷等级的目标结局
/// </summary>
public enum RoguelikeBlackFlowEnding
{
    Ending1 = 1,
    Ending2 = 2,
    Ending3 = 3,
}

/// <summary>
/// 黑流树海刷开局要求的襁褓动物
/// </summary>
public enum RoguelikeBlackFlowStartAnimal
{
    None = 0,
    Cat = 1,
    Dog = 2,
    Cerberus = 3,
    FeatheredSerpent = 4,
}

/// <summary>
/// 黑流树海开局行动奖励，后六项由襁褓羽蛇提供
/// </summary>
[System.Flags]
public enum RoguelikeBlackFlowStartReward
{
    None = 0,
    UnnumberedItem = 1 << 0,
    SurveyAdvance = 1 << 1,
    SpaceLease = 1 << 2,
    RegressionCompensation = 1 << 3,
    ForestRide = 1 << 4,
    NestParasitism = 1 << 5,
    Pegasus = 1 << 6,
    Hippogriff = 1 << 7,
    Dragon = 1 << 8,
    Hydra = 1 << 9,
    BaiZe = 1 << 10,
    GoldenCrow = 1 << 11,
    SerpentRewards = Pegasus | Hippogriff | Dragon | Hydra | BaiZe | GoldenCrow,
}

/// <summary>
/// 黑流树海襁褓动物目标品种
/// </summary>
public enum RoguelikeBlackFlowCultivationTarget
{
    Cat = 0,
    FeatheredSerpent = 1,
    Dog = 2,
    Cerberus = 3,
}

/// <summary>
/// 界园常乐节点子类型
/// </summary>
public enum RoguelikeBoskySubNodeType
{
    /// <summary>
    /// 未知
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// 令 - 常乐 掷地有声
    /// </summary>
    Ling = 1,

    /// <summary>
    /// 黍 - 常乐 种因得果
    /// </summary>
    Shu = 2,

    /// <summary>
    /// 年 - 常乐 三缺一
    /// </summary>
    Nian = 3,
}
