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
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 信用购物
/// </summary>
public class MallSettingsUserControlModel : PropertyChangedBase
{
    static MallSettingsUserControlModel()
    {
        Instance = new();
    }

    public static MallSettingsUserControlModel Instance { get; }

    private string _lastCreditFightTaskTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

    public string LastCreditFightTaskTime
    {
        get => _lastCreditFightTaskTime;
        set
        {
            SetAndNotify(ref _lastCreditFightTaskTime, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.LastCreditFightTaskTime, value);
        }
    }

    private bool _creditFightTaskEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether credit fight task is enabled.
    /// </summary>
    public bool CreditFightTaskEnabled
    {
        get
        {
            try
            {
                if (DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(_lastCreditFightTaskTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture))
                {
                    return _creditFightTaskEnabled;
                }
            }
            catch
            {
                return _creditFightTaskEnabled;
            }

            return false;
        }

        set
        {
            SetAndNotify(ref _creditFightTaskEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightTaskEnabled, value.ToString());
        }
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
        get => _lastCreditVisitFriendsTime;
        set
        {
            SetAndNotify(ref _lastCreditVisitFriendsTime, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.LastCreditVisitFriendsTime, value);
        }
    }

    private bool _creditVisitOnceADay = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to bypass the daily limit.
    /// </summary>
    public bool CreditVisitOnceADay
    {
        get => _creditVisitOnceADay;
        set
        {
            SetAndNotify(ref _creditVisitOnceADay, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitOnceADay, value.ToString());
        }
    }

    private bool _creditVisitFriendsEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether visiting friends task is enabled.
    /// </summary>
    public bool CreditVisitFriendsEnabled
    {
        get
        {
            if (!_creditVisitOnceADay)
            {
                return _creditVisitFriendsEnabled;
            }

            try
            {
                return DateTime.UtcNow.ToYjDate() > DateTime.ParseExact(_lastCreditVisitFriendsTime.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture)
                       && _creditVisitFriendsEnabled;
            }
            catch
            {
                return _creditVisitFriendsEnabled;
            }
        }

        set
        {
            SetAndNotify(ref _creditVisitFriendsEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditVisitFriendsEnabled, value.ToString());
        }
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
        get => _creditFightSelectFormation;
        set
        {
            SetAndNotify(ref _creditFightSelectFormation, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditFightSelectFormation, value.ToString());
        }
    }

    private bool _creditShopping = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to shop with credit.
    /// </summary>
    public bool CreditShopping
    {
        get => _creditShopping;
        set
        {
            SetAndNotify(ref _creditShopping, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditShopping, value.ToString());
        }
    }

    private string _creditFirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault"));

    /// <summary>
    /// Gets or sets the priority item list of credit shop.
    /// </summary>
    public string CreditFirstList
    {
        get => _creditFirstList;
        set
        {
            SetAndNotify(ref _creditFirstList, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditFirstListNew, value);
        }
    }

    private string _creditBlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault"));

    /// <summary>
    /// Gets or sets the blacklist of credit shop.
    /// </summary>
    public string CreditBlackList
    {
        get => _creditBlackList;
        set
        {
            SetAndNotify(ref _creditBlackList, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditBlackListNew, value);
        }
    }

    private bool _creditForceShoppingIfCreditFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether save credit is enabled.
    /// </summary>
    public bool CreditForceShoppingIfCreditFull
    {
        get => _creditForceShoppingIfCreditFull;
        set
        {
            SetAndNotify(ref _creditForceShoppingIfCreditFull, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, value.ToString());
        }
    }

    private bool _creditOnlyBuyDiscount = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether only buy discount is enabled.
    /// </summary>
    public bool CreditOnlyBuyDiscount
    {
        get => _creditOnlyBuyDiscount;
        set
        {
            SetAndNotify(ref _creditOnlyBuyDiscount, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditOnlyBuyDiscount, value.ToString());
        }
    }

    private bool _creditReserveMaxCredit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether reserve max credit is enabled.
    /// </summary>
    public bool CreditReserveMaxCredit
    {
        get => _creditReserveMaxCredit;
        set
        {
            SetAndNotify(ref _creditReserveMaxCredit, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CreditReserveMaxCredit, value.ToString());
        }
    }
}
