// <copyright file="MallTask.cs" company="MaaAssistantArknights">
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
using System;
using System.Globalization;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 信用购物任务
/// </summary>
public class MallTask : BaseTask
{
    public MallTask() => TaskType = TaskType.Mall;

    /// <summary>
    /// Gets or sets a value indicating whether 是否购物
    /// </summary>
    public bool Shopping { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 信用战斗
    /// </summary>
    public bool CreditFight { get; set; }

    /// <summary>
    /// Gets or sets 信用战斗选择的编队
    /// </summary>
    public int CreditFightFormation { get; set; } = 0;

    /// <summary>
    /// Gets or sets 上次打信用战斗的时间
    /// </summary>
    public string CreditFightLastTime { get; set; } = DateTime.MinValue.ToFormattedString();

    /// <summary>
    /// Gets or sets a value indicating whether 信用战斗一天仅一次
    /// </summary>
    public bool CreditFightOnceADay { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 访问好友
    /// </summary>
    public bool VisitFriends { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 访问好友一天仅一次
    /// </summary>
    public bool VisitFriendsOnceADay { get; set; }

    /// <summary>
    /// Gets or sets 上次访问好友的时间
    /// </summary>
    public string VisitFriendsLastTime { get; set; } = DateTime.MinValue.ToFormattedString();

    /// <summary>
    /// Gets or sets 优先列表
    /// </summary>
    public string FirstList { get; set; } = LocalizationHelper.GetString("HighPriorityDefault");

    /// <summary>
    /// Gets or sets 黑名单
    /// </summary>
    public string BlackList { get; set; } = LocalizationHelper.GetString("BlacklistDefault");

    /// <summary>
    /// Gets or sets a value indicating whether 信用溢出时无视黑名单
    /// </summary>
    public bool ShoppingIgnoreBlackListWhenFull { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 只买打折物品
    /// </summary>
    public bool OnlyBuyDiscount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 低于300信用停止购物
    /// </summary>
    public bool ReserveMaxCredit { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 检查月卡剩余天数并在临期时提醒
    /// </summary>
    public bool MonthlyCard { get; set; }

    /// <summary>
    /// Gets or sets 月卡剩余天数不高于该值时提醒
    /// </summary>
    public int MonthlyCardRemindDays { get; set; } = 7;

    /// <summary>
    /// Gets or sets 上次月卡到期提醒的时间（同一天不重复提醒）
    /// </summary>
    public string MonthlyCardLastRemindTime { get; set; } = DateTime.MinValue.ToFormattedString();

    /// <summary>
    /// Gets or sets 上次月卡检查的日期（鹰角日格式）
    /// </summary>
    public string MonthlyCardLastCheckDate { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 上次月卡检查读到的剩余天数；-1 表示从未读到过
    /// </summary>
    public int MonthlyCardLastDaysLeft { get; set; } = -1;

    /// <summary>
    /// 今天是否需要执行月卡检查：距可能触发提醒尚有富余天数则跳过（月卡 30 天一周期，无需每天检查）
    /// </summary>
    /// <returns>需要检查返回 true；按上次读数判定尚未到重查日返回 false。</returns>
    public bool IsMonthlyCardCheckDue()
    {
        if (MonthlyCardLastDaysLeft < 0)
        {
            return true; // 尚未读到过读数，先查一次建立基线
        }

        try
        {
            var last = DateTime.ParseExact(MonthlyCardLastCheckDate.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
            var today = DateTime.UtcNow.ToYjDate();

            // 读到 23 天、阈值 7 → 歇 15 天后重查（查到时恰剩 7 天，再留 1 天余量）；读数进入提醒窗口后钳为每天查
            var nextCheck = last.AddDays(Math.Max(MonthlyCardLastDaysLeft - MonthlyCardRemindDays - 1, 0));
            return today >= nextCheck;
        }
        catch
        {
            return true; // 调度状态损坏时退化为每天查
        }
    }

    public bool IsMonthlyCardRemindAvailable(int daysLeft)
    {
        if (daysLeft > MonthlyCardRemindDays)
        {
            return false;
        }

        try
        {
            return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(MonthlyCardLastRemindTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
            return true;
        }
    }

    public bool IsCreditFightAvailable
    {
        get {
            if (!CreditFightOnceADay)
            {
                return CreditFight;
            }

            try
            {
                return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(CreditFightLastTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture) && CreditFight;
            }
            catch
            {
                return CreditFight;
            }
        }
    }

    public bool IsVisitFriendsAvailable
    {
        get {
            if (!VisitFriendsOnceADay)
            {
                return VisitFriends;
            }

            try
            {
                return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(VisitFriendsLastTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture) && VisitFriends;
            }
            catch
            {
                return VisitFriends;
            }
        }
    }
}
