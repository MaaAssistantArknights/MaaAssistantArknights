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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 刷理智
/// </summary>
public class FightSettingsUserControlModel : TaskViewModel
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

    private ObservableCollection<CombinedData> _remainingSanityStageList = [];

    public ObservableCollection<CombinedData> RemainingSanityStageList
    {
        get => _remainingSanityStageList;
        private set => SetAndNotify(ref _remainingSanityStageList, value);
    }

    /// <summary>
    /// Gets the stage.
    /// </summary>
    public string Stage
    {
        get
        {
            Stage1 ??= _stage1Fallback;

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

    /// <remarks>Try to fix: issues#5742. 关卡选择为 null 时的一个补丁，可能是 StageList 改变后，wpf binding 延迟更新的问题。</remarks>
    public string _stage1Fallback = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;

    private string? _stage1 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the stage1.
    /// </summary>
    public string? Stage1
    {
        get => _stage1;
        set
        {
            if (_stage1 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                // 从后往前删
                if (_stage1?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref _stage1, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.Stage1, value);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    private string? _stage2 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage2, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the stage2.
    /// </summary>
    public string? Stage2
    {
        get => _stage2;
        set
        {
            if (_stage2 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (_stage2?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref _stage2, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.Stage2, value);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    private string? _stage3 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage3, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the stage3.
    /// </summary>
    public string? Stage3
    {
        get => _stage3;
        set
        {
            if (_stage3 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (_stage3?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref _stage3, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.Stage3, value);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    private string? _stage4 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage4, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the stage4.
    /// </summary>
    public string? Stage4
    {
        get => _stage4;
        set
        {
            if (_stage4 == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (_stage4?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref _stage4, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.Stage4, value);
            Instances.TaskQueueViewModel.UpdateDatePrompt();
        }
    }

    private bool _useRemainingSanityStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, bool.TrueString));

    public bool UseRemainingSanityStage
    {
        get => _useRemainingSanityStage;
        set
        {
            SetAndNotify(ref _useRemainingSanityStage, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.UseRemainingSanityStage, value.ToString());
        }
    }

    private bool _customStageCode = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomStageCode, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use custom stage code.
    /// </summary>
    public bool CustomStageCode
    {
        get => _customStageCode;
        set
        {
            if (!value)
            {
                RemoveNonExistStage();
            }

            SetAndNotify(ref _customStageCode, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomStageCode, value.ToString());
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
        RemainingSanityStage = RemainingSanityStageList.FirstOrDefault(x => x.Value == RemainingSanityStage)?.Value ?? string.Empty;
    }

    private string? _remainingSanityStage = ConfigurationHelper.GetValue(ConfigurationKeys.RemainingSanityStage, string.Empty) ?? string.Empty;

    public string? RemainingSanityStage
    {
        get => _remainingSanityStage;
        set
        {
            if (_remainingSanityStage == value)
            {
                return;
            }

            if (CustomStageCode)
            {
                if (_remainingSanityStage?.Length != 3 && value != null)
                {
                    value = ToUpperAndCheckStage(value);
                }
            }

            SetAndNotify(ref _remainingSanityStage, value);
            TaskQueueViewModel.SetFightRemainingSanityParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.RemainingSanityStage, value);
        }
    }

    /// <summary>
    /// Reset unsaved battle parameters.
    /// </summary>
    public void ResetFightVariables()
    {
        if (UseStoneWithNull == null)
        {
            UseStone = false;
        }

        if (UseMedicineWithNull == null)
        {
            UseMedicine = false;
        }

        if (HasTimesLimitedWithNull == null)
        {
            HasTimesLimited = false;
        }

        if (IsSpecifiedDropsWithNull == null)
        {
            IsSpecifiedDrops = false;
        }
    }

    private bool? _useMedicineWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicine, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use medicine with null.
    /// </summary>
    public bool? UseMedicineWithNull
    {
        get => _useMedicineWithNull;
        set
        {
            SetAndNotify(ref _useMedicineWithNull, value);
            if (value == false)
            {
                UseStone = false;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            value ??= false;
            ConfigurationHelper.SetValue(ConfigurationKeys.UseMedicine, value.ToString());
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use medicine.
    /// </summary>
    public bool UseMedicine
    {
        get => UseMedicineWithNull != false;
        set => UseMedicineWithNull = value;
    }

    private int _medicineNumber = ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicineQuantity, 999);

    /// <summary>
    /// Gets or sets the amount of medicine used.
    /// </summary>
    public int MedicineNumber
    {
        get => _medicineNumber;
        set
        {
            if (!SetAndNotify(ref _medicineNumber, value))
            {
                return;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.UseMedicineQuantity, value.ToString());
        }
    }

    public static string UseStoneString => LocalizationHelper.GetString("UseOriginitePrime");

    private bool? _useStoneWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicine, bool.FalseString)) &&
                                      Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseStone, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use originiums with null.
    /// </summary>
    public bool? UseStoneWithNull
    {
        get => _useStoneWithNull;
        set
        {
            if (!AllowUseStoneSave && value == true)
            {
                value = null;
            }

            SetAndNotify(ref _useStoneWithNull, value);
            if (value != false)
            {
                MedicineNumber = 999;
                if (!UseMedicine)
                {
                    UseMedicineWithNull = value;
                }
            }

            // IsEnabled="{c:Binding UseStone}"
            NotifyOfPropertyChange(nameof(UseStone));

            Instances.TaskQueueViewModel.SetFightParams();
            if (AllowUseStoneSave)
            {
                ConfigurationHelper.SetValue(ConfigurationKeys.UseStone, (value ?? false).ToString());
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether to use originiums.
    /// </summary>
    // ReSharper disable once MemberCanBePrivate.Global
    public bool UseStone
    {
        get => UseStoneWithNull != false;
        set => UseStoneWithNull = value;
    }

    private int _stoneNumber = ConfigurationHelper.GetValue(ConfigurationKeys.UseStoneQuantity, 0);

    /// <summary>
    /// Gets or sets the amount of originiums used.
    /// </summary>
    public int StoneNumber
    {
        get => _stoneNumber;
        set
        {
            if (!SetAndNotify(ref _stoneNumber, value))
            {
                return;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.UseStoneQuantity, value.ToString());
        }
    }

    private bool? _hasTimesLimitedWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimited, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether the number of times is limited with null.
    /// </summary>
    public bool? HasTimesLimitedWithNull
    {
        get => _hasTimesLimitedWithNull;
        set
        {
            SetAndNotify(ref _hasTimesLimitedWithNull, value);
            Instances.TaskQueueViewModel.SetFightParams();
            value ??= false;
            ConfigurationHelper.SetValue(ConfigurationKeys.TimesLimited, value.ToString());
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the number of times is limited.
    /// </summary>
    public bool HasTimesLimited
    {
        get => HasTimesLimitedWithNull != false;
        set => HasTimesLimitedWithNull = value;
    }

    private int _maxTimes = ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimitedQuantity, 5);

    /// <summary>
    /// Gets or sets the max number of times.
    /// </summary>
    public int MaxTimes
    {
        get => _maxTimes;
        set
        {
            if (!SetAndNotify(ref _maxTimes, value))
            {
                return;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.TimesLimitedQuantity, value.ToString());
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
        { LocalizationHelper.GetString("NotSelected"), -1 },
    };

    private int _series = InitFightSeries();

    private static int InitFightSeries()
    {
        var series = ConfigurationHelper.GetValue(ConfigurationKeys.SeriesQuantity, 0);
        if (SeriesList.ContainsValue(series))
        {
            return series;
        }

        ConfigurationHelper.SetValue(ConfigurationKeys.SeriesQuantity, "0");
        return 0;
    }

    /// <summary>
    /// Gets or sets the max number of times.
    /// </summary>
    public int Series
    {
        get => _series;
        set
        {
            if (!SetAndNotify(ref _series, value))
            {
                return;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.SeriesQuantity, value.ToString());
        }
    }

    #region Drops

    private bool? _isSpecifiedDropsWithNull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.DropsEnable, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether the drops are specified.
    /// </summary>
    public bool? IsSpecifiedDropsWithNull
    {
        get => _isSpecifiedDropsWithNull;
        set
        {
            if (!SetAndNotify(ref _isSpecifiedDropsWithNull, value))
            {
                return;
            }

            Instances.TaskQueueViewModel.SetFightParams();
            value ??= false;
            ConfigurationHelper.SetValue(ConfigurationKeys.DropsEnable, value.ToString());
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the drops are specified.
    /// </summary>
    public bool IsSpecifiedDrops
    {
        get => IsSpecifiedDropsWithNull != false;
        set => IsSpecifiedDropsWithNull = value;
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
            "3105", "3131", "3132", "3233", // 龙骨/加固建材
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

            AllDrops.Add(new CombinedData { Display = dis, Value = val });
        }

        AllDrops.Sort((a, b) => string.Compare(a.Value, b.Value, StringComparison.Ordinal));
        DropsList = [.. AllDrops];
    }

    /// <summary>
    /// Gets or private sets the list of drops.
    /// </summary>
    public ObservableCollection<CombinedData> DropsList { get; private set; } = [];

    private string _dropsItemId = ConfigurationHelper.GetValue(ConfigurationKeys.DropsItemId, string.Empty) ?? string.Empty;

    /// <summary>
    /// Gets or sets the item ID of drops.
    /// </summary>
    public string DropsItemId
    {
        get => _dropsItemId;
        set
        {
            SetAndNotify(ref _dropsItemId, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.DropsItemId, DropsItemId);
        }
    }

    private string _dropsItemName = ConfigurationHelper.GetValue(ConfigurationKeys.DropsItemName, LocalizationHelper.GetString("NotSelected"));

    /// <summary>
    /// Gets or sets the item Name of drops.
    /// </summary>
    public string DropsItemName
    {
        get => _dropsItemName;
        set
        {
            SetAndNotify(ref _dropsItemName, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.DropsItemName, DropsItemName);
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void DropsListDropDownClosed()
    {
        foreach (var item in DropsList)
        {
            if (DropsItemName != item.Display)
            {
                continue;
            }

            DropsItemId = item.Value;

            if (DropsItemName != item.Display || DropsItemId != item.Value)
            {
                DropsItemName = LocalizationHelper.GetString("NotSelected");
            }

            return;
        }

        DropsItemName = LocalizationHelper.GetString("NotSelected");
    }

    private int _dropsQuantity = ConfigurationHelper.GetValue(ConfigurationKeys.DropsQuantity, 5);

    /// <summary>
    /// Gets or sets the quantity of drops.
    /// </summary>
    public int DropsQuantity
    {
        get => _dropsQuantity;
        set
        {
            SetAndNotify(ref _dropsQuantity, value);
            Instances.TaskQueueViewModel.SetFightParams();
            ConfigurationHelper.SetValue(ConfigurationKeys.DropsQuantity, value.ToString());
        }
    }

    #endregion Drops

    private bool _isDrGrandet = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.IsDrGrandet, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use DrGrandet mode.
    /// </summary>
    public bool IsDrGrandet
    {
        get => _isDrGrandet;
        set
        {
            SetAndNotify(ref _isDrGrandet, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.IsDrGrandet, value.ToString());
        }
    }

    private bool _useAlternateStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseAlternateStage, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to use alternate stage.
    /// </summary>
    public bool UseAlternateStage
    {
        get => _useAlternateStage;
        set
        {
            SetAndNotify(ref _useAlternateStage, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.UseAlternateStage, value.ToString());
            if (value)
            {
                HideUnavailableStage = false;
            }
        }
    }

    private bool _allowUseStoneSave = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowUseStoneSave, bool.FalseString));

    public bool AllowUseStoneSave
    {
        get => _allowUseStoneSave;
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

            SetAndNotify(ref _allowUseStoneSave, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AllowUseStoneSave, value.ToString());
        }
    }

    private bool _useExpiringMedicine = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseExpiringMedicine, bool.FalseString));

    public bool UseExpiringMedicine
    {
        get => _useExpiringMedicine;
        set
        {
            SetAndNotify(ref _useExpiringMedicine, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.UseExpiringMedicine, value.ToString());
            Instances.TaskQueueViewModel.SetFightParams();
        }
    }

    private bool _hideUnavailableStage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideUnavailableStage, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to hide unavailable stages.
    /// </summary>
    public bool HideUnavailableStage
    {
        get => _hideUnavailableStage;
        set
        {
            SetAndNotify(ref _hideUnavailableStage, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.HideUnavailableStage, value.ToString());

            if (value)
            {
                UseAlternateStage = false;
            }

            Instances.TaskQueueViewModel.UpdateStageList();
        }
    }

    private bool _hideSeries = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideSeries, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether to hide series.
    /// </summary>
    public bool HideSeries
    {
        get => _hideSeries;
        set
        {
            SetAndNotify(ref _hideSeries, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.HideSeries, value.ToString());
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

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstFightTask()
        {
            Stage = Stage,
            Medicine = UseMedicine ? MedicineNumber : 0,
            Stone = UseStone ? StoneNumber : 0,
            Series = Series,
            MaxTimes = HasTimesLimited ? MaxTimes : int.MaxValue,
            ExpiringMedicine = UseExpiringMedicine ? 9999 : 0,
            IsDrGrandet = IsDrGrandet,
            ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
            ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
            PenguinId = SettingsViewModel.GameSettings.PenguinId,
            YituliuId = SettingsViewModel.GameSettings.PenguinId,
            ServerType = Instances.SettingsViewModel.ServerType,
            ClientType = SettingsViewModel.GameSettings.ClientType,
        };

        if (IsSpecifiedDrops && !string.IsNullOrEmpty(DropsItemId))
        {
            task.Drops.Add(DropsItemId, DropsQuantity);
        }

        return task.Serialize();
    }

    #region 双入口设置可见性

    private bool _customInfrastPlanShowInFightSettings = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanShowInFightSettings, bool.FalseString));

    public bool CustomInfrastPlanShowInFightSettings
    {
        get => _customInfrastPlanShowInFightSettings;
        set
        {
            SetAndNotify(ref _customInfrastPlanShowInFightSettings, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CustomInfrastPlanShowInFightSettings, value.ToString());
        }
    }

    #endregion

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
