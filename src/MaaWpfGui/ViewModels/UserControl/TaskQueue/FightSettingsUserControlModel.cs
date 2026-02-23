// <copyright file="FightSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using System.Windows;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 理智作战
/// </summary>
public class FightSettingsUserControlModel : TaskSettingsViewModel, FightSettingsUserControlModel.ISerialize
{
    private static readonly ILogger _logger = Log.ForContext<FightSettingsUserControlModel>();
    private static readonly Lock _lock = new();

    public static FightTimes? FightReport { get; set; }

    public static SanityInfo? SanityReport { get; set; }

    static FightSettingsUserControlModel()
    {
        Instance = new();
    }

    public FightSettingsUserControlModel()
    {
        foreach (var i in WeeklyScheduleSource)
        {
            i.PropertyChanged += (_, __) => SaveWeeklySchedule();
        }
        var item = new StagePlanItem();
        item.PropertyChanged += (_, __) => SaveStagePlan();
        StagePlan.Add(item);
        InitDrops();
    }

    public static FightSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or private sets the list of stages.
    /// </summary>
    public ObservableCollection<StageSourceItem> StageListSource { get => field; private set => SetAndNotify(ref field, value); } = [];

    private static readonly Dictionary<string, string> _stageDictionary = new()
        {
            { "AN", "Annihilation" },
            { "剿灭", "Annihilation" },
            { "CE", "CE-6" },
            { "龙门币", "CE-6" },
            { "LS", "LS-6" },
            { "经验", "LS-6" },
            { "狗粮", "LS-6" },
            { "CA", "CA-5" },
            { "技能", "CA-5" },
            { "AP", "AP-5" },
            { "红票", "AP-5" },
            { "SK", "SK-5" },
            { "碳", "SK-5" },
            { "炭", "SK-5" },
        };

    /* private readonly StageSourceItem InvalidStage = new() { Display = LocalizationHelper.GetString("InvalidStage"), Value = "__INVALID__", IsOpen = false, IsVisible = false };*/

    public ObservableCollection<StagePlanItem> StagePlan { get => field; set => SetAndNotify(ref field, value); } = [];

