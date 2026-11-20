// <copyright file="SeasonalTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class SeasonalTaskUserControlModel : TaskSettingsViewModel, SeasonalTaskUserControlModel.ISerialize
{
    static SeasonalTaskUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public static SeasonalTaskUserControlModel Instance { get; }

    // 大核心盟约（阵营盟约）选项；盟约名为游戏专有名词不做翻译，
    // 经 LocalizedObservableList 的未收录 key 原文回退直接显示
    public LocalizedObservableList<string> MajorBondList { get; } = new(
        (string.Empty, "Common.NotSelected"),
        ("乌萨斯", "乌萨斯"),
        ("阿戈尔", "阿戈尔"),
        ("谢拉格", "谢拉格"),
        ("萨尔贡", "萨尔贡"),
        ("莱塔尼亚", "莱塔尼亚"),
        ("维多利亚", "维多利亚"),
        ("卡西米尔", "卡西米尔"),
        ("伊比利亚", "伊比利亚"),
        ("叙拉古", "叙拉古"),
        ("炎", "炎"));

    // 小核心盟约（非阵营盟约）选项
    public LocalizedObservableList<string> MinorBondList { get; } = new(
        (string.Empty, "Common.NotSelected"),
        ("不屈", "不屈"),
        ("坚守", "坚守"),
        ("精准", "精准"),
        ("突袭", "突袭"),
        ("灵巧", "灵巧"),
        ("奥术", "奥术"),
        ("迅捷", "迅捷"),
        ("聚合", "聚合"),
        ("远见", "远见"),
        ("助力", "助力"),
        ("奇迹", "奇迹"),
        ("独行", "独行"),
        ("调和", "调和"),
        ("协防干员", "协防干员"),
        ("投资人", "投资人"));

    // 每回合升级目标（回合 1~9），0 = 该回合不升级
    public ObservableCollection<UpgradeRoundItem> UpgradeRounds { get; } = [];

    public SeasonalTaskUserControlModel()
    {
        UpgradeRounds.CollectionChanged += UpgradeRounds_CollectionChanged;
        for (var round = 1; round <= MaxUpgradeRounds; ++round)
        {
            UpgradeRounds.Add(new UpgradeRoundItem(round));
        }
    }

    /// <summary>
    /// Gets 升级计划可配置的最大回合数，与极速模拟的单局回合数上限一致。
    /// </summary>
    public static int MaxUpgradeRounds => 9;

    /// <summary>
    /// Gets or sets 通关循环局数，0 = 跳过任务。
    /// </summary>
    public int Rounds
    {
        get => GetTaskConfig<SeasonalTask>().Rounds;
        set => SetTaskConfig<SeasonalTask>(t => t.Rounds == value, t => t.Rounds = value);
    }

    /// <summary>
    /// Gets or sets 大核心盟约（阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMajorBond
    {
        get => GetTaskConfig<SeasonalTask>().CoreMajorBond;
        set => SetTaskConfig<SeasonalTask>(t => t.CoreMajorBond == value, t => t.CoreMajorBond = value);
    }

    /// <summary>
    /// Gets or sets 小核心盟约（非阵营盟约），空 = 不启用。
    /// </summary>
    public string CoreMinorBond
    {
        get => GetTaskConfig<SeasonalTask>().CoreMinorBond;
        set => SetTaskConfig<SeasonalTask>(t => t.CoreMinorBond == value, t => t.CoreMinorBond = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not SeasonalTask task)
        {
            return;
        }

        IsRefreshingUI = true;
        foreach (var item in UpgradeRounds)
        {
            item.PropertyChanged -= UpgradeRoundItem_PropertyChanged;
        }

        UpgradeRounds.CollectionChanged -= UpgradeRounds_CollectionChanged;
        for (var i = 0; i < UpgradeRounds.Count; ++i)
        {
            UpgradeRounds[i].Target = i < task.UpgradePlan.Count ? task.UpgradePlan[i] : 0;
        }

        UpgradeRounds.CollectionChanged += UpgradeRounds_CollectionChanged;
        foreach (var item in UpgradeRounds)
        {
            item.PropertyChanged += UpgradeRoundItem_PropertyChanged;
        }

        IsRefreshingUI = false;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    /// <summary>
    /// 语言热切换时刷新盟约选项与回合行标签文本。
    /// </summary>
    private void RefreshLocalization()
    {
        MajorBondList.RefreshLocalization();
        MinorBondList.RefreshLocalization();
        foreach (var item in UpgradeRounds)
        {
            item.RefreshDisplay();
        }
    }

    /// <summary>
    /// 将每回合升级目标同步回任务配置，末尾连续的 0（该回合不升级）不保存。
    /// </summary>
    private void SyncUpgradePlanToTaskConfig()
    {
        var plan = UpgradeRounds.Select(r => r.Target).ToList();
        while (plan.Count > 0 && plan[^1] == 0)
        {
            plan.RemoveAt(plan.Count - 1);
        }

        SetTaskConfig<SeasonalTask>(t => t.UpgradePlan.SequenceEqual(plan), t => t.UpgradePlan = plan);
    }

    private void UpgradeRounds_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action is NotifyCollectionChangedAction.Remove or NotifyCollectionChangedAction.Replace)
        {
            e.OldItems?.OfType<UpgradeRoundItem>().ToList().ForEach(item => item.PropertyChanged -= UpgradeRoundItem_PropertyChanged);
        }

        SyncUpgradePlanToTaskConfig();
    }

    private void UpgradeRoundItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!IsRefreshingUI && e.PropertyName == nameof(UpgradeRoundItem.Target))
        {
            SyncUpgradePlanToTaskConfig();
        }
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not SeasonalTask task || taskId > 0)
            {
                return (null, []);
            }

            // 轮数为 0 时跳过任务
            if (task.Rounds <= 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskQueue.Task.SeasonalTask.Skipped"), UiLogColor.Info);
                return (null, []);
            }

            Instances.TaskQueueViewModel.AddLogSection(task.NameOrTaskType);

            var (isSuccess, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                TaskType.SeasonalTask,
                new AsstSeasonalTask
                {
                    Rounds = task.Rounds,
                    CoreMajorBond = task.CoreMajorBond,
                    CoreMinorBond = task.CoreMinorBond,
                    UpgradePlan = task.UpgradePlan,
                });

            return FromSingle((isSuccess, id));
        }
    }

    /// <summary>
    /// 每回合升级目标行。
    /// </summary>
    public class UpgradeRoundItem(int round) : PropertyChangedBase
    {
        /// <summary>
        /// Gets 回合号（1 起）。
        /// </summary>
        public int Round { get; } = round;

        /// <summary>
        /// Gets 行标签文本（第 N 回合）。
        /// </summary>
        public string Display => string.Format(LocalizationHelper.GetString("TaskQueue.Task.SeasonalTask.UpgradePlan.Round"), Round);

        /// <summary>
        /// Gets or sets 该回合目标商店等级，0 = 该回合不升级。
        /// </summary>
        public int Target { get => field; set => SetAndNotify(ref field, value); }

        /// <summary>
        /// 语言切换后刷新行标签。
        /// </summary>
        public void RefreshDisplay() => NotifyOfPropertyChange(nameof(Display));
    }
}
