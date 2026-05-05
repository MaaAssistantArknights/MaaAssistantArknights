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
using System.Threading.Tasks;
using System.Windows;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.States;
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
    public const string AnnihilationName = "Annihilation";
    private static readonly ILogger _logger = Log.ForContext<FightSettingsUserControlModel>();
    private readonly RunningState _runningState;
    private readonly object _specifiedInventoryStateLock = new();
    private Dictionary<string, int>? _runStartInventorySnapshot;
    private Dictionary<string, int> _runReservedQuantityByDropId = [];
    private Dictionary<int, SpecifiedInventoryTaskState> _specifiedInventoryTaskStates = [];
    private bool _lastIdleState;

    public static FightTimes? FightReport { get; set; }

    public static SanityInfo? SanityReport { get; set; }

    static FightSettingsUserControlModel()
    {
        Instance = new();
    }

    public FightSettingsUserControlModel()
    {
        _runningState = RunningState.Instance;
        _lastIdleState = _runningState.Idle;
        _runningState.StateChanged += OnRunningStateChanged;

        foreach (var i in WeeklyScheduleSource)
        {
            i.PropertyChanged += (_, __) => SaveWeeklySchedule();
        }
        var item = new StagePlanItem();
        item.PropertyChanged += (_, __) => SaveStagePlan();
        StagePlan.Add(item);
        InitDrops();
    }

    private void OnRunningStateChanged(object? sender, RunningState.RunningStateChangedEventArgs e)
    {
        if (e.Idle == _lastIdleState)
        {
            return;
        }

        _lastIdleState = e.Idle;
        ResetSpecifiedInventoryRuntimeState();
    }

    private void ResetSpecifiedInventoryRuntimeState()
    {
        lock (_specifiedInventoryStateLock)
        {
            _runStartInventorySnapshot = null;
            _runReservedQuantityByDropId = [];
            _specifiedInventoryTaskStates = [];
        }

        Execute.OnUIThread(NotifySpecifiedDropsStateChanged);
    }

    public static FightSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets a value indicating whether a stage plan item is being dragged.
    /// </summary>
    public bool IsDragging { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or private sets the list of stages.
    /// </summary>
    public ObservableCollection<StageSourceItem> StageListSource { get => field; private set => SetAndNotify(ref field, value); } = [];

    private static readonly Dictionary<string, string> _stageDictionary = new()
        {
            { "AN", AnnihilationName },
            { "剿灭", AnnihilationName },
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
            if (IsSpecifiedInventoryLocked)
            {
                return;
            }

            if (!SetTaskConfig<FightTask>(t => t.EnableTargetDrop == value, t => t.EnableTargetDrop = value))
            {
                return;
            }

            SetFightParams();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 指定材料按库存目标计算.
    /// </summary>
    public bool UseInventoryTarget
    {
        get => GetTaskConfig<FightTask>().UseInventoryTarget;
        set {
            if (!_runningState.Idle)
            {
                return;
            }

            if (!SetTaskConfig<FightTask>(t => t.UseInventoryTarget == value, t => t.UseInventoryTarget = value))
            {
                return;
            }

            NotifySpecifiedDropsStateChanged();
            SetFightParams();
        }
    }

    public bool IsSpecifiedInventoryLocked => UseInventoryTarget && !_runningState.Idle;

    public bool UseDropQuantityMode
    {
        get => !UseInventoryTarget;
        set {
            if (value)
            {
                UseInventoryTarget = false;
            }
        }
    }

    public bool UseTargetInventoryMode
    {
        get => UseInventoryTarget;
        set {
            if (value)
            {
                UseInventoryTarget = true;
            }
        }
    }

    public string SpecifiedDropsQuantityLabel => LocalizationHelper.GetString(UseInventoryTarget ? "TargetInventory" : "Quantity");

    public string CurrentDropsInventoryText => FormatSpecifiedDropsCount(GetSpecifiedDropsInventoryCount(GetTaskConfig<FightTask>(), GetCurrentFightTaskId()));

    public string EffectiveDropsQuantityText => GetSpecifiedDropsCoreQuantity(GetTaskConfig<FightTask>(), GetCurrentFightTaskId()).FormatNumber(false);

    private static string FormatSpecifiedDropsCount(int? count) => count is int value ? value.FormatNumber(false) : "--";

    private static Dictionary<string, int> CreateCurrentInventorySnapshot()
    {
        var depotResult = Instances.ToolboxViewModel?.DepotResult;
        if (depotResult == null || depotResult.Count == 0)
        {
            return [];
        }

        return depotResult
            .Where(item => item.Count >= 0)
            .ToDictionary(item => item.Id, item => item.Count);
    }

    private IReadOnlyDictionary<string, int> GetEffectiveInventorySnapshot()
    {
        lock (_specifiedInventoryStateLock)
        {
            if (_runStartInventorySnapshot != null)
            {
                return _runStartInventorySnapshot;
            }
        }

        return CreateCurrentInventorySnapshot();
    }

    private int? GetCurrentFightTaskId()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is not FightTask fight)
        {
            return null;
        }

        int index = ConfigFactory.CurrentConfig.TaskQueue.IndexOf(fight);
        if (index < 0 || index >= Instances.TaskQueueViewModel.TaskItemViewModels.Count)
        {
            return null;
        }

        var taskIds = Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskIds;
        return taskIds.Count > 0 ? taskIds[0] : null;
    }

    private SpecifiedInventoryTaskState? GetSpecifiedInventoryTaskState(int? taskId)
    {
        if (taskId is not int id || id <= 0)
        {
            return null;
        }

        lock (_specifiedInventoryStateLock)
        {
            return _specifiedInventoryTaskStates.GetValueOrDefault(id);
        }
    }

    private int? GetSpecifiedDropsInventoryCount(FightTask fight, int? taskId = null)
    {
        if (string.IsNullOrEmpty(fight.DropId))
        {
            return null;
        }

        if (GetSpecifiedInventoryTaskState(taskId) is { } taskState && taskState.DropId == fight.DropId)
        {
            return taskState.StartInventory;
        }

        var snapshot = GetEffectiveInventorySnapshot();
        return snapshot.TryGetValue(fight.DropId, out var count) ? count : 0;
    }

    private int GetSpecifiedDropsReservedQuantityBeforeCurrentTask(FightTask currentFight, int inventoryCount)
    {
        int currentIndex = TaskSettingVisibilityInfo.Instance.CurrentIndex;
        if (currentIndex <= 0 || string.IsNullOrEmpty(currentFight.DropId))
        {
            return 0;
        }

        int reserved = 0;
        for (int index = 0; index < currentIndex; ++index)
        {
            if (ConfigFactory.CurrentConfig.TaskQueue[index] is not FightTask previousFight ||
                previousFight.IsEnable == false ||
                previousFight.EnableTargetDrop == false ||
                !previousFight.UseInventoryTarget ||
                previousFight.DropId != currentFight.DropId)
            {
                continue;
            }

            reserved += Math.Max(previousFight.DropCount - inventoryCount - reserved, 0);
        }

        return reserved;
    }

    private int GetSpecifiedDropsCoreQuantity(FightTask fight, int? taskId = null)
    {
        if (!fight.UseInventoryTarget)
        {
            return fight.DropCount;
        }

        if (GetSpecifiedInventoryTaskState(taskId) is { } taskState)
        {
            return taskState.EffectiveQuantity;
        }

        int inventoryCount = GetSpecifiedDropsInventoryCount(fight) ?? 0;
        int reservedQuantity = GetSpecifiedDropsReservedQuantityBeforeCurrentTask(fight, inventoryCount);
        return Math.Max(fight.DropCount - inventoryCount - reservedQuantity, 0);
    }

    private SpecifiedInventoryTaskState CreateSpecifiedInventoryTaskState(FightTask fight)
    {
        lock (_specifiedInventoryStateLock)
        {
            _runStartInventorySnapshot ??= CreateCurrentInventorySnapshot();

            int startInventory = _runStartInventorySnapshot.GetValueOrDefault(fight.DropId);
            int reservedQuantity = _runReservedQuantityByDropId.GetValueOrDefault(fight.DropId);
            int effectiveQuantity = Math.Max(fight.DropCount - startInventory - reservedQuantity, 0);
            return new(fight.DropId, startInventory, effectiveQuantity);
        }
    }

    private void RememberSpecifiedInventoryTaskState(int taskId, SpecifiedInventoryTaskState taskState)
    {
        if (taskId <= 0)
        {
            return;
        }

        lock (_specifiedInventoryStateLock)
        {
            _specifiedInventoryTaskStates[taskId] = taskState;
            _runReservedQuantityByDropId[taskState.DropId] = _runReservedQuantityByDropId.GetValueOrDefault(taskState.DropId) + taskState.EffectiveQuantity;
        }
    }

    private void NotifySpecifiedDropsStateChanged()
    {
        NotifyOfPropertyChange(nameof(IsSpecifiedInventoryLocked));
        NotifyOfPropertyChange(nameof(UseDropQuantityMode));
        NotifyOfPropertyChange(nameof(UseTargetInventoryMode));
        NotifyOfPropertyChange(nameof(SpecifiedDropsQuantityLabel));
        NotifyOfPropertyChange(nameof(CurrentDropsInventoryText));
        NotifyOfPropertyChange(nameof(EffectiveDropsQuantityText));
    }

    public void RefreshSpecifiedDropsDependenciesForCurrentTask(int changedTaskIndex)
    {
        if (!_runningState.Idle ||
            TaskSettingVisibilityInfo.CurrentTask is not FightTask currentFight ||
            !currentFight.UseInventoryTarget ||
            string.IsNullOrEmpty(currentFight.DropId))
        {
            return;
        }

        int currentIndex = TaskSettingVisibilityInfo.Instance.CurrentIndex;
        if (changedTaskIndex < 0 || changedTaskIndex >= currentIndex)
        {
            return;
        }

        if (ConfigFactory.CurrentConfig.TaskQueue[changedTaskIndex] is not FightTask changedFight ||
            changedFight.EnableTargetDrop == false ||
            !changedFight.UseInventoryTarget ||
            changedFight.DropId != currentFight.DropId)
        {
            return;
        }

        NotifySpecifiedDropsStateChanged();
        SetFightParams();
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
            if (IsSpecifiedInventoryLocked)
            {
                return;
            }

            if (!SetTaskConfig<FightTask>(t => t.DropId == value, t => t.DropId = value))
            {
                return;
            }

            NotifySpecifiedDropsStateChanged();
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
        if (IsSpecifiedInventoryLocked)
        {
            RefreshDropName();
            NotifySpecifiedDropsStateChanged();
            return;
        }

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
            if (IsSpecifiedInventoryLocked)
            {
                return;
            }

            if (!SetTaskConfig<FightTask>(t => t.DropCount == value, t => t.DropCount = value))
            {
                return;
            }

            NotifySpecifiedDropsStateChanged();
            SetFightParams();
        }
    }

    #endregion Drops

    public string StagePlanTip { get => field; set => SetAndNotify(ref field, value); } = string.Empty;

    public void StagePlanTipRefresh()
    {
        var stage = GetFightStage(StagePlan.Select(i => i.Stage)) ?? "--";
        if (stage == string.Empty)
        {
            stage = LocalizationHelper.GetString("DefaultStage");
        }

        StagePlanTip = LocalizationHelper.GetStringFormat("StagePlanTip", stage);
        if (CustomStageCode)
        {
            StagePlanTip += $"\n\n{LocalizationHelper.GetString("CustomStageCodeTip")}";
        }
    }

    public static Dictionary<string, string> AnnihilationModeList { get; } = new()
    {
        { LocalizationHelper.GetString("Annihilation.Current"), AnnihilationName },
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
                StageListSource.FirstOrDefault(i => i.Value == AnnihilationName)?.Display = UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == AnnihilationStage).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
            }
        }
    }

    public string AnnihilationStage
    {
        get => GetTaskConfig<FightTask>().AnnihilationStage;
        set {
            SetTaskConfig<FightTask>(t => t.AnnihilationStage == value, t => t.AnnihilationStage = value);
            StageListSource.FirstOrDefault(i => i.Value == AnnihilationName)?.Display = UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == value).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
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

    public List<GenericCombinedData<int>> MedicineExpireDayList { get; } = [
        new() { Display = "24h", Value = 1 },
        new() { Display = "48h", Value = 2 },
        new() { Display = "72h", Value = 3 },
        new() { Display = "96h", Value = 4 },
        new() { Display = "120h", Value = 5 },
        new() { Display = "144h", Value = 6 },
        new() { Display = "168h", Value = 7 },
    ];

    public int MedicineExpireDays
    {
        get => GetTaskConfig<FightTask>().MedicineExpireDays;
        set {
            SetTaskConfig<FightTask>(t => t.MedicineExpireDays == value, t => t.MedicineExpireDays = value);
            SetFightParams();
        }
    }

    public bool UseExpireMedicineForActivity
    {
        get => GetTaskConfig<FightTask>().UseExpireMedicineForActivity;
        set {
            SetTaskConfig<FightTask>(t => t.UseExpireMedicineForActivity == value, t => t.UseExpireMedicineForActivity = value);
            SetFightParams();
        }
    }

    public bool ActivityExpireIn2Days { get => field; set => SetAndNotify(ref field, value); }

    public string ActivityInfo { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

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
        set {
            if (SetTaskConfig<FightTask>(t => t.UseWeeklySchedule == value, t => t.UseWeeklySchedule = value) && value)
            {
                HideUnavailableStage = false;
            }
        }
    }

    public ObservableCollection<WeeklyScheduleItem> WeeklyScheduleSource { get; set; } = [.. Enum.GetValues<DayOfWeek>().Select(i => new WeeklyScheduleItem(i))];

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

    public static string? GetFightStage(IEnumerable<string> list)
    {
        var stage = list?.FirstOrDefault(s => Instances.StageManager.IsStageOpen(s, Instances.TaskQueueViewModel.CurDayOfWeek));
        _logger.Information("GetFightStage: from {list}, selected {stage}", list, stage);
        return stage;
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not FightTask fight)
        {
            return;
        }
        using var refresh = new UiRefreshingScope();
        if (!UseAlternateStage && fight.StagePlan.Count == 0)
        {
            fight.StagePlan.Add(string.Empty);
        }
        RefreshStageList();
        RefreshCurrentStagePlan();
        RefreshWeeklySchedule();
        RefreshDropName();
        NotifySpecifiedDropsStateChanged();
        Refresh();
    }

    private bool? SetFightParams()
    {
        if (IsRefreshingUI || TaskSettingVisibilityInfo.CurrentTask is not FightTask fight)
        {
            return null;
        }

        if (ConfigFactory.CurrentConfig.TaskQueue.IndexOf(fight) is int index && index > -1 && Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskIds.Count > 0)
        {
            return SerializeTask(fight, Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskIds[0]).IsSuccess;
        }
        return null;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    #region 关卡列表更新

    /// <summary>
    /// Updates stage list.
    /// 使用手动输入时，只更新关卡列表，不更新关卡选择
    /// 使用隐藏当日不开放时，更新关卡列表，关卡选择为未开放的关卡时清空
    /// 使用备选关卡时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 啥都不选时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 除手动输入外所有情况下，如果剩余理智为未开放的关卡，会被清空
    /// </summary>
    /// <returns>更新任务列表的Task</returns>
    // FIXME: 被注入对象只能在private函数内使用，只有Model显示之后才会被注入。如果Model还没有触发OnInitialActivate时调用函数会NullPointerException
    // 这个函数被列为public可见，意味着他注入对象前被调用
    public Task UpdateStageList()
    {
        return Execute.OnUIThreadAsync(async () => {
            using var log = new LogScope(_logger);
            var stageList = Instances.StageManager.GetStageList();
            await TaskQueueViewModel.TaskQueueSerializingLock.WaitAsync();
            var time = DateTimeOffset.Now;
            var activityList = Instances.StageManager.ActivityList.Where(ss => ss.Value.Info.StartTimeUtc <= time && time <= ss.Value.Info.ExpireTimeUtc);
            if (activityList.Any())
            {
                var activity = activityList.First();
                var timeLeft = activity.Value.Info.ExpireTimeUtc - time;
                var day = timeLeft.Days > 0 ? $"{timeLeft.Days}+" : LocalizationHelper.GetString("LessThanOneDay");
                ActivityExpireIn2Days = timeLeft.Days < 2;
                ActivityInfo = $"｢{activity.Value.Info.StageName}｣ {LocalizationHelper.GetString("DaysLeftOpen")}{day}";
            }
            else
            {
                ActivityInfo = LocalizationHelper.GetString("NoActivity");
                ActivityExpireIn2Days = false;
            }
            using (var refresh = new UiRefreshingScope())
            {
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
            }
            TaskQueueViewModel.TaskQueueSerializingLock.Release();

            foreach (var (item, index) in Instances.TaskQueueViewModel.TaskItemViewModels.Select((i, index) => (i, index)))
            {
                if (ConfigFactory.Root.CurrentConfig.TaskQueue[index] is FightTask fight && item.TaskIds.Count == 1 && Instances.AsstProxy.TasksStatus.ContainsKey(item.TaskIds[0]))
                {
                    if (SerializeTask(fight, Instances.TaskQueueViewModel.TaskItemViewModels[index].TaskIds[0]).IsSuccess is not true)
                    {
                        _logger.Warning("Failed to serialize task {taskId} when updating stage list", item.TaskIds[0]);
                    }
                }
            }
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
        listSource.FirstOrDefault(i => i.Value == AnnihilationName)?.Display = current.UseCustomAnnihilation ? (AnnihilationModeList.FirstOrDefault(i => i.Value == current.AnnihilationStage).Key ?? LocalizationHelper.GetString("Annihilation.Current")) : LocalizationHelper.GetString("Annihilation.Current");
        StageListSource = [.. listSource];
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
        StagePlan = [.. list];
        StagePlan.CollectionChanged += (_, __) => SaveStagePlan();
        SetFightParams(); // 恢复StagePlan后, 修复AsstFightTask的Stage
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

    #region Data Class

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

    #endregion Data Class

    #region UI Item

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

    #endregion UI Item

    private struct UiRefreshingScope : IDisposable
    {
        private static int _depth = 0;

        public UiRefreshingScope()
        {
            ++_depth;
            Instance.IsRefreshingUI = true;
        }

        readonly void IDisposable.Dispose()
        {
            --_depth;
            if (_depth == 0)
            {
                Instance.IsRefreshingUI = false;
            }
        }
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not FightTask fight || taskId is int and <= 0)
            {
                return (null, []);
            }

            if (fight.UseWeeklySchedule && fight.WeeklySchedule.TryGetValue(Instances.TaskQueueViewModel.CurDayOfWeek, out var isEnabled) && !isEnabled)
            {
                return (null, []);
            }

            string? stage = GetFightStage(fight.StagePlan);
            if (stage is null)
            {
                return (null, []);
            }

            var time = DateTimeOffset.Now;
            var activityExpireIn2Days = false;
            var activityList = Instances.StageManager.ActivityList.Where(ss => ss.Value.Info.StartTimeUtc <= time && time <= ss.Value.Info.ExpireTimeUtc);
            if (activityList.Any())
            {
                var activity = activityList.First();
                activityExpireIn2Days = (activity.Value.Info.ExpireTimeUtc - time).Days < 2;
            }

            var expireDays = fight.UseExpiringMedicine ? fight.MedicineExpireDays : 0;
            var yjTime = DateTimeOffset.Now.ToYjDateTime().ToLocalTime();
            var daysUntilEndOfWeek = ((7 - (int)yjTime.DayOfWeek + 7) % 7) + 1; // 距离本周结束的天数, 用鹰历计算
            var activityExpireDays = activityExpireIn2Days && fight.UseExpireMedicineForActivity ? daysUntilEndOfWeek : 0;
            SpecifiedInventoryTaskState? specifiedInventoryTaskState = null;
            int specifiedDropsQuantity = fight.DropCount;

            if (fight.EnableTargetDrop != false && !string.IsNullOrEmpty(fight.DropId) && fight.UseInventoryTarget)
            {
                specifiedInventoryTaskState = Instance.GetSpecifiedInventoryTaskState(taskId) ?? Instance.CreateSpecifiedInventoryTaskState(fight);
                specifiedDropsQuantity = specifiedInventoryTaskState.EffectiveQuantity;
                if (specifiedDropsQuantity <= 0)
                {
                    return (null, []);
                }
            }

            var effectiveMaxTimes = fight.EnableTimesLimit != false ? fight.TimesLimit : int.MaxValue;

            var task = new AsstFightTask() {
                Stage = stage,
                Medicine = fight.UseMedicine != false ? fight.MedicineCount : 0,
                Stone = fight.UseStone != false ? fight.StoneCount : 0,
                Series = fight.Series,
                MaxTimes = effectiveMaxTimes,
                MedicineExpireDays = Math.Max(expireDays, activityExpireDays),
                IsDrGrandet = fight.IsDrGrandet,
                ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
                ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
                PenguinId = SettingsViewModel.GameSettings.PenguinId,
                YituliuId = SettingsViewModel.GameSettings.PenguinId,
                ServerType = Instances.SettingsViewModel.ServerType,
                ClientType = SettingsViewModel.GameSettings.ClientType,
            };

            if (task.Stage == AnnihilationName && fight.UseCustomAnnihilation)
            {
                task.Stage = fight.AnnihilationStage;
            }

            if (fight.EnableTargetDrop != false && !string.IsNullOrEmpty(fight.DropId) && specifiedDropsQuantity > 0)
            {
                task.Drops.Add(fight.DropId, specifiedDropsQuantity);
            }

            if (effectiveMaxTimes > 0 && effectiveMaxTimes < int.MaxValue && fight.Series > 0 && effectiveMaxTimes % fight.Series != 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("FightTimesMayNotExhausted", effectiveMaxTimes, fight.Series), UiLogColor.Warning);
            }

            if (taskId is int id and > 0)
            {
                bool updated = Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task);
                if (updated && specifiedInventoryTaskState != null && Instance.GetSpecifiedInventoryTaskState(id) == null)
                {
                    Instance.RememberSpecifiedInventoryTaskState(id, specifiedInventoryTaskState);
                }

                return (updated, [id]);
            }

            if (taskId is null)
            {
                var appendResult = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, task);
                if (appendResult.IsSuccess && specifiedInventoryTaskState != null)
                {
                    Instance.RememberSpecifiedInventoryTaskState(appendResult.TaskId, specifiedInventoryTaskState);
                }

                return (appendResult.IsSuccess, appendResult.TaskId > 0 ? [appendResult.TaskId] : []);
            }

            return (null, []);
        }
    }

    private sealed record SpecifiedInventoryTaskState(string DropId, int StartInventory, int EffectiveQuantity);
}
