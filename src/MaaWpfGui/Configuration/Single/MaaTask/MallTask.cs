// <copyright file="MallTask.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;
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
    public string CreditFightLastTime { get; set; } = DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString();

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
    public string VisitFriendsLastTime { get; set; } = DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString();

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

    public bool IsCreditFightAvailable
    {
        get
        {
            try
            {
                if (DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(CreditFightLastTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
                {
                    return CreditFight;
                }
            }
            catch
            {
                return CreditFight;
            }

            return false;
        }
    }

    public bool IsVisitFriendsAvailable
    {
        get
        {
            if (!VisitFriendsOnceADay)
            {
                return true;
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
