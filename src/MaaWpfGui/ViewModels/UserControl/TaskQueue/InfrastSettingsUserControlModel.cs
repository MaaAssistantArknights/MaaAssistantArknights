// <copyright file="InfrastSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

using static MaaWpfGui.Main.AsstProxy;
using Mode = InfrastMode;

/// <summary>
/// 基建任务
/// </summary>
public class InfrastSettingsUserControlModel : TaskSettingsViewModel, InfrastSettingsUserControlModel.ISerialize
{
    static InfrastSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public InfrastSettingsUserControlModel()
    {
        _runningState = RunningState.Instance;
    }

    public static InfrastSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<InfrastSettingsUserControlModel>();
    private readonly RunningState _runningState;

    // 程序性重排（平衡模式自动纠正顺序）时置位，避免触发“用户拖回”的弹窗检测
    private bool _isProgrammaticReorder;

    // 上次检查时“无人机用途所需房间未勾选”的状态，用于仅在进入非法状态时弹窗一次
    private bool _lastDroneRoomMissing;

    /// <summary>
    /// Gets the visibility of task setting views.
    /// </summary>
    public static TaskSettingVisibilityInfo TaskSettingVisibilities => TaskSettingVisibilityInfo.Instance;

    private void RefreshInfrastRoomList()
    {
        var preList = GetTaskConfig<InfrastTask>().RoomList;
        var set = new HashSet<InfrastRoomType>(preList.Select(i => i.Room));

        // 房间列表不完整，补全
        if (set.Count != Enum.GetValues<InfrastRoomType>().Length || set.Count != preList.Count)
        {
            var list = new List<InfrastTask.RoomInfo>(preList);
            foreach (var room in Enum.GetValues<InfrastRoomType>())
            {
                if (!set.Contains(room))
                {
                    list.Add(new InfrastTask.RoomInfo(room, false));
                }
            }
            SetTaskConfig<InfrastTask>(t => t.RoomList.SequenceEqual(list), t => t.RoomList = list);
        }
        var roomList = new List<InfrastRoomItemViewModel>();
        foreach (var (room, isEnabled) in GetTaskConfig<InfrastTask>().RoomList)
        {
            var item = new InfrastRoomItemViewModel(room, isEnabled);
            item.PropertyChanged += (sender, args) => {
                if (args.PropertyName == nameof(InfrastRoomItemViewModel.IsEnabled))
                {
                    InfrastOrderSelectionChanged(sender, null);
                }
            };
            roomList.Add(item);
        }

        InfrastRoomModels = new ObservableCollection<InfrastRoomItemViewModel>(roomList);
        InfrastRoomModels.CollectionChanged += InfrastOrderSelectionChanged;

        // 加载配置时同样保证平衡模式的“贸易站在制造站前”顺序
        EnsureDroneBalanceOrder();

        // 加载配置时不弹窗，仅记录当前“无人机用途所需房间未勾选”的基线状态
        _lastDroneRoomMissing = GetMissingDroneRooms().Count > 0;
    }

    /// <summary>
    /// Gets or sets the infrast item view models.
    /// </summary>
    public ObservableCollection<InfrastRoomItemViewModel> InfrastRoomModels { get; set; } = [];

    /// <summary>
    /// Gets the list of uses of drones.
    /// </summary>
    public LocalizedObservableList<string> UsesOfDronesList { get; } = new(
        ("_NotUse", "DronesNotUse"),
        ("Money", "Money"),
        ("SyntheticJade", "SyntheticJade"),
        ("CombatRecord", "CombatRecord"),
        ("PureGold", "PureGold"),
        ("OriginStone", "OriginStone"),
        ("Chip", "Chip"),
        ("PureGold-Money", "PureGoldMoney"),
        ("OriginStone-SyntheticJade", "OriginStoneSyntheticJade"));

