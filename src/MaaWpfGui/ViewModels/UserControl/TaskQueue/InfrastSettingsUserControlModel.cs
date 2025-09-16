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
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;
using static MaaWpfGui.Main.AsstProxy;
using Mode = InfrastMode;

/// <summary>
/// 基建任务
/// </summary>
public class InfrastSettingsUserControlModel : TaskSettingsViewModel
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
        var roomList = new List<InfrastRoomItemViewModel>();
        foreach (var (room, isEnabled) in GetTaskConfig<InfrastTask>().RoomList)
        {
            var item = new InfrastRoomItemViewModel(room, isEnabled);
            item.PropertyChanged += (sender, args) =>
            {
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
        set
        {
            if (!SetTaskConfig<InfrastTask>(t => t.Mode == value, t => t.Mode = value))
            {
                return;
            }

            RefreshCustomInfrastPlan();
            NotifyOfPropertyChange(nameof(CustomInfrastPlanIndex));
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
        set
        {
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
        var dialog = new OpenFileDialog
        {
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
        set
        {
            SetTaskConfig<InfrastTask>(t => t.Filename == value, t => t.Filename = value);
            RefreshCustomInfrastPlan();

            // SetAndNotify 在值没有变化时不会触发 PropertyChanged 事件，所以这里手动触发一下
            NeedAddCustomInfrastPlanInfo = false;
            NotifyOfPropertyChange(nameof(CustomInfrastPlanIndex));
            NeedAddCustomInfrastPlanInfo = true;
        }
    }

    public List<CustomInfrastConfig.Plan> CustomInfrastPlanList
    {
        get => GetTaskConfig<InfrastTask>().InfrastPlan;
        set => SetTaskConfig<InfrastTask>(t => t.InfrastPlan == value, t => t.InfrastPlan = value);
    }

    public bool NeedAddCustomInfrastPlanInfo { get; set; } = true;

    public int CustomInfrastPlanIndex
    {
        get
        {
            var task = GetTaskConfig<InfrastTask>();
            if (task.InfrastPlan.Count == 0)
            {
                return 0;
            }

            var value = task.PlanIndex;
            if (value < 0 || value >= task.InfrastPlan.Count)
            {
                CustomInfrastPlanIndex = value;
            }

            return task.PlanIndex;
        }

        set
        {
            var task = GetTaskConfig<InfrastTask>();
            if (task.InfrastPlan.Count == 0)
            {
                return;
            }

            if (value < 0 || value >= task.InfrastPlan.Count)
            {
                var count = task.InfrastPlan.Count;
                value = ((value % count) + count) % count;
                _logger.Warning("CustomInfrastPlanIndex out of range, reset to Index % Count: {Value}", value);
            }

            var index = task.PlanIndex;
            if (value != index && NeedAddCustomInfrastPlanInfo)
            {
                var plan = task.InfrastPlan[value];
                Instances.TaskQueueViewModel.AddLog(plan.Name!, UiLogColor.Message);

                foreach (var period in plan.Period)
                {
                    Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:ss} - {period[1]:HH:ss} ]");
                }

                if (!string.IsNullOrEmpty(plan.Description))
                {
                    Instances.TaskQueueViewModel.AddLog(plan.Description);
                }
            }

            SetTaskConfig<InfrastTask>(t => t.PlanIndex == value, t => t.PlanIndex = value);
        }
    }

    public struct CustomInfrastPlanInfo
    {
        // ReSharper disable InconsistentNaming
        public int Index;

        public string Name;
        public string Description;
        public string DescriptionPost;

        // 有效时间段
        public struct Period
        {
            public int BeginHour;
            public int BeginMinute;
            public int EndHour;
            public int EndMinute;
        }

        public List<Period> PeriodList;

        // ReSharper restore InconsistentNaming
    }

    private bool _customInfrastInfoOutput;

    public void RefreshCustomInfrastPlan()
    {
        if (InfrastMode != Mode.Custom || !File.Exists(CustomInfrastFile))
        {
            return;
        }

        try
        {
            string jsonStr = File.ReadAllText(CustomInfrastFile);
            if (JsonConvert.DeserializeObject<CustomInfrastConfig>(jsonStr) is not CustomInfrastConfig root)
            {
                throw new JsonException("DeserializeObject returned null");
            }
            else if (_customInfrastInfoOutput)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message);
                Instances.TaskQueueViewModel.AddLog($"title: {root.Title}", UiLogColor.Info);
                Instances.TaskQueueViewModel.AddLog($"description: {root.Description}", UiLogColor.Info);
            }

            var planList = root.Plans;
            var list = new List<CustomInfrastConfig.Plan>();
            for (int i = 0; i < planList.Count; ++i)
            {
                var plan = planList[i];
                plan.Index = i;
                plan.Name ??= "Plan " + ((char)('A' + i));
                plan.Description ??= string.Empty;
                plan.DescriptionPost ??= string.Empty;
                list.Add(plan);


                if (_customInfrastInfoOutput)
                {
                    Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);
                    foreach (var period in plan.Period)
                    {
                        Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:ss} - {period[1]:HH:ss} ]");
                    }

                    if (string.IsNullOrEmpty(plan.Description))
                    {
                        Instances.TaskQueueViewModel.AddLog(plan.Description);
                    }

                    if (string.IsNullOrEmpty(plan.DescriptionPost))
                    {
                        Instances.TaskQueueViewModel.AddLog(plan.DescriptionPost);
                    }
                }
            }

            CustomInfrastPlanList = [.. list];
            _customInfrastInfoOutput = true;
        }
        catch (Exception)
        {
            _customInfrastInfoOutput = true;
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastFileParseFailed"), UiLogColor.Error);
            return;
        }

        RefreshCustomInfrastPlanIndexByPeriod();
    }

    public void RefreshCustomInfrastPlanIndexByPeriod()
    {
        // TODO 更换为全列表遍历
        if (InfrastMode != Mode.Custom || !CustomInfrastPlanList.Any(p => p.Period.Count > 0))
        {
            return;
        }

        if (!_runningState.Idle)
        {
            return;
        }

        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        var currentPlan = CustomInfrastPlanList.First(p => p.Index == CustomInfrastPlanIndex);
        if (currentPlan.Period.Any(p => p[0] <= now && now <= p[1]))
        {// 当前 index 仍在有效时间内，不需要切换
            return;
        }

        foreach (var plan in CustomInfrastPlanList.Where(plan => plan.Period.Any(p => p[0] <= now && now <= p[1])))
        {
            CustomInfrastPlanIndex = plan.Index;
            return;
        }
    }

    public static void IncreaseCustomInfrastPlanIndex(InfrastTask? task)
    {
        if (task is null || task.Mode != Mode.Custom || !task.InfrastPlan.Any(p => p.Period.Count > 0))
        {
            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
        var prePlanPostDesc = task.InfrastPlan[task.PlanIndex].DescriptionPost;
        if (!string.IsNullOrEmpty(prePlanPostDesc))
        {
            Instances.TaskQueueViewModel.AddLog(prePlanPostDesc);
        }

        ++task.PlanIndex;
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is InfrastTask)
        {
            RefreshInfrastRoomList();
            RefreshCustomInfrastPlan();
            Refresh();
        }
    }

    [Obsolete]
    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        return new AsstInfrastTask
        {
            Mode = InfrastMode,
            Facilitys = GetInfrastOrderList(),
            UsesOfDrones = UsesOfDrones,
            ContinueTraining = ContinueTraining,
            DormThreshold = DormThreshold / 100.0,
            DormFilterNotStationedEnabled = DormFilterNotStationedEnabled,
            DormTrustEnabled = DormTrustEnabled,
            OriginiumShardAutoReplenishment = OriginiumShardAutoReplenishment,
            ReceptionMessageBoard = ReceptionMessageBoardReceive,
            ReceptionClueExchange = ReceptionClueExchange,
            Filename = CustomInfrastFile,
            PlanIndex = CustomInfrastPlanIndex,
        }.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not InfrastTask infrast)
        {
            return null;
        }

        var task = new AsstInfrastTask
        {
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
            Filename = infrast.Filename,
            PlanIndex = infrast.PlanIndex,
        };
        return taskId switch
        {
            int id => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
            _ => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Infrast, task),
        };
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
