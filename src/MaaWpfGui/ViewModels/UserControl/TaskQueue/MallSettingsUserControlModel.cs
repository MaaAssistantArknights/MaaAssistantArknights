// <copyright file="MallSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;

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

    private string _lastCreditFightTaskTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

    public string LastCreditFightTaskTime
    {
        get => GetTaskConfig<MallTask>()?.CreditFightLastTime ?? string.Empty;
        set => SetTaskConfig<MallTask>(t => t.CreditFightLastTime == value, t => t.CreditFightLastTime = value);
    }

    private bool _creditFightTaskEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether credit fight task is enabled.
    /// </summary>
    public bool CreditFightTaskEnabled
    {
        get => GetTaskConfig<MallTask>()?.CreditFight ?? default;
        set => SetTaskConfig<MallTask>(t => t.CreditFight == value, t => t.CreditFight = value);
    }

    public bool CreditFightTaskEnabledDisplay
    {
        get
        {
            return _creditFightTaskEnabled;
        }

        set
        {
            SetAndNotify(ref _creditFightTaskEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightTaskEnabled, value.ToString());
        }
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

    private string _lastCreditVisitFriendsTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditVisitFriendsTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

    public string LastCreditVisitFriendsTime
    {
        get => GetTaskConfig<MallTask>()?.VisitFriendsLastTime ?? string.Empty;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsLastTime == value, t => t.VisitFriendsLastTime = value);
    }

    private bool _creditVisitOnceADay = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to bypass the daily limit.
    /// </summary>
    public bool CreditVisitOnceADay
    {
        get => GetTaskConfig<MallTask>()?.VisitFriendsOnceADay ?? default;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsOnceADay == value, t => t.VisitFriendsOnceADay = value);
    }

    private bool _creditVisitFriendsEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether visiting friends task is enabled.
    /// </summary>
    public bool CreditVisitFriendsEnabled
    {
        get => GetTaskConfig<MallTask>()?.VisitFriends ?? default;
        set => SetTaskConfig<MallTask>(t => t.VisitFriends == value, t => t.VisitFriends = value);
    }

    public bool CreditVisitFriendsEnabledDisplay
    {
        get
        {
            return _creditVisitFriendsEnabled;
        }

        set
        {
            SetAndNotify(ref _creditVisitFriendsEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitFriendsEnabled, value.ToString());
        }
    }

    private int _creditFightSelectFormation = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightSelectFormation, "0"));

    /// <summary>
    /// Gets or sets a value indicating which formation will be select in credit fight.
    /// </summary>
    public int CreditFightSelectFormation
    {
        get => GetTaskConfig<MallTask>()?.CreditFightFormation ?? default;
        set => SetTaskConfig<MallTask>(t => t.CreditFightFormation == value, t => t.CreditFightFormation = value);
    }

    private bool _creditShopping = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to shop with credit.
    /// </summary>
    public bool CreditShopping
    {
        get => GetTaskConfig<MallTask>()?.Shopping ?? default;
        set => SetTaskConfig<MallTask>(t => t.Shopping == value, t => t.Shopping = value);
    }

    private string _creditFirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault")).Replace("；", ";").Trim();

    /// <summary>
    /// Gets or sets the priority item list of credit shop.
    /// </summary>
    public string CreditFirstList
    {
        get => _creditFirstList;
        set
        {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.FirstList == value, t => t.FirstList = value);
        }
    }

    private string _creditBlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault")).Replace("；", ";").Trim();

    /// <summary>
    /// Gets or sets the blacklist of credit shop.
    /// </summary>
    public string CreditBlackList
    {
        get => _creditBlackList;
        set
        {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.BlackList == value, t => t.BlackList = value);
        }
    }

    private bool _creditForceShoppingIfCreditFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether save credit is enabled.
    /// </summary>
    public bool CreditForceShoppingIfCreditFull
    {
        get => GetTaskConfig<MallTask>()?.ShoppingIgnoreBlackListWhenFull ?? default;
        set => SetTaskConfig<MallTask>(t => t.ShoppingIgnoreBlackListWhenFull == value, t => t.ShoppingIgnoreBlackListWhenFull = value);
    }

    private bool _creditOnlyBuyDiscount = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether only buy discount is enabled.
    /// </summary>
    public bool CreditOnlyBuyDiscount
    {
        get => GetTaskConfig<MallTask>()?.OnlyBuyDiscount ?? default;
        set => SetTaskConfig<MallTask>(t => t.OnlyBuyDiscount == value, t => t.OnlyBuyDiscount = value);
    }

    private bool _creditReserveMaxCredit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, bool.FalseString));

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
            SetAndNotify(ref _creditReserveMaxCredit, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditReserveMaxCredit, value.ToString());
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var fightEnable = Instances.TaskQueueViewModel.TaskItemViewModels.Where(x => x.OriginalName == "Combat").FirstOrDefault()?.IsCheckedWithNull is not false;
        var task = new AsstMallTask()
        {
            CreditFight = fightEnable ? (!string.IsNullOrEmpty(FightSettingsUserControlModel.Instance.Stage) && CreditFightTaskEnabled) : CreditFightTaskEnabled,
            SelectFormation = CreditFightSelectFormation,
            VisitFriends = CreditVisitFriendsEnabled,
            WithShopping = CreditShopping,
            FirstList = CreditFirstList.Split(';').Select(s => s.Trim()).ToList(),
            Blacklist = CreditBlackList.Split(';').Select(s => s.Trim()).Union(_blackCharacterListMapping[SettingsViewModel.GameSettings.ClientType]).ToList(),
            ForceShoppingIfCreditFull = CreditForceShoppingIfCreditFull,
            OnlyBuyDiscount = CreditOnlyBuyDiscount,
            ReserveMaxCredit = CreditReserveMaxCredit,
        };
        return task.Serialize();
    }
}
