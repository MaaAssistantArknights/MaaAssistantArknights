// <copyright file="AsstMallTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 领取信用及商店购物。
/// </summary>
public class AsstMallTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Mall;

    /// <summary>
    /// Gets or sets a value indicating whether 是否信用战斗
    /// </summary>
    [JsonProperty("credit_fight")]
    public bool CreditFight { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 信用战斗选择编队
    /// </summary>
    [JsonProperty("select_formation")]
    public int SelectFormation { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否访问好友
    /// </summary>
    [JsonProperty("visit_friends")]
    public bool VisitFriends { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 是否购物
    /// </summary>
    [JsonProperty("shopping")]
    public bool WithShopping { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 优先购买列表
    /// </summary>
    [JsonProperty("buy_first")]
    public List<string> FirstList { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 黑名单列表
    /// </summary>
    [JsonProperty("blacklist")]
    public List<string> Blacklist { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 是否在信用溢出时无视黑名单
    /// </summary>
    [JsonProperty("force_shopping_if_credit_full")]
    public bool ForceShoppingIfCreditFull { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 只购买折扣信用商品(未打折的白名单物品仍会购买)
    /// </summary>
    [JsonProperty("only_buy_discount")]
    public bool OnlyBuyDiscount { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 300以下信用点停止购买商品
    /// </summary>
    [JsonProperty("reserve_max_credit")]
    public bool ReserveMaxCredit { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
