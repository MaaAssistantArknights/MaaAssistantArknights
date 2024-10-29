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
using System.Linq;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Configuration.MaaTask;

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

    /// <summary>
    /// 序列化任务为 JSON 对象
    /// </summary>
    /// <param name = "mode" >
    /// 模式。可用值包括：
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
    /// </param>
    /// <param name="starts">开始探索次数。</param>
    /// <param name="investmentEnabled">是否投资源石锭</param>
    /// <param name="investmentWithMoreScore">投资时候刷更多分</param>
    /// <param name="invests">投资源石锭次数。</param>
    /// <param name="stopWhenFull">投资满了自动停止任务。</param>
    /// <param name="squad">开局分队</param>
    /// <param name="roles">开局职业组</param>
    /// <param name="coreChar">开局干员名</param>
    /// <param name="startWithEliteTwo">是否凹开局直升</param>
    /// <param name="onlyStartWithEliteTwo">是否只凹开局直升，不进行作战</param>
    /// <param name="roguelike3FirstFloorFoldartal">凹第一层远见板子</param>
    /// <param name="roguelike3StartFloorFoldartal">需要凹的板子</param>
    /// <param name="roguelike3NewSquad2StartingFoldartal">是否在萨米肉鸽生活队凹开局板子</param>
    /// <param name="roguelike3NewSquad2StartingFoldartals">需要凹的板子，用;连接的字符串</param>
    /// <param name="useSupport">是否core_char使用好友助战</param>
    /// <param name="enableNonFriendSupport">是否允许使用非好友助战</param>
    /// <param name="theme">肉鸽主题["Phantom", "Mizuki", "Sami", "Sarkaz"]</param>
    /// <param name="refreshTraderWithDice">是否用骰子刷新商店购买特殊商品，目前支持水月肉鸽的指路鳞</param>
    /// <param name="stopAtFinalBoss">是否在五层BOSS前停下来</param>
    /// <returns>Core Task</returns>
    public override JObject SerializeJsonTask()
    {
        var taskParams = new JObject()
        {
            ["theme"] = Theme.ToString(),
            ["mode"] = (int)Mode,
            ["starts_count"] = StartCount,
            ["investment_enabled"] = false,
        };

        if (Theme != RoguelikeTheme.Phantom)
        {
            taskParams["difficulty"] = Difficulty;
        }

        if (Investment)
        {
            taskParams["investment_enabled"] = true;
            taskParams["investment_with_more_score"] = InvestWithMoreScore && Mode == RoguelikeMode.Investment;
            taskParams["investments_count"] = InvestCount;
            taskParams["stop_when_investment_full"] = StopWhenDepositFull;
        }

        if (!string.IsNullOrEmpty(Squad))
        {
            taskParams["squad"] = Squad;
        }

        if (!string.IsNullOrEmpty(Roles))
        {
            taskParams["roles"] = Roles;
        }

        if (!string.IsNullOrEmpty(CoreChar))
        {
            taskParams["core_char"] = CoreChar;
        }

        taskParams["start_with_elite_two"] = StartWithEliteTwo && SquadIsProfessional;
        taskParams["only_start_with_elite_two"] = StartWithEliteTwoOnly && StartWithEliteTwo && SquadIsProfessional;

        if (SamiFirstFloorFoldartal && Mode == RoguelikeMode.Collectible && Theme == RoguelikeTheme.Sami && string.IsNullOrEmpty(SamiFirstFloorFoldartals))
        {
            taskParams["first_floor_foldartal"] = SamiFirstFloorFoldartals;
        }

        if (SamiNewSquad2StartingFoldartal && SquadIsFoldartal && string.IsNullOrEmpty(SamiNewSquad2StartingFoldartals))
        {
            taskParams["start_foldartal_list"] = new JArray(SamiNewSquad2StartingFoldartals?.Trim().Split(';', '；').Where(i => !string.IsNullOrEmpty(i)).Take(3) ?? []);
        }

        taskParams["use_support"] = UseSupport;
        taskParams["use_nonfriend_support"] = UseSupportNonFriend;
        taskParams["refresh_trader_with_dice"] = Theme == RoguelikeTheme.Mizuki && RefreshTraderWithDice;

        taskParams["stop_at_final_boss"] = Mode == RoguelikeMode.Exp && StopAtFinalBoss;

        return taskParams;
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
}
