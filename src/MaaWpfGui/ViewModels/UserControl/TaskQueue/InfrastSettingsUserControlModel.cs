// <copyright file="InfrastSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 基建任务
/// </summary>
public class InfrastSettingsUserControlModel : TaskViewModel
{
    static InfrastSettingsUserControlModel()
    {
        Instance = new();
    }

    public static InfrastSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<InfrastSettingsUserControlModel>();

    /// <summary>
    /// Gets the visibility of task setting views.
    /// </summary>
    public static TaskSettingVisibilityInfo TaskSettingVisibilities => TaskSettingVisibilityInfo.Current;

    public void InitInfrast()
    {
        var facilityList = new[]
        {
                "Mfg",
                "Trade",
                "Control",
                "Power",
                "Reception",
                "Office",
                "Dorm",
                "Processing",
                "Training",
        };

        var tempOrderList = new List<DragItemViewModel?>(new DragItemViewModel[facilityList.Length]);
        var nonOrderList = new List<DragItemViewModel>();
        for (int i = 0; i != facilityList.Length; ++i)
        {
            var facility = facilityList[i];
            bool parsed = int.TryParse(ConfigurationHelper.GetFacilityOrder(facility, "-1"), out int order);

            DragItemViewModel vm = new DragItemViewModel(
                LocalizationHelper.GetString(facility),
                facility,
                "Infrast.");

            if (!parsed || order < 0 || order >= tempOrderList.Count || tempOrderList[order] != null)
            {
                nonOrderList.Add(vm);
            }
            else
            {
                tempOrderList[order] = vm;
            }
        }

        foreach (var newVm in nonOrderList)
        {
            int i = 0;
            while (i < tempOrderList.Count && tempOrderList[i] != null)
            {
                ++i;
            }

            tempOrderList[i] = newVm;
            ConfigurationHelper.SetFacilityOrder(newVm.OriginalName, i.ToString());
        }

        InfrastItemViewModels = new ObservableCollection<DragItemViewModel>(tempOrderList!);
        InfrastItemViewModels.CollectionChanged += InfrastOrderSelectionChanged;

        _dormThresholdLabel = LocalizationHelper.GetString("DormThreshold") + ": " + _dormThreshold + "%";
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
            DormThresholdLabel = LocalizationHelper.GetString("DormThreshold") + ": " + _dormThreshold + "%";
            ConfigurationHelper.SetValue(ConfigurationKeys.DormThreshold, value.ToString());
        }
    }

    private string _dormThresholdLabel = string.Empty;

    /// <summary>
    /// Gets or sets the label of dormitory threshold.
    /// </summary>
    public string DormThresholdLabel
    {
        get => _dormThresholdLabel;
        set => SetAndNotify(ref _dormThresholdLabel, value);
    }

    /// <summary>
    /// Gets infrast order list.
    /// </summary>
    /// <returns>The infrast order list.</returns>
    public List<string> GetInfrastOrderList()
    {
        return (from item in InfrastItemViewModels where item.IsChecked select item.OriginalName).ToList();
    }

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
    public void InfrastItemSelectedAll()
    {
        foreach (var item in InfrastItemViewModels)
        {
            item.IsChecked = true;
        }
    }

    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
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
            ConfigurationHelper.SetFacilityOrder(item.OriginalName, index.ToString());
            ++index;
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

    private bool _prioritizeSendingClue = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.PrioritizeSendingClue, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to send clue before filling own clue inventory.
    /// </summary>
    public bool PrioritizeSendingClue
    {
        get => _prioritizeSendingClue;
        set
        {
            SetAndNotify(ref _prioritizeSendingClue, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.PrioritizeSendingClue, value.ToString());
        }
    }

    private bool _sendClueToOCR = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.SendClueToOCR, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to send clue to specfic OCR target.
    /// </summary>
    public bool SendClueToOCR
    {
        get => _sendClueToOCR;
        set
        {
            SetAndNotify(ref _sendClueToOCR, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.SendClueToOCR, value.ToString());
        }
    }

    private bool _onlySendClueToOCR = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.OnlySendClueToOCR, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to send clue to specfic OCR target.
    /// </summary>
    public bool OnlySendClueToOCR
    {
        get => _onlySendClueToOCR;
        set
        {
            SetAndNotify(ref _onlySendClueToOCR, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.OnlySendClueToOCR, value.ToString());
        }
    }

    private string _sendClueList = ConfigurationHelper.GetValue(ConfigurationKeys.SendClueList, LocalizationHelper.GetString("SendClueListDefault"));

    /// <summary>
    /// Gets or sets a list of OCR target to send clue to.
    /// </summary>
    public string SendClueList
    {
        get => _sendClueList;
        set
        {
            SetAndNotify(ref _sendClueList, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.SendClueList, value);
        }
    }

    private string _defaultInfrast = ConfigurationHelper.GetValue(ConfigurationKeys.DefaultInfrast, UserDefined);

    private const string UserDefined = "user_defined";

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
                CustomInfrastFile = @"resource\custom_infrast\" + value;
                IsCustomInfrastFileReadOnly = true;
            }
            else
            {
                IsCustomInfrastFileReadOnly = false;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.DefaultInfrast, value);
        }
    }

    private bool _isCustomInfrastFileReadOnly = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether  CustomInfrastFile is read-only
    /// </summary>
    public bool IsCustomInfrastFileReadOnly
    {
        get => _isCustomInfrastFileReadOnly;
        set
        {
            SetAndNotify(ref _isCustomInfrastFileReadOnly, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.IsCustomInfrastFileReadOnly, value.ToString());
        }
    }

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

    private bool _customInfrastEnabled = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastEnabled, bool.FalseString));

    public bool CustomInfrastEnabled
    {
        get => _customInfrastEnabled;
        set
        {
            SetAndNotify(ref _customInfrastEnabled, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastEnabled, value.ToString());
            RefreshCustomInfrastPlan();
        }
    }

    /// <summary>
    /// Selects infrast config file.
    /// </summary>
    // UI 绑定的方法
    // ReSharper disable once UnusedMember.Global
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
            RefreshCustomInfrastPlan();

            // SetAndNotify 在值没有变化时不会触发 PropertyChanged 事件，所以这里手动触发一下
            NeedAddCustomInfrastPlanInfo = false;
            NotifyOfPropertyChange(nameof(CustomInfrastPlanIndex));
            NeedAddCustomInfrastPlanInfo = true;
        }
    }

    public bool NeedAddCustomInfrastPlanInfo { get; set; } = true;

    private int _customInfrastPlanIndex = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanIndex, "0"));

    public int CustomInfrastPlanIndex
    {
        get
        {
            if (CustomInfrastPlanInfoList.Count == 0)
            {
                return 0;
            }

            if (_customInfrastPlanIndex >= CustomInfrastPlanInfoList.Count || _customInfrastPlanIndex < 0)
            {
                CustomInfrastPlanIndex = _customInfrastPlanIndex;
            }

            return _customInfrastPlanIndex;
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
                _logger.Warning($"CustomInfrastPlanIndex out of range, reset to Index % Count: {value}");
            }

            if (value != _customInfrastPlanIndex && NeedAddCustomInfrastPlanInfo)
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

            SetAndNotify(ref _customInfrastPlanIndex, value);
            Instances.TaskQueueViewModel.SetInfrastParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanIndex, value.ToString());
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

        if (!CustomInfrastEnabled)
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
        if (!CustomInfrastEnabled || !_customInfrastPlanHasPeriod || Instances.TaskQueueViewModel.InfrastTaskRunning)
        {
            return;
        }

        var now = DateTime.Now;
        foreach (var plan in CustomInfrastPlanInfoList.Where(
                     plan => plan.PeriodList.Any(
                         period => TimeLess(period.BeginHour, period.BeginMinute, now.Hour, now.Minute)
                                   && TimeLess(now.Hour, now.Minute, period.EndHour, period.EndMinute))))
        {
            CustomInfrastPlanIndex = plan.Index;
            return;
        }

        if (CustomInfrastPlanIndex >= CustomInfrastPlanList.Count || CustomInfrastPlanList.Count < 0)
        {
            CustomInfrastPlanIndex = 0;
        }

        return;

        static bool TimeLess(int lHour, int lMin, int rHour, int rMin) => (lHour != rHour) ? (lHour < rHour) : (lMin <= rMin);
    }

    public void IncreaseCustomInfrastPlanIndex()
    {
        if (!CustomInfrastEnabled || _customInfrastPlanHasPeriod || CustomInfrastPlanInfoList.Count == 0)
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

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        return new AsstInfrastTask
        {
            Facilitys = GetInfrastOrderList(),
            UsesOfDrones = UsesOfDrones,
            ContinueTraining = ContinueTraining,
            DormThreshold = DormThreshold / 100.0,
            DormFilterNotStationedEnabled = DormFilterNotStationedEnabled,
            DormDormTrustEnabled = DormTrustEnabled,
            OriginiumShardAutoReplenishment = OriginiumShardAutoReplenishment,
            IsCustom = CustomInfrastEnabled,
            ReceptionMessageBoard = ReceptionMessageBoardReceive,
            Filename = CustomInfrastFile,
            PlanIndex = CustomInfrastPlanIndex,
        }.Serialize();
    }
}
