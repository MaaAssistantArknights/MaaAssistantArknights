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
using static MaaWpfGui.Models.MaaTask;

namespace MaaWpfGui.Configuration.MaaTask
{
    /// <summary>
    /// 信用购物任务
    /// </summary>
    public class MallTask : BaseTask
    {
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

        public static bool IsCreditFightAvailable(MallTask task)
        {
            try
            {
                if (DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(task.CreditFightLastTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
                {
                    return task.CreditFight;
                }
            }
            catch
            {
                return task.CreditFight;
            }

            return false;
        }

        public static bool IsVisitFriendsAvailable(MallTask task)
        {
            if (!task.VisitFriendsOnceADay)
            {
                return true;
            }

            try
            {
                return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(task.VisitFriendsLastTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture)
                       && task.VisitFriends;
            }
            catch
            {
                return task.VisitFriends;
            }
        }

        private static readonly Dictionary<string, IEnumerable<string>> BlackCharacterListMapping = new()
        {
            { string.Empty, new[] { "讯使", "嘉维尔", "坚雷" } },
            { "Official", new[] { "讯使", "嘉维尔", "坚雷" } },
            { "Bilibili", new[] { "讯使", "嘉维尔", "坚雷" } },
            { "YoStarEN", new[] { "Courier", "Gavial", "Dur-nar" } },
            { "YoStarJP", new[] { "クーリエ", "ガヴィル", "ジュナー" } },
            { "YoStarKR", new[] { "쿠리어", "가비알", "듀나" } },
            { "txwy", new[] { "訊使", "嘉維爾", "堅雷" } },
        };

        public override JObject SerializeJsonTask()
        {
            return new JObject
            {
                // TODO 差一个当前/上次检查
                ["credit_fight"] = IsCreditFightAvailable(this),
                ["select_formation"] = CreditFightFormation,
                ["visit_friends"] = IsVisitFriendsAvailable(this),
                ["shopping"] = Shopping,
                ["buy_first"] = new JArray(FirstList.Split(';', '；').Select(s => s.Trim()).ToArray()),
                ["blacklist"] = new JArray(BlackList.Split(';', '；').Select(s => s.Trim()).Union(BlackCharacterListMapping[Instances.SettingsViewModel.ClientType]).ToArray()),
                ["force_shopping_if_credit_full"] = ShoppingIgnoreBlackListWhenFull,
                ["only_buy_discount"] = OnlyBuyDiscount,
                ["reserve_max_credit"] = ReserveMaxCredit,
            };
        }
    }
}
