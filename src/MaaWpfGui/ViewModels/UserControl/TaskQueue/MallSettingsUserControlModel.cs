// <copyright file="MallSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 信用购物
/// </summary>
public class MallSettingsUserControlModel : TaskViewModel
{
    static MallSettingsUserControlModel()
    {
        Instance = new();
    }

    public static MallSettingsUserControlModel Instance { get; }

    private readonly Dictionary<string, IEnumerable<string>> _blackCharacterListMapping = new()
        {
            { string.Empty, ["讯使", "嘉维尔", "坚雷"] },
            { "Official", ["讯使", "嘉维尔", "坚雷"] },
            { "Bilibili", ["讯使", "嘉维尔", "坚雷"] },
            { "YoStarEN", ["Courier", "Gavial", "Dur-nar"] },
            { "YoStarJP", ["クーリエ", "ガヴィル", "ジュナー"] },
            { "YoStarKR", ["쿠리어", "가비알", "듀나"] },
            { "txwy", ["訊使", "嘉維爾", "堅雷"] },
        };

    public string LastCreditFightTaskTime
    {
        get => GetTaskConfig<MallTask>()?.CreditFightLastTime ?? string.Empty;
        set => SetTaskConfig<MallTask>(t => t.CreditFightLastTime == value, t => t.CreditFightLastTime = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether credit fight task is enabled.
    /// </summary>
    public bool CreditFightTaskEnabled
    {
        get => GetTaskConfig<MallTask>()?.CreditFight ?? default;
        set => SetTaskConfig<MallTask>(t => t.CreditFight == value, t => t.CreditFight = value);
    }

    /// <summary>
    /// Gets 设置选择的编队
    /// </summary>
    // ReSharper disable once MemberCanBePrivate.Global
    public List<GenericCombinedData<int>> FormationSelectList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("Current"), Value = 0 },
            new() { Display = "1", Value = 1 },
            new() { Display = "2", Value = 2 },
            new() { Display = "3", Value = 3 },
            new() { Display = "4", Value = 4 },
        ];

