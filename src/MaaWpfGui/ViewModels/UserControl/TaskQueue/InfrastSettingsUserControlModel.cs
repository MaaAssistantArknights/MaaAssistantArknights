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
    }

    public InfrastSettingsUserControlModel()
    {
        _runningState = RunningState.Instance;
    }

    public static InfrastSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<InfrastSettingsUserControlModel>();
    private readonly RunningState _runningState;

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
    }

    /// <summary>
    /// Gets or sets the infrast item view models.
    /// </summary>
    public ObservableCollection<InfrastRoomItemViewModel> InfrastRoomModels { get; set; } = [];

    /// <summary>
    /// Gets the list of uses of drones.
    /// </summary>
    public List<CombinedData> UsesOfDronesList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("DronesNotUse"), Value = "_NotUse" },
            new() { Display = LocalizationHelper.GetString("Money"), Value = "Money" },
            new() { Display = LocalizationHelper.GetString("SyntheticJade"), Value = "SyntheticJade" },
            new() { Display = LocalizationHelper.GetString("CombatRecord"), Value = "CombatRecord" },
            new() { Display = LocalizationHelper.GetString("PureGold"), Value = "PureGold" },
            new() { Display = LocalizationHelper.GetString("OriginStone"), Value = "OriginStone" },
            new() { Display = LocalizationHelper.GetString("Chip"), Value = "Chip" },
        ];

    /// <summary>
    /// Gets the list of uses of default infrast.
    /// </summary>
    public List<CombinedData> DefaultInfrastList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("UserDefined"), Value = UserDefined },
            new() { Display = LocalizationHelper.GetString("153Time3"), Value = "153_layout_3_times_a_day.json" },
            new() { Display = LocalizationHelper.GetString("153Time4"), Value = "153_layout_4_times_a_day.json" },
            new() { Display = LocalizationHelper.GetString("243Time3"), Value = "243_layout_3_times_a_day.json" },
            new() { Display = LocalizationHelper.GetString("243Time4"), Value = "243_layout_4_times_a_day.json" },
            new() { Display = LocalizationHelper.GetString("333Time3"), Value = "333_layout_for_Orundum_3_times_a_day.json" },
        ];

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
    }

    /// <summary>
    /// Gets the list of uses of infrast mode.
    /// </summary>
    public List<GenericCombinedData<Mode>> InfrastModeList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("InfrastModeNormal"), Value = Mode.Normal },
        new() { Display = LocalizationHelper.GetString("InfrastModeRotation"), Value = Mode.Rotation },
        new() { Display = LocalizationHelper.GetString("InfrastModeCustom"), Value = Mode.Custom },
    ];

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

            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastMode, value.ToString());
            ParseCustomInfrastPlan();
        }
    }

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string UsesOfDrones
    {
        get => GetTaskConfig<InfrastTask>().UsesOfDrones;
        set => SetTaskConfig<InfrastTask>(t => t.UsesOfDrones == value, t => t.UsesOfDrones = value);
    }

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

    private string _defaultInfrast = ConfigurationHelper.GetValue(ConfigurationKeys.DefaultInfrast, UserDefined);

    public const string UserDefined = "user_defined";

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string DefaultInfrast
    {
        get => _defaultInfrast;
        set {
            SetAndNotify(ref _defaultInfrast, value);
            if (_defaultInfrast != UserDefined)
            {
                CustomInfrastFile = Path.Combine(PathsHelper.ResourceDir, "custom_infrast", value);
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.DefaultInfrast, value);
        }
    }

    [PropertyDependsOn(nameof(DefaultInfrast))]
    public bool IsCustomInfrastFileReadOnly => _defaultInfrast != UserDefined;

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

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        bool? ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not InfrastTask infrast)
            {
                return null;
            }

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
                int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
                null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Infrast, task),
                _ => null,
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
