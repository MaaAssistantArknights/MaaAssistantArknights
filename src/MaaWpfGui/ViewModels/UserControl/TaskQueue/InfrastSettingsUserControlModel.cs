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

    private int _dormThreshold = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.DormThreshold, "30"));

    /// <summary>
    /// Gets or sets the threshold to enter dormitory.
    /// </summary>
    public int DormThreshold
    {
        get => _dormThreshold;
        set
        {
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

    // 5.16.0-b1，后续版本直接使用 ConfigurationHelper.GetValue(ConfigurationKeys.InfrastMode, Mode.Normal);
    private Mode _infrastMode = GetInfrastMode();

    private static Mode GetInfrastMode()
    {
        if (ConfigurationHelper.ContainsKey(ConfigurationKeys.CustomInfrastEnabled) &&
            ConfigurationHelper.DeleteValue(ConfigurationKeys.CustomInfrastEnabled, out string outStr) &&
            bool.TryParse(outStr, out bool enable) && enable)
        {
            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastMode, Mode.Custom.ToString());
            return Mode.Custom;
        }

        return ConfigurationHelper.GetValue(ConfigurationKeys.InfrastMode, Mode.Normal);
    }

    /// <summary>
    /// Gets or sets the infrast mode.
    /// </summary>
    public Mode InfrastMode
    {
        get => _infrastMode;
        set
        {
            if (!SetAndNotify(ref _infrastMode, value))
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastMode, value.ToString());
            ParseCustomInfrastPlan();
            NotifyOfPropertyChange(nameof(CustomInfrastPlanIndex));
        }
    }

    private string _usesOfDrones = ConfigurationHelper.GetValue(ConfigurationKeys.UsesOfDrones, "Money");

    /// <summary>
    /// Gets or sets the uses of drones.
    /// </summary>
    public string UsesOfDrones
    {
        get => _usesOfDrones;
        set
        {
            SetAndNotify(ref _usesOfDrones, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.UsesOfDrones, value);
        }
    }

    private bool _receptionMessageBoardReceive = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive, bool.TrueString));

    public bool ReceptionMessageBoardReceive
    {
        get => _receptionMessageBoardReceive;
        set
        {
            SetAndNotify(ref _receptionMessageBoardReceive, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive, value.ToString());
        }
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

    private bool _continueTraining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ContinueTraining, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to continue training after current training completed.
    /// </summary>
    public bool ContinueTraining
    {
        get => _continueTraining;
        set
        {
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

    private bool _dormFilterNotStationedEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DormFilterNotStationedEnabled, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether the not stationed filter in dorm is enabled.
    /// </summary>
    public bool DormFilterNotStationedEnabled
    {
        get => _dormFilterNotStationedEnabled;
        set
        {
            SetAndNotify(ref _dormFilterNotStationedEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.DormFilterNotStationedEnabled, value.ToString());
        }
    }

    private bool _dormTrustEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DormTrustEnabled, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether trust in dorm is enabled.
    /// </summary>
    public bool DormTrustEnabled
    {
        get => _dormTrustEnabled;
        set
        {
            SetAndNotify(ref _dormTrustEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.DormTrustEnabled, value.ToString());
        }
    }

    private bool _originiumShardAutoReplenishment = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether Originium shard auto replenishment is enabled.
    /// </summary>
    public bool OriginiumShardAutoReplenishment
    {
        get => _originiumShardAutoReplenishment;
        set
        {
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

    private string _customInfrastFile = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);

    public string CustomInfrastFile
    {
        get => _customInfrastFile;
        set
        {
            SetAndNotify(ref _customInfrastFile, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastFile, value);
            ParseCustomInfrastPlan();
        }
    }

    public bool NeedAddCustomInfrastPlanInfo { get; set; } = true;

    private int _customInfrastPlanIndex = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanIndex, 0);

    public int CustomInfrastPlanIndex
    {
        get
        {
            if (CustomInfrastPlanList.Count == 0)
            {
                return 0;
            }

            if (_customInfrastPlanIndex < 0 || _customInfrastPlanIndex >= CustomInfrastPlanList.Count)
            {
                CustomInfrastPlanIndex = _customInfrastPlanIndex;
            }

            return _customInfrastPlanIndex;
        }

        set
        {
            if (CustomInfrastPlanList.Count == 0)
            {
                return;
            }

            if (value < 0 || value >= CustomInfrastPlanList.Count)
            {
                var count = CustomInfrastPlanList.Count;
                value = ((value % count) + count) % count;
                _logger.Warning("CustomInfrastPlanIndex out of range, reset to Index % Count: {Value}", value);
            }

            SetAndNotify(ref _customInfrastPlanIndex, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanIndex, value.ToString());
        }
    }

    private int _customPlanSelect = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanSelect, -1);

    /// <summary>
    /// Gets or sets 手动指定的自定义配置, -1: 自动轮换, 0~n: 选中编号
    /// </summary>
    public int CustomInfrastPlanSelect
    {
        get => _customPlanSelect;
        set
        {
            if (value < -1 || value >= CustomInfrastPlanList.Count)
            {
                value = -1;
            }

            SetAndNotify(ref _customPlanSelect, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanSelect, value.ToString());
        }
    }

    private CustomInfrastConfig.Plan _defaultItem = new() { Name = "自动轮换", Index = -1 };

    private List<CustomInfrastConfig.Plan> _customInfrastPlanList = [];

    public List<CustomInfrastConfig.Plan> CustomInfrastPlanList
    {
        get => _customInfrastPlanList;
        set => SetAndNotify(ref _customInfrastPlanList, value);
    }

    private readonly ObservableCollection<CustomInfrastConfig.Plan> _customInfrastPlanListDisplay = [];

    [PropertyDependsOn(nameof(CustomInfrastPlanList), nameof(CustomInfrastPlanIndex))]
    public ObservableCollection<CustomInfrastConfig.Plan> CustomPlanListDisplay
    {
        get
        {
            var task = GetTaskConfig<InfrastTask>();
            _customInfrastPlanListDisplay.Clear();
            var plan = CustomInfrastPlanList.FirstOrDefault(i => i.Index == CustomInfrastPlanIndex);
            plan ??= CustomInfrastPlanList.FirstOrDefault();
            plan ??= new();
            _defaultItem.Name = $"自动切换({plan.Name})";
            _customInfrastPlanListDisplay.Add(_defaultItem);
            foreach (var item in CustomInfrastPlanList)
            {
                _customInfrastPlanListDisplay.Add(item);
            }

            if (CustomInfrastPlanSelect < -1 || CustomInfrastPlanSelect >= _customInfrastPlanListDisplay.Count)
            {
                CustomInfrastPlanSelect = -1;
            }
            else
            {
                NotifyOfPropertyChange(nameof(CustomInfrastPlanSelect));
            }

            return _customInfrastPlanListDisplay;
        }
    }

    private bool _customInfrastPlanHasPeriod;
    private bool _customInfrastInfoOutput;

    public void ParseCustomInfrastPlan()
    {
        if (InfrastMode != Mode.Custom || !File.Exists(CustomInfrastFile))
        {
            CustomInfrastPlanList = [];
            _customInfrastPlanHasPeriod = false;
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
            return;
        }
        finally
        {
            _customInfrastInfoOutput = true;
            _customInfrastPlanHasPeriod = CustomInfrastPlanList.Any(i => i.Period.Count > 0);
            if (CustomInfrastPlanIndex < 0 || CustomInfrastPlanIndex >= CustomInfrastPlanList.Count)
            {
                CustomInfrastPlanIndex = 0;
            }
            else
            {
                NotifyOfPropertyChange(nameof(CustomInfrastPlanIndex));
            }
        }

        RefreshCustomInfrastPlanIndexByPeriod();
    }

    public void RefreshCustomInfrastPlanIndexByPeriod()
    {
        if (InfrastMode != Mode.Custom || !_customInfrastPlanHasPeriod || CustomInfrastPlanList.Count == 0)
        {
            return;
        }

        if (!_runningState.GetIdle() &&
             Instances.AsstProxy.TasksStatus.FirstOrDefault(i => i.Value.Type == AsstProxy.TaskType.Infrast).Value.Status != TaskStatus.Completed)
        {
            return;
        }

        int index = CustomInfrastPlanIndex;
        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        var currentPlan = CustomInfrastPlanList.FirstOrDefault(p => p.Index == CustomInfrastPlanIndex);
        if (currentPlan?.Period.Any(p => p[0] <= now && now <= p[1]) is true)
        { // index对应任务存在 且 在生效时间内，不需要切换
        }
        else if (CustomInfrastPlanList.FirstOrDefault(i => i.Period.Any(p => p[0] <= now && now <= p[1])) is { } plan)
        { // 有plan的时间段可用
            index = plan.Index;
            Instances.TaskQueueViewModel.AddLog(plan.Name!, UiLogColor.Message);
            foreach (var period in plan.Period)
            {
                Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:mm} - {period[1]:HH:mm} ]");
            }

            Instances.TaskQueueViewModel.AddLog(plan.Description);
        }
        else if (currentPlan is null)
        { // index对应计划不存在
            index = 0;
        }
        else
        { // index对应计划存在，且不在生效时间内，且没有任何plan的时间段可用。看看要不要改，目前未校验period是否覆盖全天
            // Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanNoValidPeriod"), UiLogColor.Warning);
            index = 0;
        }

        if (index != CustomInfrastPlanIndex)
        {
            CustomInfrastPlanIndex = index;
            OutputCustomPlanInfo();
        }
    }

    public void IncreaseCustomInfrastPlanIndex()
    {
        if (InfrastMode != Mode.Custom || _customInfrastPlanHasPeriod || CustomInfrastPlanList.Count == 0)
        {
            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CustomInfrastPlanIndexAutoSwitch"), UiLogColor.Message);
        Instances.TaskQueueViewModel.AddLog(CustomInfrastPlanList[CustomInfrastPlanIndex].DescriptionPost);

        ++CustomInfrastPlanIndex;
        OutputCustomPlanInfo();
    }

    private void OutputCustomPlanInfo()
    {
        var plan = CustomInfrastPlanList[CustomInfrastPlanIndex];
        Instances.TaskQueueViewModel.AddLog(plan.Name, UiLogColor.Message);
        foreach (var period in plan.Period)
        {
            Instances.TaskQueueViewModel.AddLog($"[ {period[0]:HH:mm} - {period[1]:HH:mm} ]");
        }

        Instances.TaskQueueViewModel.AddLog(plan.Description);
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
