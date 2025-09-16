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
using System.Windows;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 刷理智
/// </summary>
public class FightSettingsUserControlModel : TaskSettingsViewModel
{
    public static FightTimes? FightReport { get; set; }

    public static SanityInfo? SanityReport { get; set; }

    static FightSettingsUserControlModel()
    {
        Instance = new();
    }

    public static FightSettingsUserControlModel Instance { get; }

    private ObservableCollection<CombinedData> _stageList = [];

    /// <summary>
    /// Gets or private sets the list of stages.
    /// </summary>
    public ObservableCollection<CombinedData> StageList
    {
        get => _stageList;
        private set => SetAndNotify(ref _stageList, value);
    }

    /// <summary>
    /// Gets the stage.
    /// </summary>
    public string Stage
    {
        get
        {
            Stage1 ??= Stage1Fallback;

            if (!UseAlternateStage)
            {
                return Stage1;
            }

            if (Instances.TaskQueueViewModel.IsStageOpen(Stage1))
            {
                return Stage1;
            }

            if (Instances.TaskQueueViewModel.IsStageOpen(Stage2 ??= string.Empty))
            {
                return Stage2;
            }

            if (Instances.TaskQueueViewModel.IsStageOpen(Stage3 ??= string.Empty))
            {
                return Stage3;
            }

            return Instances.TaskQueueViewModel.IsStageOpen(Stage4 ??= string.Empty) ? Stage4 : Stage1;
        }
    }

    private readonly Dictionary<string, string> _stageDictionary = new()
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

    public string?[] Stages => [Stage1, Stage2, Stage3, Stage4];