    /// <summary>
    /// Gets the list of uses of default infrast.
    /// </summary>
    public LocalizedObservableList<string> DefaultInfrastList { get; } = new(
        (UserDefined, "UserDefined"),
        ("153_layout_3_times_a_day.json", "153Time3"),
        ("153_layout_4_times_a_day.json", "153Time4"),
        ("243_layout_3_times_a_day.json", "243Time3"),
        ("243_layout_4_times_a_day.json", "243Time4"),
        ("333_layout_for_Orundum_3_times_a_day.json", "333Time3"));

    /// <summary>
    /// Gets or sets the threshold to enter dormitory.
    /// </summary>
    public int DormThreshold
    {
        get => GetTaskConfig<InfrastTask>().DormThreshold;
        set => SetTaskConfig<InfrastTask>(t => t.DormThreshold == value, t => t.DormThreshold = value);
    }

    /// <summary>
    /// Gets infrast order list.
    /// </summary>
    /// <returns>The infrast order list.</returns>
    public List<string> GetInfrastOrderList() => [.. InfrastRoomModels.Where(i => i.IsEnabled).Select(i => i.Name)];

    // UI 绑定的方法
    [UsedImplicitly]
    public void InfrastItemSelectedAll()
    {
        foreach (var item in InfrastRoomModels)
        {
            item.IsEnabled = true;
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void InfrastItemUnselectedAll()
    {
        foreach (var item in InfrastRoomModels)
        {
            item.IsEnabled = false;
        }
    }

    /// <summary>
    /// 基建房间顺序、启用变更后存一下
    /// </summary>
    /// <param name="sender">ignored object</param>
    /// <param name="e">ignored NotifyCollectionChangedEventArgs</param>
    private void InfrastOrderSelectionChanged(object? sender, NotifyCollectionChangedEventArgs? e)
    {
        var list = InfrastRoomModels.Select<InfrastRoomItemViewModel, InfrastTask.RoomInfo>(i => new(i.RoomType, i.IsEnabled)).ToList();
        SetTaskConfig<InfrastTask>(t => t.RoomList.SequenceEqual(list), t => t.RoomList = list);

        // 平衡模式下强制贸易站在制造站前：用户手动把制造站拖回贸易站前时，弹窗并回滚顺序
        if (!_isProgrammaticReorder && IsDroneBalanceOrderViolated())
        {
            _isProgrammaticReorder = true;
            Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.Background, (Action)(() => {
                try
                {
                    MessageBoxHelper.Show(
                        LocalizationHelper.GetString("DroneBalanceOrderRollback"),
                        buttons: MessageBoxButton.OK,
                        icon: MessageBoxImage.Warning);
                    EnsureDroneBalanceOrder();
                }
                finally
                {
                    _isProgrammaticReorder = false;
                }
            }));
        }

        // 无人机用途所需房间未勾选时提示
        CheckDroneUsageRoomMissing();
    }

    /// <summary>
    /// 无人机自动平衡模式（PureGold-Money / OriginStone-SyntheticJade）是否激活且非自定义模式
    /// </summary>
    private bool IsDroneBalanceModeEnabled()
        => UsesOfDrones is "PureGold-Money" or "OriginStone-SyntheticJade" && InfrastMode != Mode.Custom;

    /// <summary>
    /// 平衡模式下是否违反了“贸易站在制造站前”的顺序（仅统计启用中的房间）
    /// </summary>
    private bool IsDroneBalanceOrderViolated()
    {
        if (!IsDroneBalanceModeEnabled())
        {
            return false;
        }

        int? mfgIndex = null;
        int? tradeIndex = null;
        for (int i = 0; i < InfrastRoomModels.Count; ++i)
        {
            var item = InfrastRoomModels[i];
            if (!item.IsEnabled)
            {
                continue;
            }
            if (item.RoomType == InfrastRoomType.Mfg && mfgIndex == null)
            {
                mfgIndex = i;
            }
            else if (item.RoomType == InfrastRoomType.Trade && tradeIndex == null)
            {
                tradeIndex = i;
            }
        }

        return mfgIndex != null && tradeIndex != null && mfgIndex < tradeIndex;
    }

    /// <summary>
    /// 平衡模式下把贸易站提到制造站前（其余保持原序），并持久化
    /// </summary>
    private void EnsureDroneBalanceOrder()
    {
        if (!IsDroneBalanceModeEnabled())
        {
            return;
        }

        int mfgIndex = IndexOfRoom(InfrastRoomType.Mfg);
        int tradeIndex = IndexOfRoom(InfrastRoomType.Trade);
        if (mfgIndex >= 0 && tradeIndex >= 0 && mfgIndex < tradeIndex)
        {
            _isProgrammaticReorder = true;
            try
            {
                InfrastRoomModels.Move(tradeIndex, mfgIndex);
            }
            finally
            {
                _isProgrammaticReorder = false;
            }
        }
    }

    private int IndexOfRoom(InfrastRoomType roomType)
    {
        for (int i = 0; i < InfrastRoomModels.Count; ++i)
        {
            if (InfrastRoomModels[i].RoomType == roomType)
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>
    /// 当前无人机用途所需、但未勾选的房间本地化名列表（Custom 模式或不使用无人机时为空）
    /// </summary>
    private List<string> GetMissingDroneRooms()
        => GetMissingDroneRooms(UsesOfDrones, InfrastMode, InfrastRoomModels.Select(m => (m.RoomType, m.IsEnabled)));

    /// <summary>
    /// 根据无人机用途与房间勾选状态，返回所需但未勾选的房间本地化名列表（Custom 模式或不使用无人机时为空）
    /// </summary>
    /// <param name="usesOfDrones">无人机用途</param>
    /// <param name="mode">基建模式</param>
    /// <param name="rooms">房间类型与其勾选状态</param>
    private static List<string> GetMissingDroneRooms(string usesOfDrones, Mode mode, IEnumerable<(InfrastRoomType Room, bool IsEnabled)> rooms)
    {
        if (mode == Mode.Custom || usesOfDrones == "_NotUse")
        {
            return [];
        }

        List<InfrastRoomType> required = usesOfDrones switch {
            "Money" or "SyntheticJade" => [InfrastRoomType.Trade],
            "CombatRecord" or "PureGold" or "OriginStone" or "Chip" => [InfrastRoomType.Mfg],
            "PureGold-Money" or "OriginStone-SyntheticJade" => [InfrastRoomType.Trade, InfrastRoomType.Mfg],
            _ => [],
        };

        return required.Where(r => rooms.FirstOrDefault(m => m.Room == r).IsEnabled != true)
                       .Select(r => LocalizationHelper.GetString(r.ToString()))
                       .ToList();
    }

    /// <summary>
    /// 无人机用途所需房间未勾选时的非阻断提示（弹窗 + 日志）
    /// </summary>
    /// <param name="missing">缺失的房间本地化名列表</param>
    /// <param name="usesOfDrones">无人机用途</param>
    private static void ShowDroneUsageRoomMissingWarning(List<string> missing, string usesOfDrones)
    {
        if (missing.Count == 0)
        {
            return;
        }

        var usageDisplay = Instance.UsesOfDronesList.FirstOrDefault(i => i.Value == usesOfDrones)?.Display ?? usesOfDrones;
        var message = LocalizationHelper.GetStringFormat("DroneUsageRoomMissing", string.Join(", ", missing), usageDisplay);
        Instances.TaskQueueViewModel.AddLog(message, UiLogColor.Warning);
        Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.Background, (Action)(() => {
            MessageBoxHelper.Show(message, LocalizationHelper.GetString("Warning"), MessageBoxButton.OK, MessageBoxImage.Warning);
        }));
    }

    /// <summary>
    /// 无人机用途所需房间检查：仅在配置从合法变为非法时提示一次（运行中配置被锁定，不会触发）
    /// </summary>
    private void CheckDroneUsageRoomMissing()
    {
        var missingRooms = GetMissingDroneRooms();
        bool missing = missingRooms.Count > 0;
        if (missing && !_lastDroneRoomMissing && !_isProgrammaticReorder)
        {
            ShowDroneUsageRoomMissingWarning(missingRooms, UsesOfDrones);
        }

        _lastDroneRoomMissing = missing;
    }

    /// <summary>
    /// Gets the list of uses of infrast mode.
    /// </summary>
    public LocalizedObservableList<Mode> InfrastModeList { get; } = new(
        (Mode.Normal, "InfrastModeNormal"),
        (Mode.Rotation, "InfrastModeRotation"),
        (Mode.Custom, "InfrastModeCustom"));

    /// <summary>
    /// Gets or sets the infrast mode.
    /// </summary>
    public Mode InfrastMode
    {
        get => GetTaskConfig<InfrastTask>().Mode;
        set {
            if (!SetTaskConfig<InfrastTask>(t => t.Mode == value, t => t.Mode = value))
            {
                return;
            }

            ParseCustomInfrastPlan();
            EnsureDroneBalanceOrder();
        }
    }

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string UsesOfDrones
    {
        get => GetTaskConfig<InfrastTask>().UsesOfDrones;
        set {
            SetTaskConfig<InfrastTask>(t => t.UsesOfDrones == value, t => t.UsesOfDrones = value);
            EnsureDroneBalanceOrder();
            CheckDroneUsageRoomMissing();
        }
    }

    /// <summary>
    /// Gets or sets 无人机自动平衡阈值（PureGold-Money / OriginStone-SyntheticJade 使用）
    /// </summary>
    public int DroneUsageThreshold
    {
        get => GetTaskConfig<InfrastTask>().DroneUsageThreshold;
        set => SetTaskConfig<InfrastTask>(t => t.DroneUsageThreshold == value, t => t.DroneUsageThreshold = value);
    }

    /// <summary>
    /// Gets a value indicating whether 是否显示无人机自动平衡阈值输入（选择 PureGold-Money / OriginStone-SyntheticJade 且非自定义模式）
    /// </summary>
    [PropertyDependsOn(nameof(UsesOfDrones), nameof(InfrastMode))]
    public bool IsDroneUsageThresholdVisible
        => UsesOfDrones is "PureGold-Money" or "OriginStone-SyntheticJade" && InfrastMode != Mode.Custom;

    public bool ReceptionMessageBoardReceive
    {
        get => GetTaskConfig<InfrastTask>().ReceptionMessageBoard;
        set => SetTaskConfig<InfrastTask>(t => t.ReceptionMessageBoard == value, t => t.ReceptionMessageBoard = value);
    }

    public bool ReceptionClueExchange
    {
        get => GetTaskConfig<InfrastTask>().ReceptionClueExchange;
        set => SetTaskConfig<InfrastTask>(t => t.ReceptionClueExchange == value, t => t.ReceptionClueExchange = value);
    }

    public bool ReceptionSendClue
    {
        get => GetTaskConfig<InfrastTask>().SendClue;
        set => SetTaskConfig<InfrastTask>(t => t.SendClue == value, t => t.SendClue = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to continue training after current training completed.
    /// </summary>
    public bool ContinueTraining
    {
        get => GetTaskConfig<InfrastTask>().ContinueTraining;
        set => SetTaskConfig<InfrastTask>(t => t.ContinueTraining == value, t => t.ContinueTraining = value);
    }

    public const string UserDefined = "user_defined";

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string DefaultInfrast
    {
        get => GetTaskConfig<InfrastTask>().CustomFileType;
        set {
            SetTaskConfig<InfrastTask>(t => t.CustomFileType == value, t => t.CustomFileType = value);
            if (value != UserDefined)
            {
                CustomInfrastFile = Path.Combine(PathsHelper.ResourceDir, "custom_infrast", value);
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.DefaultInfrast, value);
        }
    }

    [PropertyDependsOn(nameof(DefaultInfrast))]
    public bool IsCustomInfrastFileReadOnly => DefaultInfrast != UserDefined;

    /// <summary>
    /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
    /// </summary>
    public bool DormFilterNotStationedEnabled
    {
        get => GetTaskConfig<InfrastTask>().DormFilterNotStationed;
        set => SetTaskConfig<InfrastTask>(t => t.DormFilterNotStationed == value, t => t.DormFilterNotStationed = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether trust in dorm is enabled.
    /// </summary>
    public bool DormTrustEnabled
    {
        get => GetTaskConfig<InfrastTask>().DormTrustEnabled;
        set => SetTaskConfig<InfrastTask>(t => t.DormTrustEnabled == value, t => t.DormTrustEnabled = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
    /// </summary>
    public bool OriginiumShardAutoReplenishment
    {
        get => GetTaskConfig<InfrastTask>().OriginiumShardAutoReplenishment;
        set => SetTaskConfig<InfrastTask>(t => t.OriginiumShardAutoReplenishment == value, t => t.OriginiumShardAutoReplenishment = value);
    }

    /// <summary>
    /// Selects infrast config file.
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public void SelectCustomInfrastFile()
    {
        var dialog = new OpenFileDialog {
            Filter = LocalizationHelper.GetString("CustomInfrastFile") + "|*.json",
        };

        if (dialog.ShowDialog() == true)
        {
            CustomInfrastFile = dialog.FileName;
        }

        DefaultInfrast = UserDefined;
        if (CustomInfrastPlanList.Count > 0)
        {
            AchievementTrackerHelper.Instance.Unlock(AchievementIds.PrivateDormManager);
        }
    }

    public string CustomInfrastFile
    {
        get => GetTaskConfig<InfrastTask>().Filename;
        set {
            SetTaskConfig<InfrastTask>(t => t.Filename == value, t => t.Filename = value);
            ParseCustomInfrastPlan(true);

            int index = CustomInfrastPlanList.Any(i => i.Period.Count > 0) ? -1 : 0;
            if (index != CustomInfrastPlanSelect)
            {
                CustomInfrastPlanSelect = index;
            }
            else
            {
                NotifyOfPropertyChange(nameof(CustomInfrastPlanSelect));
            }
        }
    }

    /// <summary>
    /// Gets or sets 手动指定的自定义配置, -1: 时间轮换, 0~n: index轮换
    /// </summary>
    public int CustomInfrastPlanSelect
    {
        get => GetTaskConfig<InfrastTask>().PlanSelect;
        set {
            if (value < -1)
            {
                value = -1;
            }
            else if (value >= CustomInfrastPlanList.Count)
            {
                value = 0;
            }

            SetTaskConfig<InfrastTask>(t => t.PlanSelect == value, t => t.PlanSelect = value);
        }
    }

    private readonly GenericCombinedData<int> _defaultItem = new() { Display = LocalizationHelper.GetStringFormat("CustomInfrastTimeSchedule", string.Empty), Value = -1 };

    public List<CustomInfrastConfig.Plan> CustomInfrastPlanList
    {
        get => GetTaskConfig<InfrastTask>().InfrastPlan;
        set {
            SetTaskConfig<InfrastTask>(t => t.InfrastPlan == value, t => t.InfrastPlan = value);
            RefreshCustomInfrastPlanList();
        }
    }

    private readonly ObservableCollection<GenericCombinedData<int>> _customInfrastPlanListDisplay = [];

    public ObservableCollection<GenericCombinedData<int>> CustomPlanListDisplay
    {
        get => _customInfrastPlanListDisplay;
    }

    public void ParseCustomInfrastPlan(bool output = false)
    {
        if (InfrastMode != Mode.Custom || !File.Exists(CustomInfrastFile))
        {
            CustomInfrastPlanList = [];
            return;
        }

        try
        {
            string jsonStr = File.ReadAllText(CustomInfrastFile);
            if (JsonConvert.DeserializeObject<CustomInfrastConfig>(jsonStr) is not CustomInfrastConfig root)
            {
                throw new JsonException("DeserializeObject returned null");
            }

            if (output)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message, splitMode: UI.TaskQueueViewModel.LogCardSplitMode.Before);
                Instances.TaskQueueViewModel.AddLog($"title: {root.Title}", UiLogColor.Info);
                Instances.TaskQueueViewModel.AddLog($"description: {root.Description}", UiLogColor.Info);
            }

            var planList = root.Plans;
            var list = new List<CustomInfrastConfig.Plan>();
            var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
            for (int i = 0; i < planList.Count; ++i)
            {
                var plan = planList[i];
                plan.Index = i;
                plan.Name ??= "Plan " + ((char)('A' + i));
                plan.Description ??= string.Empty;
                plan.DescriptionPost ??= string.Empty;
                list.Add(plan);

                if (output)
                {
                    Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);
                    foreach (var period in plan.Period)
                    {
                        Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:mm} - {period[1]:HH:mm} ]");
                    }

                    Instances.TaskQueueViewModel.AddLog(plan.Description);
                    Instances.TaskQueueViewModel.AddLog(plan.DescriptionPost);
                }
            }

            if (list.Any(i => i.Period.Count > 0) && list.Any(p => p.Period.Count == 0))
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastFileHasPlanNoPeriod"), UiLogColor.Warning);
            }

            Instances.TaskQueueViewModel.AddLog(string.Empty, splitMode: UI.TaskQueueViewModel.LogCardSplitMode.After);

            CustomInfrastPlanList = [.. list];
            if (DefaultInfrast == UserDefined && list.Count > 0)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.PrivateDormManager);
            }
        }
        catch (Exception)
        {
            CustomInfrastPlanList = [];
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastFileParseFailed"), UiLogColor.Error);
        }
    }

