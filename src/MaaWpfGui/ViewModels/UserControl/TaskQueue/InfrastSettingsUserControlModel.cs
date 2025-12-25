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
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
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
        var roomTypes = Enum.GetNames<InfrastRoomType>();
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

    private int _dormThreshold = ConfigurationHelper.GetValue(ConfigurationKeys.DormThreshold, 30);

    /// <summary>
    /// Gets or sets the threshold to enter dormitory.
    /// </summary>
    public int DormThreshold
    {
        get => _dormThreshold;
        set {
            SetAndNotify(ref _dormThreshold, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.DormThreshold, value.ToString());
        }
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

    private Mode _infrastMode = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastMode, Mode.Normal);

    /// <summary>
    /// Gets or sets the infrast mode.
    /// </summary>
    public Mode InfrastMode
    {
        get => _infrastMode;
        set {
            if (!SetAndNotify(ref _infrastMode, value))
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastMode, value.ToString());
            ParseCustomInfrastPlan();
        }
    }

    private string _usesOfDrones = ConfigurationHelper.GetValue(ConfigurationKeys.UsesOfDrones, "Money");

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string UsesOfDrones
    {
        get => _usesOfDrones;
        set {
            SetAndNotify(ref _usesOfDrones, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.UsesOfDrones, value);
        }
    }

    private bool _receptionMessageBoardReceive = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive, true);

    public bool ReceptionMessageBoardReceive
    {
        get => _receptionMessageBoardReceive;
        set {
            SetAndNotify(ref _receptionMessageBoardReceive, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive, value.ToString());
        }
    }

    private bool _receptionClueExchange = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionClueExchange, true);

    public bool ReceptionClueExchange
    {
        get => _receptionClueExchange;
        set {
            SetAndNotify(ref _receptionClueExchange, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastReceptionClueExchange, value.ToString());
        }
    }

    private bool _continueTraining = ConfigurationHelper.GetValue(ConfigurationKeys.ContinueTraining, false);

    /// <summary>
    /// Gets or sets a value indicating whether to continue training after current training completed.
    /// </summary>
    public bool ContinueTraining
    {
        get => _continueTraining;
        set {
            SetAndNotify(ref _continueTraining, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ContinueTraining, value.ToString());
        }
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

    private bool _dormFilterNotStationedEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.DormFilterNotStationedEnabled, true);

    /// <summary>
    /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
    /// </summary>
    public bool DormFilterNotStationedEnabled
    {
        get => _dormFilterNotStationedEnabled;
        set {
            SetAndNotify(ref _dormFilterNotStationedEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.DormFilterNotStationedEnabled, value.ToString());
        }
    }

    private bool _dormTrustEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.DormTrustEnabled, false);

    /// <summary>
    /// Gets or sets a value indicating whether trust in dorm is enabled.
    /// </summary>
    public bool DormTrustEnabled
    {
        get => _dormTrustEnabled;
        set {
            SetAndNotify(ref _dormTrustEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.DormTrustEnabled, value.ToString());
        }
    }

    private bool _originiumShardAutoReplenishment = ConfigurationHelper.GetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, true);

    /// <summary>
    /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
    /// </summary>
    public bool OriginiumShardAutoReplenishment
    {
        get => _originiumShardAutoReplenishment;
        set {
            SetAndNotify(ref _originiumShardAutoReplenishment, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, value.ToString());
        }
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

    private string _customInfrastFile = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);

    public string CustomInfrastFile
    {
        get => _customInfrastFile;
        set {
            SetAndNotify(ref _customInfrastFile, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastFile, value);
            ParseCustomInfrastPlan(true);
        }
    }

    private static int CustomPlanSelectInit()
    {
        if (ConfigurationHelper.ContainsKey(ConfigurationKeys.CustomInfrastPlanSelect))
        {
            return ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanSelect, -1);
        }
        else if (ConfigurationHelper.ContainsKey(ConfigurationKeys.CustomInfrastPlanIndex))
        {
            int i;
            if (!ConfigurationHelper.DeleteValue(ConfigurationKeys.CustomInfrastPlanIndex, out string index))
            {
                return -1;
            }
            else if (!int.TryParse(index, out i))
            {
                return -1;
            }

            var path = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);
            string jsonStr = File.ReadAllText(path);
            if (JsonConvert.DeserializeObject<CustomInfrastConfig>(jsonStr) is not CustomInfrastConfig root)
            {
                return -1;
            }

            return root.Plans.Any(i => i.Period.Count > 0) ? -1 : i;
        }

        return -1;
    }

    private int _customPlanSelect = CustomPlanSelectInit();

    /// <summary>
    /// Gets or sets 手动指定的自定义配置, -1: 时间轮换, 0~n: index轮换
    /// </summary>
    public int CustomInfrastPlanSelect
    {
        get => _customPlanSelect;
        set {
            if (value < -1)
            {
                value = -1;
            }
            else if (value >= CustomInfrastPlanList.Count)
            {
                value = 0;
            }

            SetAndNotify(ref _customPlanSelect, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanSelect, value.ToString());
        }
    }

    private readonly GenericCombinedData<int> _defaultItem = new() { Display = LocalizationHelper.GetStringFormat("CustomInfrastTimeSchedule", string.Empty), Value = -1 };

    private List<CustomInfrastConfig.Plan> _customInfrastPlanList = [];

    public List<CustomInfrastConfig.Plan> CustomInfrastPlanList
    {
        get => _customInfrastPlanList;
        set {
            SetAndNotify(ref _customInfrastPlanList, value);
            _customInfrastPlanListDisplay.Clear();
            if (CustomInfrastPlanList.Any(i => i.Period.Count > 0))
            {
                var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
                var plan = CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1]));
                plan ??= CustomInfrastPlanList.FirstOrDefault();
                plan ??= new();
                _defaultItem.Display = LocalizationHelper.GetStringFormat("CustomInfrastTimeSchedule", plan.Name ?? string.Empty);
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
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastTitle"), UiLogColor.Message);
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

            CustomInfrastPlanList = [.. list];
        }
        catch (Exception)
        {
            CustomInfrastPlanList = [];
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastFileParseFailed"), UiLogColor.Error);
        }
        finally
        {
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
    /// 刷新自定义基建计划第一个时间轮换项显示, 每分钟调用一次
    /// </summary>
    public void RefreshCustomInfrastPlanDisplay()
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
    public void IncreaseCustomInfrastPlanIndex()
    {
        if (InfrastMode != Mode.Custom || CustomInfrastPlanSelect == -1 || CustomInfrastPlanSelect >= CustomInfrastPlanList.Count)
        {
            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
        Instances.TaskQueueViewModel.AddLog(CustomInfrastPlanList[CustomInfrastPlanSelect].DescriptionPost);

        ++CustomInfrastPlanSelect;
        OutputCurrentCustomPlanInfo();
    }

    private void OutputCurrentCustomPlanInfo()
    {
        if (InfrastMode != Mode.Custom || CustomInfrastPlanSelect >= CustomInfrastPlanList.Count)
        {
            return;
        }

        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        CustomInfrastConfig.Plan? plan;
        if (CustomInfrastPlanSelect == -1)
        {
            plan = CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1]));
            plan ??= CustomInfrastPlanList.First();
        }
        else
        {
            plan = CustomInfrastPlanList[CustomInfrastPlanSelect];
        }

        Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);
        foreach (var period in plan.Period)
        {
            Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:mm} - {period[1]:HH:mm} ]");
        }

        Instances.TaskQueueViewModel.AddLog(plan.Description);
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstInfrastTask {
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
        };

        if (InfrastMode != Mode.Custom)
        {
        }
        else if (CustomInfrastPlanSelect >= 0)
        {
            task.PlanIndex = CustomInfrastPlanSelect;
        }
        else
        {
            var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
            if (CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1])) is { } plan)
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

        return task.Serialize();
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
