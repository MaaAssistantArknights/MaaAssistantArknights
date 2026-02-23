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
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 信用购物
/// </summary>
public class MallSettingsUserControlModel : TaskSettingsViewModel, MallSettingsUserControlModel.ISerialize
{
    static MallSettingsUserControlModel()
    {
        Instance = new();
    }

    public static MallSettingsUserControlModel Instance { get; }

    private readonly Dictionary<string, IEnumerable<string>> _blackCharacterListMapping = new()
        {
            { ClientType.Official, ["讯使", "嘉维尔", "坚雷"] },
            { ClientType.Bilibili, ["讯使", "嘉维尔", "坚雷"] },
            { ClientType.EN, ["Courier", "Gavial", "Dur-nar"] },
            { ClientType.JP, ["クーリエ", "ガヴィル", "ジュナー"] },
            { ClientType.KR, ["쿠리어", "가비알", "듀나"] },
            { ClientType.Txwy, ["訊使", "嘉維爾", "堅雷"] },
        };

    public string LastCreditFightTaskTime
    {
        get => GetTaskConfig<MallTask>().CreditFightLastTime;
        set => SetTaskConfig<MallTask>(t => t.CreditFightLastTime == value, t => t.CreditFightLastTime = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to bypass the daily limit for credit fight.
    /// </summary>
    public bool CreditFightOnceADay
    {
        get => GetTaskConfig<MallTask>().CreditFightOnceADay;
        set => SetTaskConfig<MallTask>(t => t.CreditFightOnceADay == value, t => t.CreditFightOnceADay = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether credit fight task is enabled.
    /// </summary>
    public bool CreditFightTaskEnabled
    {
        get => GetTaskConfig<MallTask>().CreditFight;
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
        get => GetTaskConfig<MallTask>().VisitFriendsLastTime;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsLastTime == value, t => t.VisitFriendsLastTime = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to bypass the daily limit.
    /// </summary>
    public bool CreditVisitOnceADay
    {
        get => GetTaskConfig<MallTask>().VisitFriendsOnceADay;
        set => SetTaskConfig<MallTask>(t => t.VisitFriendsOnceADay == value, t => t.VisitFriendsOnceADay = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether visiting friends task is enabled.
    /// </summary>
    public bool CreditVisitFriendsEnabled
    {
        get => GetTaskConfig<MallTask>().VisitFriends;
        set => SetTaskConfig<MallTask>(t => t.VisitFriends == value, t => t.VisitFriends = value);
    }

    /// <summary>
    /// Gets or sets a value indicating which formation will be select in credit fight.
    /// </summary>
    public int CreditFightSelectFormation
    {
        get => GetTaskConfig<MallTask>().CreditFightFormation;
        set => SetTaskConfig<MallTask>(t => t.CreditFightFormation == value, t => t.CreditFightFormation = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to shop with credit.
    /// </summary>
    public bool CreditShopping
    {
        get => GetTaskConfig<MallTask>().Shopping;
        set => SetTaskConfig<MallTask>(t => t.Shopping == value, t => t.Shopping = value);
    }

    /// <summary>
    /// Gets or sets the priority item list of credit shop.
    /// </summary>
    public string CreditFirstList
    {
        get => GetTaskConfig<MallTask>().FirstList;
        set {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.FirstList == value, t => t.FirstList = value);
        }
    }

    /// <summary>
    /// Gets or sets the blacklist of credit shop.
    /// </summary>
    public string CreditBlackList
    {
        get => GetTaskConfig<MallTask>().BlackList;
        set {
            value = value.Replace("；", ";").Trim();
            SetTaskConfig<MallTask>(t => t.BlackList == value, t => t.BlackList = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether save credit is enabled.
    /// </summary>
    public bool CreditForceShoppingIfCreditFull
    {
        get => GetTaskConfig<MallTask>().ShoppingIgnoreBlackListWhenFull;
        set => SetTaskConfig<MallTask>(t => t.ShoppingIgnoreBlackListWhenFull == value, t => t.ShoppingIgnoreBlackListWhenFull = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether only buy discount is enabled.
    /// </summary>
    public bool CreditOnlyBuyDiscount
    {
        get => GetTaskConfig<MallTask>().OnlyBuyDiscount;
        set => SetTaskConfig<MallTask>(t => t.OnlyBuyDiscount == value, t => t.OnlyBuyDiscount = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether reserve max credit is enabled.
    /// </summary>
    public bool CreditReserveMaxCredit
    {
        get => GetTaskConfig<MallTask>().ReserveMaxCredit;
        set => SetTaskConfig<MallTask>(t => t.ReserveMaxCredit == value, t => t.ReserveMaxCredit = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is MallTask)
        {
            Refresh();
        }
    }

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize)?.Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        bool? ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not MallTask mall)
            {
                return null;
            }

            var fightStageEmpty = false;
            var tasks = ConfigFactory.CurrentConfig.TaskQueue;
            int taskIndex = -1, stageIndex = -1;
            for (int i = 0; i < tasks.Count; i++)
            {
                if (ConfigFactory.CurrentConfig.TaskQueue.ElementAt(i) is not FightTask t)
                {
                    continue;
                }
                else if (!TaskQueueViewModel.IsTaskEnable(t))
                {
                    continue;
                }

                var weekly = Enum.GetValues<DayOfWeek>().Where(d => !t.UseWeeklySchedule || (t.WeeklySchedule.TryGetValue(d, out var isEnabled) && isEnabled));
                if (weekly.Any(day => GetStageForDayOfWeek(t, day) == string.Empty))
                {
                    taskIndex = i;
                    stageIndex = t.StagePlan.IndexOf(string.Empty);
                    fightStageEmpty = true;
                    break;
                }
            }

            if (mall.IsCreditFightAvailable && fightStageEmpty)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("CreditFightWhenOF-1Warning", ConfigFactory.CurrentConfig.TaskQueue[taskIndex].Name, taskIndex + 1, stageIndex + 1), UiLogColor.Warning);
            }

            var task = new AsstMallTask() {
                CreditFight = mall.IsCreditFightAvailable && !fightStageEmpty,
                FormationIndex = mall.CreditFightFormation,
                VisitFriends = mall.IsVisitFriendsAvailable,
                WithShopping = mall.Shopping,
                FirstList = [.. mall.FirstList.Split(';').Select(s => s.Trim())],
                Blacklist = [.. mall.BlackList.Split(';').Select(s => s.Trim()).Union(Instance._blackCharacterListMapping[SettingsViewModel.GameSettings.ClientType])],
                ForceShoppingIfCreditFull = mall.ShoppingIgnoreBlackListWhenFull,
                OnlyBuyDiscount = mall.OnlyBuyDiscount,
                ReserveMaxCredit = mall.ReserveMaxCredit,
            };

            return taskId switch {
                int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
                null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Mall, task),
                _ => null,
            };

            string? GetStageForDayOfWeek(FightTask fightTask, DayOfWeek day)
            {
                var result = fightTask.StagePlan.FirstOrDefault(stage => Instances.StageManager.IsStageOpen(stage, day));
                result ??= fightTask.StagePlan.FirstOrDefault();
                return result;
            }
        }
    }
}