    private void RefreshCustomInfrastPlanList()
    {
        _customInfrastPlanListDisplay.Clear();
        if (CustomInfrastPlanList.Any(i => i.Period.Count > 0))
        {
            var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
            var plan = CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1]));
            plan ??= CustomInfrastPlanList.FirstOrDefault();
            plan ??= new();
            _defaultItem.Display = LocalizationHelper.GetStringFormat("CustomInfrastTimeSchedule", plan.Name ?? "???");
            _customInfrastPlanListDisplay.Add(_defaultItem);
        }

        foreach (var item in CustomInfrastPlanList)
        {
            _customInfrastPlanListDisplay.Add(new GenericCombinedData<int> {
                Display = item.Name,
                Value = item.Index,
            });
        }

        if (_customInfrastPlanListDisplay.Any(i => i.Value == CustomInfrastPlanSelect))
        {
            NotifyOfPropertyChange(nameof(CustomInfrastPlanSelect));
        }
        else
        {
            CustomInfrastPlanSelect = -1;
        }
    }

    /// <summary>
    /// 刷新自定义基建计划第一个时间轮换项显示, 每分钟调用一次
    /// </summary>
    public void RefreshInfrastTimeRotationDisplay()
    {
        if (InfrastMode != Mode.Custom || !CustomInfrastPlanList.Any(i => i.Period.Count > 0) || CustomPlanListDisplay.Count == 0 || CustomPlanListDisplay[0].Value != -1)
        {
            return;
        }

        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        var plan = CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1]));
        plan ??= CustomInfrastPlanList.FirstOrDefault();
        plan ??= new();
        _defaultItem.Display = LocalizationHelper.GetStringFormat("CustomInfrastTimeSchedule", plan.Name ?? "???");
    }

    /// <summary>
    /// 基建任务完成一次后, 自动切换到下一个; 仅非时间轮换有效
    /// </summary>
    /// <param name="infrast">基建任务</param>
    public static void IncreaseCustomInfrastPlanIndex(InfrastTask? infrast)
    {
        if (infrast is null || infrast.Mode != Mode.Custom || infrast.PlanSelect == -1 || infrast.PlanSelect >= infrast.InfrastPlan.Count)
        {
            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
        Instances.TaskQueueViewModel.AddLog(infrast.InfrastPlan[infrast.PlanSelect].DescriptionPost);

        ++infrast.PlanSelect;
        if (infrast.PlanSelect >= infrast.InfrastPlan.Count)
        {
            infrast.PlanSelect = 0;
        }
        if (TaskSettingVisibilityInfo.CurrentTask == infrast)
        {
            Instance.NotifyOfPropertyChange(nameof(CustomInfrastPlanSelect));
        }
        OutputCurrentCustomPlanInfo(infrast);
    }

    private static void OutputCurrentCustomPlanInfo(InfrastTask infrast)
    {
        if (infrast.Mode != Mode.Custom || infrast.PlanSelect >= infrast.InfrastPlan.Count)
        {
            return;
        }

        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        CustomInfrastConfig.Plan? plan;
        if (infrast.PlanSelect == -1)
        {
            plan = infrast.InfrastPlan.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1]));
            plan ??= infrast.InfrastPlan.First();
        }
        else
        {
            plan = infrast.InfrastPlan[infrast.PlanSelect];
        }

        Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);
        foreach (var period in plan.Period)
        {
            Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:mm} - {period[1]:HH:mm} ]");
        }

        Instances.TaskQueueViewModel.AddLog(plan.Description);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is InfrastTask)
        {
            RefreshInfrastRoomList();
            RefreshCustomInfrastPlanList();
            Refresh();
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        UsesOfDronesList.RefreshLocalization();
        DefaultInfrastList.RefreshLocalization();
        InfrastModeList.RefreshLocalization();
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not InfrastTask infrast)
            {
                return (null, []);
            }

            // 启动时校验：无人机用途所需房间未勾选时弹窗 + 日志提醒（不阻断任务）
            ShowDroneUsageRoomMissingWarning(
                GetMissingDroneRooms(infrast.UsesOfDrones, infrast.Mode, infrast.RoomList.Select(r => (r.Room, r.IsEnabled))),
                infrast.UsesOfDrones);

            var task = new AsstInfrastTask {
                Mode = infrast.Mode,
                Facilitys = [.. infrast.RoomList.Where(i => i.IsEnabled).Select(i => i.Room.ToString())],
                UsesOfDrones = infrast.UsesOfDrones,
                ContinueTraining = infrast.ContinueTraining,
                DormThreshold = infrast.DormThreshold / 100.0,
                DormFilterNotStationedEnabled = infrast.DormFilterNotStationed,
                DormTrustEnabled = infrast.DormTrustEnabled,
                OriginiumShardAutoReplenishment = infrast.OriginiumShardAutoReplenishment,
                ReceptionMessageBoard = infrast.ReceptionMessageBoard,
                ReceptionClueExchange = infrast.ReceptionClueExchange,
                ReceptionSendClue = infrast.SendClue,
                Filename = infrast.Filename,
            };

            if (infrast.UsesOfDrones is "PureGold-Money" or "OriginStone-SyntheticJade")
            {
                task.DroneUsageThreshold = infrast.DroneUsageThreshold;
            }

            if (infrast.Mode != Mode.Custom)
            {
            }
            else if (infrast.PlanSelect != -1 && infrast.InfrastPlan.Count <= infrast.PlanSelect)
            {
                throw new InvalidOperationException(LocalizationHelper.GetString("CustomInfrastPlanSelectOutOfIndex"));
            }
            else if (infrast.PlanSelect >= 0)
            {
                task.PlanIndex = infrast.PlanSelect;
            }
            else
            {
                var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
                if (infrast.InfrastPlan.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1])) is { } plan)
                {
                    task.PlanIndex = plan.Index;
                }
                else
                {
                    task.PlanIndex = 0;
                    _logger.Warning("No valid plan found for current time, use PlanIndex 0");
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastFileHasPlanNoPeriod"), UiLogColor.Error);
                }
            }

            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Infrast, task)),
                _ => (null, []),
            };
        }
    }
}

public enum InfrastMode
{
    /// <summary>
    /// 普通
    /// </summary>
    Normal,

    /// <summary>
    /// 自定义
    /// </summary>
    Custom = 10000,

    /// <summary>
    /// 轮换
    /// </summary>
    Rotation = 20000,
}
