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
using MaaWpfGui.Main;
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
public class InfrastSettingsUserControlModel : TaskViewModel
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

    public void InitInfrast()
    {
        var roomTypes = Enum.GetNames(typeof(InfrastRoomType));
        var list = new List<KeyValuePair<string, int>>();
        var roomList = new List<DragItemViewModel>(roomTypes.Length);
        foreach (var item in roomTypes)
        {
            var index = ConfigurationHelper.GetValue("Infrast.Order." + item, -1);
            list.Add(new KeyValuePair<string, int>(item, index));
        }

        list.Sort((x, y) => x.Value.CompareTo(y.Value));
        for (int i = 0; i < list.Count; ++i)
        {
            var item = list[i];
            if (item.Value != i)
            {
                ConfigurationHelper.SetValue("Infrast.Order." + item.Key, i.ToString());
            }

            roomList.Add(new DragItemViewModel(LocalizationHelper.GetString(item.Key), item.Key, "Infrast."));
        }

        InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(roomList);
        InfrastItemViewModels.CollectionChanged += InfrastOrderSelectionChanged;
    }

    /// <summary>
    /// Gets or sets the infrast item view models.
    /// </summary>
    public ObservableCollection<DragItemViewModel> InfrastItemViewModels { get; set; } = [];

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
        get => GetTaskConfig<InfrastTask>()?.DormThreshold ?? 30;
        set => SetTaskConfig<InfrastTask>(t => t.DormThreshold == value, t => t.DormThreshold = value);
    }

    /// <summary>
    /// Gets infrast order list.
    /// </summary>
    /// <returns>The infrast order list.</returns>
    public List<string> GetInfrastOrderList()
    {
        return [.. InfrastItemViewModels.Where(i => i.IsChecked).Select(i => i.OriginalName)];
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void InfrastItemSelectedAll()
    {
        foreach (var item in InfrastItemViewModels)
        {
            item.IsChecked = true;
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void InfrastItemUnselectedAll()
    {
        foreach (var item in InfrastItemViewModels)
        {
            item.IsChecked = false;
        }
    }

    /// <summary>
    /// 实时更新基建换班顺序
    /// </summary>
    /// <param name="sender">ignored object</param>
    /// <param name="e">ignored NotifyCollectionChangedEventArgs</param>
    public void InfrastOrderSelectionChanged(object? sender = null, NotifyCollectionChangedEventArgs? e = null)
    {
        _ = (sender, e);
        int index = 0;
        foreach (var item in InfrastItemViewModels)
        {
            ConfigurationHelper.SetValue("Infrast.Order." + item.OriginalName, index.ToString());
            ++index;
        }
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
        get => GetTaskConfig<InfrastTask>()?.Mode ?? default;
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
        get => GetTaskConfig<InfrastTask>()?.UsesOfDrones ?? string.Empty;
        set => SetTaskConfig<InfrastTask>(t => t.UsesOfDrones == value, t => t.UsesOfDrones = value);
    }

    public bool ReceptionMessageBoardReceive
    {
        get => GetTaskConfig<InfrastTask>()?.ReceptionMessageBoard ?? true;
        set => SetTaskConfig<InfrastTask>(t => t.ReceptionMessageBoard == value, t => t.ReceptionMessageBoard = value);
    }

    private bool _receptionClueExchange = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionClueExchange, bool.TrueString));

    public bool ReceptionClueExchange
    {
        get => _receptionClueExchange;
        set
        {
            SetAndNotify(ref _receptionClueExchange, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastReceptionClueExchange, value.ToString());
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to continue training after current training completed.
    /// </summary>
    public bool ContinueTraining
    {
        get => GetTaskConfig<InfrastTask>()?.ContinueTraining ?? true;
        set => SetTaskConfig<InfrastTask>(t => t.ReceptionMessageBoard == value, t => t.ReceptionMessageBoard = value);
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
        get => GetTaskConfig<InfrastTask>()?.DormFilterNotStationed ?? true;
        set => SetTaskConfig<InfrastTask>(t => t.DormFilterNotStationed == value, t => t.DormFilterNotStationed = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether trust in dorm is enabled.
    /// </summary>
    public bool DormTrustEnabled
    {
        get => GetTaskConfig<InfrastTask>()?.DormTrustEnabled ?? true;
        set => SetTaskConfig<InfrastTask>(t => t.DormTrustEnabled == value, t => t.DormTrustEnabled = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
    /// </summary>
    public bool OriginiumShardAutoReplenishment
    {
        get => GetTaskConfig<InfrastTask>()?.OriginiumShardAutoReplenishment ?? true;
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
        get => GetTaskConfig<InfrastTask>()?.Filename ?? string.Empty;
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

    public bool NeedAddCustomInfrastPlanInfo { get; set; } = true;

    public int CustomInfrastPlanIndex
    {
        get
        {
            if (CustomInfrastPlanInfoList.Count == 0)
            {
                return 0;
            }

            var value = GetTaskConfig<InfrastTask>()?.PlanIndex ?? 0;
            if (value >= CustomInfrastPlanInfoList.Count || value < 0)
            {
                CustomInfrastPlanIndex = value;
            }

            return GetTaskConfig<InfrastTask>()?.PlanIndex ?? 0;
        }

        set
        {
            if (CustomInfrastPlanInfoList.Count == 0)
            {
                return;
            }

            if (value >= CustomInfrastPlanInfoList.Count || value < 0)
            {
                var count = CustomInfrastPlanInfoList.Count;
                value = ((value % count) + count) % count;
                _logger.Warning("CustomInfrastPlanIndex out of range, reset to Index % Count: {Value}", value);
            }

            var index = GetTaskConfig<InfrastTask>()?.PlanIndex ?? 0;
            if (value != index && NeedAddCustomInfrastPlanInfo)
            {
                var plan = CustomInfrastPlanInfoList[value];
                Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);

                foreach (var period in plan.PeriodList)
                {
                    Instances.TaskQueueViewModel.AddLog($"[ {period.BeginHour:D2}:{period.BeginMinute:D2} - {period.EndHour:D2}:{period.EndMinute:D2} ]");
                }

                if (plan.Description != string.Empty)
                {
                    Instances.TaskQueueViewModel.AddLog(plan.Description);
                }
            }

            SetTaskConfig<InfrastTask>(t => t.PlanIndex == value, t => t.PlanIndex = value);
        }
    }

    public ObservableCollection<GenericCombinedData<int>> CustomInfrastPlanList { get; } = [];

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

    public List<CustomInfrastPlanInfo> CustomInfrastPlanInfoList { get; } = [];

    private bool _customInfrastPlanHasPeriod;
    private bool _customInfrastInfoOutput;

    public void RefreshCustomInfrastPlan()
    {
        CustomInfrastPlanInfoList.Clear();
        CustomInfrastPlanList.Clear();
        _customInfrastPlanHasPeriod = false;

        if (InfrastMode != Mode.Custom)
        {
            return;
        }

        if (!File.Exists(CustomInfrastFile))
        {
            return;
        }

        try
        {
            string jsonStr = File.ReadAllText(CustomInfrastFile);
            var root = (JObject?)JsonConvert.DeserializeObject(jsonStr);

            if (root != null && _customInfrastInfoOutput && root.TryGetValue("title", out var title))
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message);
                Instances.TaskQueueViewModel.AddLog($"title: {title}", UiLogColor.Info);
                if (root.TryGetValue("description", out var value))
                {
                    Instances.TaskQueueViewModel.AddLog($"description: {value}", UiLogColor.Info);
                }
            }

            var planList = (JArray?)root?["plans"];
            if (planList != null)
            {
                for (int i = 0; i < planList.Count; ++i)
                {
                    var plan = (JObject)planList[i];
                    string display = plan.TryGetValue("name", out var name) ? name.ToString() : ("Plan " + ((char)('A' + i)));
                    CustomInfrastPlanList.Add(new GenericCombinedData<int> { Display = display, Value = i });
                    string desc = plan.TryGetValue("description", out var description) ? description.ToString() : string.Empty;
                    string descPost = plan.TryGetValue("description_post", out var descriptionPost) ? descriptionPost.ToString() : string.Empty;

                    if (_customInfrastInfoOutput)
                    {
                        Instances.TaskQueueViewModel.AddLog(display, UiLogColor.Message);
                    }

                    var periodList = new List<CustomInfrastPlanInfo.Period>();
                    if (plan.TryGetValue("period", out var token))
                    {
                        var periodArray = (JArray)token;
                        foreach (var periodJson in periodArray)
                        {
                            var period = default(CustomInfrastPlanInfo.Period);
                            var beginTime = periodJson[0]?.ToString();
                            if (beginTime != null)
                            {
                                var beginSplit = beginTime.Split(':');
                                period.BeginHour = int.Parse(beginSplit[0]);
                                period.BeginMinute = int.Parse(beginSplit[1]);
                            }

                            var endTime = periodJson[1]?.ToString();
                            if (endTime != null)
                            {
                                var endSplit = endTime.Split(':');
                                period.EndHour = int.Parse(endSplit[0]);
                                period.EndMinute = int.Parse(endSplit[1]);
                            }

                            periodList.Add(period);
                            if (_customInfrastInfoOutput)
                            {
                                Instances.TaskQueueViewModel.AddLog($"[ {period.BeginHour:D2}:{period.BeginMinute:D2} - {period.EndHour:D2}:{period.EndMinute:D2} ]");
                            }
                        }

                        if (periodList.Count != 0)
                        {
                            _customInfrastPlanHasPeriod = true;
                        }
                    }

                    if (_customInfrastInfoOutput && desc != string.Empty)
                    {
                        Instances.TaskQueueViewModel.AddLog(desc);
                    }

                    if (_customInfrastInfoOutput && descPost != string.Empty)
                    {
                        Instances.TaskQueueViewModel.AddLog(descPost);
                    }

                    CustomInfrastPlanInfoList.Add(new CustomInfrastPlanInfo
                    {
                        Index = i,
                        Name = display,
                        Description = desc,
                        DescriptionPost = descPost,
                        PeriodList = periodList,
                    });
                }
            }

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
        if (InfrastMode != Mode.Custom || !_customInfrastPlanHasPeriod || CustomInfrastPlanInfoList.Count == 0)
        {
            return;
        }

        if (!_runningState.GetIdle() &&
             Instances.AsstProxy.TasksStatus.FirstOrDefault(i => i.Value.Type == AsstProxy.TaskType.Infrast).Value.Status != TaskStatus.Completed)
        {
            return;
        }

        var now = DateTime.Now;

        if (CustomInfrastPlanIndex >= CustomInfrastPlanInfoList.Count || CustomInfrastPlanIndex < 0)
        {
            CustomInfrastPlanIndex = 0;
        }

        var currentPlan = CustomInfrastPlanInfoList.First(p => p.Index == CustomInfrastPlanIndex);
        foreach (var period in currentPlan.PeriodList)
        {
            if (TimeLess(period.BeginHour, period.BeginMinute, now.Hour, now.Minute) &&
                TimeLess(now.Hour, now.Minute, period.EndHour, period.EndMinute))
            {
                return; // 当前 index 仍在有效时间内，不需要切换
            }
        }

        foreach (var plan in CustomInfrastPlanInfoList.Where(
                     plan => plan.PeriodList.Any(
                         period => TimeLess(period.BeginHour, period.BeginMinute, now.Hour, now.Minute) &&
                                   TimeLess(now.Hour, now.Minute, period.EndHour, period.EndMinute))))
        {
            CustomInfrastPlanIndex = plan.Index;
            return;
        }

        return;

        static bool TimeLess(int lHour, int lMin, int rHour, int rMin) => (lHour != rHour) ? (lHour < rHour) : (lMin <= rMin);
    }

    public void IncreaseCustomInfrastPlanIndex()
    {
        if (InfrastMode != Mode.Custom || _customInfrastPlanHasPeriod || CustomInfrastPlanInfoList.Count == 0)
        {
            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
        var prePlanPostDesc = CustomInfrastPlanInfoList[CustomInfrastPlanIndex].DescriptionPost;
        if (prePlanPostDesc != string.Empty)
        {
            Instances.TaskQueueViewModel.AddLog(prePlanPostDesc);
        }

        ++CustomInfrastPlanIndex;
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is InfrastTask)
        {
            Refresh();
        }
    }

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
            Filename = infrast.Filename,
            PlanIndex = infrast.PlanIndex,
        };
        if (taskId is { } id)
        {
            return Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task);
        }
        else
        {
            return Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Infrast, task);
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