    public string LastCreditVisitFriendsTime
    {
        get => GetTaskConfig<MallTask>()?.VisitFriendsLastTime ?? string.Empty;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsLastTime == value, t => t.VisitFriendsLastTime = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to bypass the daily limit.
    /// </summary>
    public bool CreditVisitOnceADay
    {
        get => GetTaskConfig<MallTask>()?.VisitFriendsOnceADay ?? default;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsOnceADay == value, t => t.VisitFriendsOnceADay = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether visiting friends task is enabled.
    /// </summary>
    public bool CreditVisitFriendsEnabled
    {
        get => GetTaskConfig<MallTask>()?.VisitFriends ?? default;
        set => SetTaskConfig<MallTask>(t => t.VisitFriends == value, t => t.VisitFriends = value);
    }

    /// <summary>
    /// Gets or sets a value indicating which formation will be select in credit fight.
    /// </summary>
    public int CreditFightSelectFormation
    {
        get => GetTaskConfig<MallTask>()?.CreditFightFormation ?? default;
        set => SetTaskConfig<MallTask>(t => t.CreditFightFormation == value, t => t.CreditFightFormation = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to shop with credit.
    /// </summary>
    public bool CreditShopping
    {
        get => GetTaskConfig<MallTask>()?.Shopping ?? default;
        set => SetTaskConfig<MallTask>(t => t.Shopping == value, t => t.Shopping = value);
    }

    /// <summary>
    /// Gets or sets the priority item list of credit shop.
    /// </summary>
    public string CreditFirstList
    {
        get => GetTaskConfig<MallTask>()?.FirstList ?? string.Empty;
        set
        {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.FirstList == value, t => t.FirstList = value);
        }
    }

    /// <summary>
    /// Gets or sets the blacklist of credit shop.
    /// </summary>
    public string CreditBlackList
    {
        get => GetTaskConfig<MallTask>()?.BlackList ?? string.Empty;
        set
        {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.BlackList == value, t => t.BlackList = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether save credit is enabled.
    /// </summary>
    public bool CreditForceShoppingIfCreditFull
    {
        get => GetTaskConfig<MallTask>()?.ShoppingIgnoreBlackListWhenFull ?? default;
        set => SetTaskConfig<MallTask>(t => t.ShoppingIgnoreBlackListWhenFull == value, t => t.ShoppingIgnoreBlackListWhenFull = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether only buy discount is enabled.
    /// </summary>
    public bool CreditOnlyBuyDiscount
    {
        get => GetTaskConfig<MallTask>()?.OnlyBuyDiscount ?? default;
        set => SetTaskConfig<MallTask>(t => t.OnlyBuyDiscount == value, t => t.OnlyBuyDiscount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether reserve max credit is enabled.
    /// </summary>
    public bool CreditReserveMaxCredit
    {
        get => GetTaskConfig<MallTask>()?.ReserveMaxCredit ?? default;
        set => SetTaskConfig<MallTask>(t => t.ReserveMaxCredit == value, t => t.ReserveMaxCredit = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is MallTask)
        {
            Refresh();
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var fightEnable = ConfigFactory.CurrentConfig.TaskQueue.Where(x => x is FightTask).FirstOrDefault()?.IsEnable is not false;

        var creditFight = CreditFightTaskEnabled; // 要换成对应Task
        var visitFriends = CreditVisitFriendsEnabled;
        try
        {
            creditFight &= DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(LastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
        }

        try
        {
            visitFriends &= !CreditVisitOnceADay || DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(LastCreditVisitFriendsTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
        }

        var task = new AsstMallTask()
        {
            CreditFight = creditFight && (!fightEnable || !string.IsNullOrEmpty(FightSettingsUserControlModel.Instance.Stage)),
            SelectFormation = CreditFightSelectFormation,
            VisitFriends = visitFriends,
            WithShopping = CreditShopping,
            FirstList = CreditFirstList.Split(';').Select(s => s.Trim()).ToList(),
            Blacklist = CreditBlackList.Split(';').Select(s => s.Trim()).Union(_blackCharacterListMapping[SettingsViewModel.GameSettings.ClientType]).ToList(),
            ForceShoppingIfCreditFull = CreditForceShoppingIfCreditFull,
            OnlyBuyDiscount = CreditOnlyBuyDiscount,
            ReserveMaxCredit = CreditReserveMaxCredit,
        };
        return task.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not MallTask mall)
        {
            return null;
        }

        var fightStage = ConfigFactory.CurrentConfig.TaskQueue.Where(x => x is FightTask).FirstOrDefault()?.IsEnable is not false && ConfigFactory.CurrentConfig.TaskQueue.Where(x => x is FightTask).Cast<FightTask>().FirstOrDefault()?.Stage1 == string.Empty;
        if (fightStage)
        {
            Log.Warning("刷理智 当前/上次导致无法OF-1");
            return false;
        }

        var creditFight = mall.CreditFight;
        var visitFriends = mall.VisitFriends;

        var lastCreditFightTaskTime = GetTaskConfig<MallTask>()?.CreditFightLastTime ?? string.Empty;
        bool creditVisitOnceADay = GetTaskConfig<MallTask>()?.VisitFriendsOnceADay ?? default;
        var lastCreditVisitFriendsTime = GetTaskConfig<MallTask>()?.VisitFriendsLastTime ?? string.Empty;
        try
        {
            creditFight &= DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(lastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
        }

        try
        {
            visitFriends &= !creditVisitOnceADay || DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(lastCreditVisitFriendsTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
        }

        var task = new AsstMallTask()
        {
            CreditFight = creditFight && !fightStage,
            SelectFormation = mall.CreditFightFormation,
            VisitFriends = visitFriends,
            WithShopping = mall.Shopping,
            FirstList = [.. mall.FirstList.Split(';').Select(s => s.Trim())],
            Blacklist = [.. mall.BlackList.Split(';').Select(s => s.Trim()).Union(_blackCharacterListMapping[SettingsViewModel.GameSettings.ClientType])],
            ForceShoppingIfCreditFull = mall.ShoppingIgnoreBlackListWhenFull,
            OnlyBuyDiscount = mall.OnlyBuyDiscount,
            ReserveMaxCredit = mall.ReserveMaxCredit,
        };

        if (taskId is int id)
        {
            return Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task);
        }
        else
        {
            return Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Mall, task);
        }
    }
}