    // Try to fix: issues#5742. 关卡选择为 null 时的一个补丁，可能是 StageList 改变后，wpf binding 延迟更新的问题。</remarks>
    public string Stage1Fallback = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the stage1.
    /// </summary>
    public string Stage1
    {
        get => GetTaskConfig<FightTask>().Stage1;
        set
        {
            if (GetTaskConfig<FightTask>().Stage1 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                // 从后往前删
                if (GetTaskConfig<FightTask>().Stage1.Length != 3)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetTaskConfig<FightTask>(t => t.Stage1 == value, t => t.Stage1 = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    /// <summary>
    /// Gets or sets the stage2.
    /// </summary>
    public string Stage2
    {
        get => GetTaskConfig<FightTask>().Stage2;
        set
        {
            if (GetTaskConfig<FightTask>().Stage2 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (GetTaskConfig<FightTask>().Stage2.Length != 3)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetTaskConfig<FightTask>(t => t.Stage2 == value, t => t.Stage2 = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    /// <summary>
    /// Gets or sets the stage3.
    /// </summary>
    public string Stage3
    {
        get => GetTaskConfig<FightTask>().Stage3;
        set
        {
            if (GetTaskConfig<FightTask>().Stage3 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (GetTaskConfig<FightTask>().Stage3.Length != 3)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetTaskConfig<FightTask>(t => t.Stage3 == value, t => t.Stage3 = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    /// <summary>
    /// Gets or sets the stage4.
    /// </summary>
    public string Stage4
    {
        get => GetTaskConfig<FightTask>().Stage4;
        set
        {
            if (GetTaskConfig<FightTask>().Stage4 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (GetTaskConfig<FightTask>().Stage4.Length != 3)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetTaskConfig<FightTask>(t => t.Stage4 == value, t => t.Stage4 = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use custom stage code.
    /// </summary>
    public bool CustomStageCode
    {
        get => GetTaskConfig<FightTask>().IsStageManually;
        set
        {
            if (!value)
            {
                RemoveNonExistStage();
            }

            SetTaskConfig<FightTask>(t => t.IsStageManually == value, t => t.IsStageManually = value);
        }
    }

    /// <summary>
    /// 移除不在关卡列表中的关卡，关闭自定义和刷新关卡列表时调用
    /// </summary>
    public void RemoveNonExistStage()
    {
        Stage1 = StageList.FirstOrDefault(x => x.Value == Stage1)?.Value ?? string.Empty;
        Stage2 = StageList.FirstOrDefault(x => x.Value == Stage2)?.Value ?? string.Empty;
        Stage3 = StageList.FirstOrDefault(x => x.Value == Stage3)?.Value ?? string.Empty;
        Stage4 = StageList.FirstOrDefault(x => x.Value == Stage4)?.Value ?? string.Empty;
    }

    /// <summary>
    /// Reset unsaved battle parameters.
    /// </summary>
    public void ResetFightVariables()
    {
        UseStone ??= false;
        UseMedicine ??= false;
        HasTimesLimited ??= false;
        IsSpecifiedDrops ??= false;
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use medicine with null.
    /// </summary>
    public bool? UseMedicine
    {
        get => GetTaskConfig<FightTask>().UseMedicine;
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.UseMedicine == value, t => t.UseMedicine = value))
            {
                return;
            }

            if (value == false)
            {
                UseStoneDisplay = false;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    /// <summary>
    /// Gets or sets the amount of medicine used.
    /// </summary>
    public int MedicineNumber
    {
        get => GetTaskConfig<FightTask>().MedicineCount;
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.MedicineCount == value, t => t.MedicineCount = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    public static string UseStoneString => LocalizationHelper.GetString("UseOriginitePrime");

    private bool? _useStone = ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicine, false) &&
                                      ConfigurationHelper.GetValue(ConfigurationKeys.UseStone, false);

    /// <summary>
    /// Gets or sets a value indicating whether to use originiums with null.
    /// </summary>
    public bool? UseStone
    {
        get => GetTaskConfig<FightTask>().UseStone;
        set
        {
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

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
            if (GetTaskConfig<FightTask>().UseStoneAllowSave)
            {
                SetTaskConfig<FightTask>(t => t.UseStone == value, t => t.UseStone = value);
            }
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
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.StoneCount == value, t => t.StoneCount = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the number of times is limited with null.
    /// </summary>
    public bool? HasTimesLimited
    {
        get => GetTaskConfig<FightTask>().EnableTimesLimit;
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.EnableTimesLimit == value, t => t.EnableTimesLimit = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    /// <summary>
    /// Gets or sets the max number of times.
    /// </summary>
    public int MaxTimes
    {
        get => GetTaskConfig<FightTask>().TimesLimit;
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.TimesLimit == value, t => t.TimesLimit = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    public static Dictionary<string, int> SeriesList { get; } = new()
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
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.Series == value, t => t.Series = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    #region Drops

    /// <summary>
    /// Gets or sets a value indicating whether the drops are specified.
    /// </summary>
    public bool? IsSpecifiedDrops
    {
        get => GetTaskConfig<FightTask>().EnableTargetDrop;
        set
        {
            if (!SetTaskConfig<FightTask>(t => t.EnableTargetDrop == value, t => t.EnableTargetDrop = value))
            {
                return;
            }

            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
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
    ];

    public void InitDrops()
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
        if (AllDrops.FirstOrDefault(i => i.Value == DropsItemId) is { } item)
        {
            DropsItemName = item.Display;
            NotifyOfPropertyChange(nameof(DropsItemName));
        }
        else
        {
            DropsItemId = string.Empty;
            DropsItemName = string.Empty;
            NotifyOfPropertyChange(nameof(DropsItemName));
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
        set
        {
            SetTaskConfig<FightTask>(t => t.DropId == value, t => t.DropId = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    /// <summary>
    /// Gets or sets the item Name of drops.
    /// </summary>
    public string DropsItemName { get; set; } = string.Empty;

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
        set
        {
            SetTaskConfig<FightTask>(t => t.DropCount == value, t => t.DropCount = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    #endregion Drops

    public static Dictionary<string, string> AnnihilationModeList { get; } = new()
    {
        { LocalizationHelper.GetString("Annihilation"), "Annihilation" },
        { LocalizationHelper.GetString("Chernobog"), "Chernobog@Annihilation" },
        { LocalizationHelper.GetString("LungmenOutskirts"), "LungmenOutskirts@Annihilation" },
        { LocalizationHelper.GetString("LungmenDowntown"), "LungmenDowntown@Annihilation" },
    };

    public bool UseCustomAnnihilation
    {
        get => GetTaskConfig<FightTask>().UseCustomAnnihilation;
        set => SetTaskConfig<FightTask>(t => t.UseCustomAnnihilation == value, t => t.UseCustomAnnihilation = value);
    }

    public string AnnihilationStage
    {
        get => GetTaskConfig<FightTask>().AnnihilationStage;
        set => SetTaskConfig<FightTask>(t => t.AnnihilationStage == value, t => t.AnnihilationStage = value);
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
        set
        {
            SetTaskConfig<FightTask>(t => t.UseOptionalStage == value, t => t.UseOptionalStage = value);
            if (value)
            {
                HideUnavailableStage = false;
            }
        }
    }

    public bool AllowUseStoneSave
    {
        get => GetTaskConfig<FightTask>().UseStoneAllowSave;
        set
        {
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
        set
        {
            SetTaskConfig<FightTask>(t => t.UseExpiringMedicine == value, t => t.UseExpiringMedicine = value);
            SerializeTask(TaskSettingVisibilityInfo.CurrentTask, TaskSettingVisibilityInfo.CurrentTask.TaskId);
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to hide unavailable stages.
    /// </summary>
    public bool HideUnavailableStage
    {
        get => GetTaskConfig<FightTask>().HideUnavailableStage;
        set
        {
            SetTaskConfig<FightTask>(t => t.HideUnavailableStage == value, t => t.HideUnavailableStage = value);
            if (value)
            {
                UseAlternateStage = false;
            }

            UpdateStageList();
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to hide series.
    /// </summary>
    public bool HideSeries
    {
        get => GetTaskConfig<FightTask>().HideSeries;
        set => SetTaskConfig<FightTask>(t => t.HideSeries == value, t => t.HideSeries = value);
    }

    private bool _autoRestartOnDrop = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, bool.TrueString));

    public bool AutoRestartOnDrop
    {
        get => _autoRestartOnDrop;
        set
        {
            SetAndNotify(ref _autoRestartOnDrop, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AutoRestartOnDrop, value.ToString());
        }
    }

    private string ToUpperAndCheckStage(string value)
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

        if (StageList == null)
        {
            return value;
        }

        foreach (var item in StageList)
        {
            if (upperValue == item.Value.ToUpper() || upperValue == item.Display.ToUpper())
            {
                return item.Value;
            }
        }

        return value;
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is FightTask)
        {
            Refresh();
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstFightTask()
        {
            Stage = Stage,
            Medicine = UseMedicine != false ? MedicineNumber : 0,
            Stone = UseStoneDisplay ? StoneNumber : 0,
            Series = Series,
            MaxTimes = HasTimesLimited != false ? MaxTimes : int.MaxValue,
            ExpiringMedicine = UseExpiringMedicine ? 9999 : 0,
            IsDrGrandet = IsDrGrandet,
            ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
            ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
            PenguinId = SettingsViewModel.GameSettings.PenguinId,
            YituliuId = SettingsViewModel.GameSettings.PenguinId,
            ServerType = Instances.SettingsViewModel.ServerType,
            ClientType = SettingsViewModel.GameSettings.ClientType,
        };

        if (Stage == "Annihilation" && UseCustomAnnihilation)
        {
            task.Stage = AnnihilationStage;
        }

        if (IsSpecifiedDrops != false && !string.IsNullOrEmpty(DropsItemId))
        {
            task.Drops.Add(DropsItemId, DropsQuantity);
        }

        return task.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not FightTask fight)
        {
            return null;
        }

        var list = new List<string>() { fight.Stage1, fight.Stage2, fight.Stage3, fight.Stage4 };
        string stage = !fight.UseOptionalStage ? list.First() : (list.FirstOrDefault(Instances.TaskQueueViewModel.IsStageOpen) ?? list.First());
        var task = new AsstFightTask()
        {
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

        if (Stage == "Annihilation" && fight.UseCustomAnnihilation)
        {
            task.Stage = fight.AnnihilationStage;
        }

        if (fight.EnableTargetDrop != false && !string.IsNullOrEmpty(fight.DropId))
        {
            task.Drops.Add(fight.DropId, fight.DropCount);
        }

        if (taskId is int id)
        {
            return Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task);
        }
        else
        {
            return Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, task);
        }
    }

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
        Execute.PostToUIThreadAsync(() =>
        {
            var hideUnavailableStage = HideUnavailableStage;

            Instances.TaskQueueViewModel.EnableSetFightParams = false;

            var stage1 = Stage1 ?? string.Empty;
            var stage2 = Stage2 ?? string.Empty;
            var stage3 = Stage3 ?? string.Empty;
            var stage4 = Stage4 ?? string.Empty;

            var tempStageList = hideUnavailableStage
                ? Instances.StageManager.GetStageList(Instances.TaskQueueViewModel.CurDayOfWeek).ToList()
                : Instances.StageManager.GetStageList().ToList();

            if (CustomStageCode)
            {
                // 7%
                // 使用自定义的时候不做处理
            }
            else if (hideUnavailableStage)
            {
                // 15%
                stage1 = Instances.TaskQueueViewModel.GetValidStage(stage1);
                stage2 = Instances.TaskQueueViewModel.GetValidStage(stage2);
                stage3 = Instances.TaskQueueViewModel.GetValidStage(stage3);
                stage4 = Instances.TaskQueueViewModel.GetValidStage(stage4);
            }
            else if (UseAlternateStage)
            {
                // 11%
                AddStagesIfNotExist([stage1, stage2, stage3, stage4], tempStageList);
            }
            else
            {
                // 啥都没选
                AddStageIfNotExist(stage1, tempStageList);

                // 避免关闭了使用备用关卡后，始终添加备用关卡中的未开放关卡
                stage2 = Instances.TaskQueueViewModel.GetValidStage(stage2);
                stage3 = Instances.TaskQueueViewModel.GetValidStage(stage3);
                stage4 = Instances.TaskQueueViewModel.GetValidStage(stage4);
            }

            UpdateObservableCollection(StageList, tempStageList);

            Stage1 = stage1;
            Stage2 = stage2;
            Stage3 = stage3;
            Stage4 = stage4;
            if (!CustomStageCode)
            {
                RemoveNonExistStage();
            }

            Instances.TaskQueueViewModel.EnableSetFightParams = true;
        });
    }

    private void AddStagesIfNotExist(IEnumerable<string> stages, List<CombinedData> stageList)
    {
        foreach (var stage in stages)
        {
            AddStageIfNotExist(stage, stageList);
        }
    }

    private void AddStageIfNotExist(string stage, List<CombinedData> stageList)
    {
        if (stageList.Any(x => x.Value == stage))
        {
            return;
        }

        var stageInfo = Instances.StageManager.GetStageInfo(stage);
        stageList.Add(stageInfo);
    }

    /// <summary>
    /// 更新 ObservableCollection，确保不替换原集合，而是增删项
    /// </summary>
    /// <param name="originalCollection">原始 ObservableCollection</param>
    /// <param name="newList">新的列表</param>
    public static void UpdateObservableCollection(ObservableCollection<CombinedData> originalCollection, List<CombinedData> newList)
    {
        originalCollection.Clear();

        foreach (var item in newList)
        {
            originalCollection.Add(item);
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
    }
}