    // UI 绑定的方法
    [UsedImplicitly]
    public void AddStageToPlan()
    {
        var item = new StagePlanItem();
        item.PropertyChanged += (_, __) => SaveStagePlan();
        StagePlan.Add(item);
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void RemoveStageFromPlan(StagePlanItem plan)
    {
        if (StagePlan.Count == 1)
        {
            _logger.Warning("Attempted to remove the last stage from the plan. Operation aborted.");
            return;
        }

        StagePlan.Remove(plan);
    }

    /// <summary>
    /// Gets or sets the stage1.
    /// </summary>
    public string? Stage
    {
        get => field;
        set {
            if (field == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                // 从后往前删
                if (field?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref field, value);
            SetFightParams();
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use custom stage code.
    /// </summary>
    public bool CustomStageCode
    {
        get => GetTaskConfig<FightTask>().IsStageManually;
        set {
            bool ret = SetTaskConfig<FightTask>(t => t.IsStageManually == value, t => t.IsStageManually = value);
            if (ret && !value)
            {
                var stagePlan = GetTaskConfig<FightTask>().StagePlan;
                for (int i = 0; i < stagePlan.Count; i++)
                {
                    var stage = stagePlan[i];
                    if (!Instances.StageManager.GetStageList().Any(p => p.Value == stage))
                    {
                        stagePlan[i] = string.Empty;
                    }
                }
                SetTaskConfig<FightTask>(t => t.StagePlan.SequenceEqual(stagePlan), t => t.StagePlan = stagePlan);
                RefreshCurrentStagePlan();
            }
        }
    }

    /// <summary>
    /// Reset unsaved battle parameters.
    /// </summary>
    /// <param name="fight">The fight task.</param>
    public static void ResetFightVariables(FightTask? fight)
    {
        fight?.UseStone ??= false;
        fight?.UseMedicine ??= false;
        fight?.EnableTimesLimit ??= false;
        fight?.EnableTargetDrop ??= false;
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use medicine with null.
    /// </summary>
    public bool? UseMedicine
    {
        get => GetTaskConfig<FightTask>().UseMedicine;
        set {
            if (!SetTaskConfig<FightTask>(t => t.UseMedicine == value, t => t.UseMedicine = value))
            {
                return;
            }

            if (value == false)
            {
                UseStoneDisplay = false;
            }

            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets the amount of medicine used.
    /// </summary>
    public int MedicineNumber
    {
        get => GetTaskConfig<FightTask>().MedicineCount;
        set {
            if (!SetTaskConfig<FightTask>(t => t.MedicineCount == value, t => t.MedicineCount = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    public static string UseStoneString => LocalizationHelper.GetString("UseOriginitePrime");

    /// <summary>
    /// Gets or sets a value indicating whether to use originiums with null.
    /// </summary>
    public bool? UseStone
    {
        get => GetTaskConfig<FightTask>().UseStone;
        set {
            if (!AllowUseStoneSave && value == true)
            {
                value = null;
            }

            if (value != false)
            {
                MedicineNumber = 999;
                if (UseMedicine == false)
                {
                    UseMedicine = value;
                }
            }

            SetFightParams();
            SetTaskConfig<FightTask>(t => t.UseStone == value, t => t.UseStone = value);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use originiums.
    /// </summary>
    // ReSharper disable once MemberCanBePrivate.Global
    [PropertyDependsOn(nameof(UseStone))]
    public bool UseStoneDisplay
    {
        get => UseStone != false;
        set => UseStone = value;
    }

    /// <summary>
    /// Gets or sets the amount of originiums used.
    /// </summary>
    public int StoneNumber
    {
        get => GetTaskConfig<FightTask>().StoneCount;
        set {
            if (!SetTaskConfig<FightTask>(t => t.StoneCount == value, t => t.StoneCount = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the number of times is limited with null.
    /// </summary>
    public bool? HasTimesLimited
    {
        get => GetTaskConfig<FightTask>().EnableTimesLimit;
        set {
            if (!SetTaskConfig<FightTask>(t => t.EnableTimesLimit == value, t => t.EnableTimesLimit = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets the max number of times.
    /// </summary>
    public int MaxTimes
    {
        get => GetTaskConfig<FightTask>().TimesLimit;
        set {
            if (!SetTaskConfig<FightTask>(t => t.TimesLimit == value, t => t.TimesLimit = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    public static Dictionary<string, int> SeriesList { get; set; } = new()
    {
        { "AUTO", 0 },
        { "6", 6 },
        { "5", 5 },
        { "4", 4 },
        { "3", 3 },
        { "2", 2 },
        { "1", 1 },
        { LocalizationHelper.GetString("NotSwitch"), -1 },
    };

    /// <summary>
    /// Gets or sets the max number of times.
    /// </summary>
    public int Series
    {
        get => GetTaskConfig<FightTask>().Series;
        set {
            if (!SetTaskConfig<FightTask>(t => t.Series == value, t => t.Series = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    #region Drops

    /// <summary>
    /// Gets or sets a value indicating whether the drops are specified.
    /// </summary>
    public bool? IsSpecifiedDrops
    {
        get => GetTaskConfig<FightTask>().EnableTargetDrop;
        set {
            if (!SetTaskConfig<FightTask>(t => t.EnableTargetDrop == value, t => t.EnableTargetDrop = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    /// <summary>
    /// Gets the list of all drops.
    /// </summary>
    private List<CombinedData> AllDrops { get; } = [];

    /// <summary>
    /// 关卡不可掉落的材料
    /// </summary>
    private static readonly HashSet<string> _excludedValues =
    [
        "3213", "3223", "3233", "3243", // 双芯片
        "3253", "3263", "3273", "3283", // 双芯片
        "7001", "7002", "7003", "7004", // 许可
        "4004", "4005", // 凭证
        "3105", "3131", "3132", "3133", // 龙骨/加固建材
        "6001", // 演习券
        "3141", "4002", // 源石
        "32001", // 芯片助剂
        "30115", // 聚合剂
        "30125", // 双极纳米片
        "30135", // D32钢
        "30145", // 晶体电子单元
        "30155", // 烧结核凝晶
        "30165", // 重相位对映体
    ];

    private void InitDrops()
    {
        AllDrops.Add(new() { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty });
        foreach (var (val, value) in ItemListHelper.ArkItems)
        {
            // 不是数字的东西都是正常关卡不会掉的（大概吧）
            if (!int.TryParse(val, out _))
            {
                continue;
            }

            var dis = value.Name;

            if (_excludedValues.Contains(val))
            {
                continue;
            }

            AllDrops.Add(new() { Display = dis, Value = val });
        }

        AllDrops.Sort((a, b) => string.Compare(a.Value, b.Value, StringComparison.Ordinal));
        DropsList = [.. AllDrops];

        foreach (var task in ConfigFactory.CurrentConfig.TaskQueue.OfType<FightTask>())
        {
            if (AllDrops.FirstOrDefault(i => i.Value == task.DropId) is not { } item)
            {
                task.DropId = string.Empty;
            }
        }
    }

    /// <summary>
    /// Gets or private sets the list of drops.
    /// </summary>
    public ObservableCollection<CombinedData> DropsList { get; private set; } = [];

    /// <summary>
    /// Gets or sets the item ID of drops.
    /// </summary>
    public string DropsItemId
    {
        get => GetTaskConfig<FightTask>().DropId;
        set {
            SetTaskConfig<FightTask>(t => t.DropId == value, t => t.DropId = value);
            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets the item Name of drops.
    /// </summary>
    public string DropsItemName { get => field; set => SetAndNotify(ref field, value); } = string.Empty;

    // UI 绑定的方法
    [UsedImplicitly]
    public void DropsListDropDownClosed()
    {
        if (DropsList.FirstOrDefault(i => i.Display == DropsItemName) is { } item)
        {
            DropsItemId = item.Value;
        }
        else
        {
            DropsItemId = string.Empty;
            DropsItemName = LocalizationHelper.GetString("NotSelected");
            NotifyOfPropertyChange(nameof(DropsItemName));
        }
    }

    /// <summary>
    /// Gets or sets the quantity of drops.
    /// </summary>
    public int DropsQuantity
    {
        get => GetTaskConfig<FightTask>().DropCount;
        set {
            SetTaskConfig<FightTask>(t => t.DropCount == value, t => t.DropCount = value);
            SetFightParams();
        }
    }

    #endregion Drops

    public static Dictionary<string, string> AnnihilationModeList { get; } = new()
    {
        { LocalizationHelper.GetString("Annihilation.Current"), "Annihilation" },
        { LocalizationHelper.GetString("Chernobog"), "Chernobog@Annihilation" },
        { LocalizationHelper.GetString("LungmenOutskirts"), "LungmenOutskirts@Annihilation" },
        { LocalizationHelper.GetString("LungmenDowntown"), "LungmenDowntown@Annihilation" },
    };

    public bool UseCustomAnnihilation
    {
        get => GetTaskConfig<FightTask>().UseCustomAnnihilation;
        set {
            bool ret = SetTaskConfig<FightTask>(t => t.UseCustomAnnihilation == value, t => t.UseCustomAnnihilation = value);
            if (ret)
            {
                StageListSource.FirstOrDefault(i => i.Value == "Annihilation")?.Display = UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == AnnihilationStage).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
            }
        }
    }

    public string AnnihilationStage
    {
        get => GetTaskConfig<FightTask>().AnnihilationStage;
        set {
            SetTaskConfig<FightTask>(t => t.AnnihilationStage == value, t => t.AnnihilationStage = value);
            StageListSource.FirstOrDefault(i => i.Value == "Annihilation")?.Display = UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == value).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use DrGrandet mode.
    /// </summary>
    public bool IsDrGrandet
    {
        get => GetTaskConfig<FightTask>().IsDrGrandet;
        set => SetTaskConfig<FightTask>(t => t.IsDrGrandet == value, t => t.IsDrGrandet = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use alternate stage.
    /// </summary>
    public bool UseAlternateStage
    {
        get => GetTaskConfig<FightTask>().UseOptionalStage;
        set {
            SetTaskConfig<FightTask>(t => t.UseOptionalStage == value, t => t.UseOptionalStage = value);
            if (value)
            {
                HideUnavailableStage = false;
                StageResetMode = FightStageResetMode.Ignore;
            }
            else
            {
                var list = StagePlan;
                if (list.Count == 0)
                {
                    var item = new StagePlanItem();
                    item.PropertyChanged += (_, __) => SaveStagePlan();
                    StagePlan.Add(item);
                }
                else
                {
                    var stage = list[0];
                    StagePlan.Clear();
                    StagePlan.Add(stage);
                }
            }
        }
    }

    public bool AllowUseStoneSave
    {
        get => GetTaskConfig<FightTask>().UseStoneAllowSave;
        set {
            if (value)
            {
                var result = MessageBoxHelper.Show(
                    LocalizationHelper.GetString("AllowUseStoneSaveWarning"),
                    LocalizationHelper.GetString("Warning"),
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning,
                    no: LocalizationHelper.GetString("Confirm"),
                    yes: LocalizationHelper.GetString("Cancel"),
                    iconBrushKey: "DangerBrush");
                if (result != MessageBoxResult.No)
                {
                    return;
                }
            }

            SetTaskConfig<FightTask>(t => t.UseStoneAllowSave == value, t => t.UseStoneAllowSave = value);
        }
    }

    public bool UseExpiringMedicine
    {
        get => GetTaskConfig<FightTask>().UseExpiringMedicine;
        set {
            SetTaskConfig<FightTask>(t => t.UseExpiringMedicine == value, t => t.UseExpiringMedicine = value);
            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to hide unavailable stages.
    /// </summary>
    public bool HideUnavailableStage
    {
        get => GetTaskConfig<FightTask>().HideUnavailableStage;
        set {
            var update = SetTaskConfig<FightTask>(t => t.HideUnavailableStage == value, t => t.HideUnavailableStage = value);

            if (value)
            {
                UseAlternateStage = false;
                StageResetMode = FightStageResetMode.Current;
            }
            if (update)
            {
                RefreshStageList();
                RefreshCurrentStagePlan(); // 这个刷新可以优化
            }
        }
    }

    public List<GenericCombinedData<FightStageResetMode>> StageResetModeList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("DefaultStage"), Value = FightStageResetMode.Current },
        new() { Display = LocalizationHelper.GetString("NotSwitch"), Value = FightStageResetMode.Ignore },
    ];

    public FightStageResetMode StageResetMode
    {
        get => GetTaskConfig<FightTask>().StageResetMode;
        set => SetTaskConfig<FightTask>(t => t.StageResetMode == value, t => t.StageResetMode = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to hide series.
    /// </summary>
    public bool HideSeries
    {
        get => GetTaskConfig<FightTask>().HideSeries;
        set => SetTaskConfig<FightTask>(t => t.HideSeries == value, t => t.HideSeries = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use weekly schedule.
    /// </summary>
    public bool UseWeeklySchedule
    {
        get => GetTaskConfig<FightTask>().UseWeeklySchedule;
        set => SetTaskConfig<FightTask>(t => t.UseWeeklySchedule == value, t => t.UseWeeklySchedule = value);
    }

    public ObservableCollection<WeeklyScheduleItem> WeeklyScheduleSource { get; set; } = new(Enum.GetValues<DayOfWeek>().Select(i => new WeeklyScheduleItem(i)));

    public bool AutoRestartOnDrop
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AutoRestartOnDrop, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, true);

    private static string ToUpperAndCheckStage(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return value;
        }

        string upperValue = value.ToUpper();
        if (_stageDictionary.TryGetValue(upperValue, out var stage))
        {
            return stage;
        }

        if (Instance.StageListSource == null)
        {
            return value;
        }

        foreach (var item in Instance.StageListSource)
        {
            if (upperValue.Equals(item.Value, StringComparison.CurrentCultureIgnoreCase) || upperValue.Equals(item.Display, StringComparison.CurrentCultureIgnoreCase))
            {
                return item.Value;
            }
        }

        return value;
    }

    public static string? GetFightStage(FightTask fightTask)
    {
        if (fightTask == null)
        {
            return null;
        }

        var list = fightTask.StagePlan;
        var stage = list?.FirstOrDefault(Instances.TaskQueueViewModel.IsStageOpen);
        stage ??= list?.FirstOrDefault();
        return stage;
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not FightTask fight)
        {
            return;
        }
        IsRefreshingUI = true;
        if (!UseAlternateStage && fight.StagePlan.Count == 0)
        {
            fight.StagePlan.Add(string.Empty);
        }
        RefreshStageList();
        RefreshCurrentStagePlan();
        RefreshWeeklySchedule();
        RefreshDropName();
        Refresh();
        IsRefreshingUI = false;
    }

    private bool? SetFightParams()
    {
        if (IsRefreshingUI || TaskSettingVisibilityInfo.CurrentTask is not FightTask fight)
        {
            return null;
        }

        if (ConfigFactory.CurrentConfig.TaskQueue.IndexOf(fight) is int index && index > -1 && Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskId > 0)
        {
            return SerializeTask(fight, Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskId);
        }
        return null;
    }

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    #region 关卡列表更新

    /// <summary>
    /// Updates stage list.
    /// 使用手动输入时，只更新关卡列表，不更新关卡选择
    /// 使用隐藏当日不开放时，更新关卡列表，关卡选择为未开放的关卡时清空
    /// 使用备选关卡时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 啥都不选时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 除手动输入外所有情况下，如果剩余理智为未开放的关卡，会被清空
    /// </summary>
    // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
    // 这个函数被列为public可见，意味着他注入对象前被调用
    public void UpdateStageList()
    {
        Execute.PostToUIThreadAsync(() => {
            _logger.Information("Updating stage list...");
            using var scope = _lock.EnterScope();
            var stageList = Instances.StageManager.GetStageList();
            RefreshStageList();
            foreach (var task in ConfigFactory.CurrentConfig.TaskQueue.OfType<FightTask>().Where(i => !i.IsStageManually))
            {
                var originalPlan = task.StagePlan.ToList();
                bool reset = false;
                for (int i = 0; i < task.StagePlan.Count; i++)
                {
                    var stage = task.StagePlan[i];
                    if (!stageList.Any(p => p.Value == stage))
                    {
                        reset = true;
                        if (task.StageResetMode == FightStageResetMode.Current)
                        {
                            task.StagePlan[i] = string.Empty;
                        }
                    }
                }
                if (reset)
                {
                    _logger.Information("Reset non-existing stage: {} to {}", string.Join(", ", originalPlan), string.Join(", ", task.StagePlan));
                }
            }
            RefreshCurrentStagePlan();
        });
    }

    private void RefreshStageList()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is not FightTask current)
        {
            return;
        }
        var stageList = Instances.StageManager.GetStageList().ToList();
        var listCurrent = current.StagePlan.ToList();

        var listSource = stageList.Select(i => new StageSourceItem() { Display = i.Display, Value = i.Value, IsVisible = !HideUnavailableStage || i.IsStageOpen(Instances.TaskQueueViewModel.CurDayOfWeek), IsOpen = Instances.StageManager.GetStageList().FirstOrDefault(p => p.Value == i.Value)?.IsStageOpen(Instances.TaskQueueViewModel.CurDayOfWeek) ?? true }).ToList();

        // 补过期关卡进来
        foreach (var item in listCurrent.Where(i => !listSource.Any(p => p.Value == i)))
        {
            listSource.Add(new StageSourceItem() { Display = item, Value = item, IsOpen = false, IsVisible = false, IsOutdated = true });
        }
        listSource.FirstOrDefault(i => i.Value == "Annihilation")?.Display = current.UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == current.AnnihilationStage).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
        StageListSource = new ObservableCollection<StageSourceItem>(listSource);
        current.StagePlan = listCurrent; // StageListSource更新后, 恢复StagePlan
    }

    private void RefreshCurrentStagePlan()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is not FightTask current)
        {
            return;
        }
        var plan = current.StagePlan.ToList();
        var list = plan.Select((i, index) => new StagePlanItem(i)).ToList();
        foreach (var item in list)
        {
            item.PropertyChanged += (_, __) => SaveStagePlan();
        }
        StagePlan = new ObservableCollection<StagePlanItem>(list);
        StagePlan.CollectionChanged += (_, __) => SaveStagePlan();
    }

    private void SaveStagePlan()
    {
        var list = StagePlan.Select(i => i.Stage).ToList();
        SetTaskConfig<FightTask>(t => t.StagePlan.SequenceEqual(list), t => t.StagePlan = list);
    }

    private void RefreshWeeklySchedule()
    {
        var plan = GetTaskConfig<FightTask>().WeeklySchedule;
        foreach (var item in WeeklyScheduleSource)
        {
            item.Value = !plan.TryGetValue(item.DayOfWeek, out var value) || value;
        }
    }

    private void SaveWeeklySchedule()
    {
        if (IsRefreshingUI)
        {
            return;
        }

        var dict = WeeklyScheduleSource.ToDictionary(i => i.DayOfWeek, i => i.Value);
        SetTaskConfig<FightTask>(t => t.WeeklySchedule.SequenceEqual(dict), t => t.WeeklySchedule = dict);
    }

    private void RefreshDropName()
    {
        var id = GetTaskConfig<FightTask>().DropId;
        if (AllDrops.FirstOrDefault(i => i.Value == id) is { } item)
        {
            DropsItemName = item.Display;
        }
        else
        {
            SetTaskConfig<FightTask>(t => t.DropId == string.Empty, t => t.DropId = string.Empty);
            DropsItemName = AllDrops.FirstOrDefault(i => i.Value == string.Empty)?.Display ?? string.Empty;
        }
    }

    #endregion 关卡列表更新

    public class SanityInfo
    {
        [JsonProperty("current_sanity")]
        public int SanityCurrent { get; set; }

        [JsonProperty("max_sanity")]
        public int SanityMax { get; set; }

        [JsonProperty("report_time")]
        public DateTimeOffset ReportTime { get; set; }
    }

    public class FightTimes
    {
        [JsonProperty("sanity_cost")]
        public int SanityCost { get; set; }

        [JsonProperty("series")]
        public int Series { get; set; }

        [JsonProperty("times_finished")]
        public int TimesFinished { get; set; }

        [JsonProperty("finished")]
        public bool IsFinished { get; set; }
    }

    public class WeeklyScheduleItem(DayOfWeek dayOfWeek) : PropertyChangedBase
    {
        public string Display => LocalizationHelper.CustomCultureInfo.DateTimeFormat.GetDayName(DayOfWeek);

        public DayOfWeek DayOfWeek { get; } = dayOfWeek;

        public bool Value { get => field; set => SetAndNotify(ref field, value); } = true;
    }

    public class StageSourceItem : PropertyChangedBase
    {
        public string Display { get => field; set => SetAndNotify(ref field, value); } = string.Empty;

        public string Value { get; set; } = string.Empty;

        public bool IsOpen { get => field; set => SetAndNotify(ref field, value); } = true;

        public bool IsVisible { get => field; set => SetAndNotify(ref field, value); } = true;

        /// <summary>
        /// Gets or sets a value indicating whether 过期活动关卡, 加删除线
        /// </summary>
        public bool IsOutdated { get; set; } = false;
    }

    public class StagePlanItem(string stage = "") : PropertyChangedBase
    {
        public string Stage
        {
            get => field;
            set {
                value ??= string.Empty;
                if (TaskSettingVisibilityInfo.CurrentTask is FightTask task && task.UseOptionalStage)
                {
                    // 从后往前删
                    if (value.Length != 3)
                    {
                        value = ToUpperAndCheckStage(value);
                    }
                }

                if (!SetAndNotify(ref field, value))
                {
                    return;
                }

                IsOpen = Instances.StageManager.GetStageList().FirstOrDefault(p => p.Value == value)?.IsStageOpen(Instances.TaskQueueViewModel.CurDayOfWeek) ?? true;
                Instance.SetFightParams();
            }
        } = stage;

        // 仅供 ComboBox本身 和 手写Stage的TextBlock 绑定使用
        public bool IsOpen { get => field; set => SetAndNotify(ref field, value); } = Instances.TaskQueueViewModel.IsStageOpen(stage);
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        bool? ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not FightTask fight || taskId is int and <= 0)
            {
                return null;
            }

            if (fight.UseWeeklySchedule && fight.WeeklySchedule.TryGetValue(Instances.TaskQueueViewModel.CurDayOfWeek, out var isEnabled) && !isEnabled)
            {
                return null;
            }

            using var scope = _lock.EnterScope();
            var stage = GetFightStage(fight);
            if (stage is null)
            {
                return null;
            }
            var task = new AsstFightTask() {
                Stage = stage,
                Medicine = fight.UseMedicine != false ? fight.MedicineCount : 0,
                Stone = fight.UseStone != false ? fight.StoneCount : 0,
                Series = fight.Series,
                MaxTimes = fight.EnableTimesLimit != false ? fight.TimesLimit : int.MaxValue,
                ExpiringMedicine = fight.UseExpiringMedicine ? 9999 : 0,
                IsDrGrandet = fight.IsDrGrandet,
                ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
                ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
                PenguinId = SettingsViewModel.GameSettings.PenguinId,
                YituliuId = SettingsViewModel.GameSettings.PenguinId,
                ServerType = Instances.SettingsViewModel.ServerType,
                ClientType = SettingsViewModel.GameSettings.ClientType,
            };

            if (task.Stage == "Annihilation" && fight.UseCustomAnnihilation)
            {
                task.Stage = fight.AnnihilationStage;
            }

            if (fight.EnableTargetDrop != false && !string.IsNullOrEmpty(fight.DropId))
            {
                task.Drops.Add(fight.DropId, fight.DropCount);
            }

            if (fight.EnableTimesLimit is not false && fight.Series > 0 && fight.TimesLimit % fight.Series != 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("FightTimesMayNotExhausted", fight.TimesLimit, fight.Series), UiLogColor.Warning);
            }

            return taskId switch {
                int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
                null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, task),
                _ => null,
            };
        }
    }
}
