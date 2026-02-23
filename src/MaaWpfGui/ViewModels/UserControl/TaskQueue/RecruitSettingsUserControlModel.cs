// <copyright file="RecruitSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 自动公招model
/// </summary>
public class RecruitSettingsUserControlModel : TaskSettingsViewModel, RecruitSettingsUserControlModel.ISerialize
{
    static RecruitSettingsUserControlModel()
    {
        Instance = new();
    }

    public static RecruitSettingsUserControlModel Instance { get; }

    private static readonly List<string> _autoRecruitTagList = ["近战位", "远程位", "先锋干员", "近卫干员", "狙击干员", "重装干员", "医疗干员", "辅助干员", "术师干员", "治疗", "费用回复", "输出", "生存", "群攻", "防护", "减速",];

    private static readonly Lazy<List<CombinedData>> _autoRecruitTagShowList = new(() =>
        _autoRecruitTagList.Select<string, (string, string)?>(tag => DataHelper.RecruitTags.TryGetValue(tag, out var value) ? value : null)
            .Where(tag => tag is not null)
            .Cast<(string Display, string Client)>()
            .Select(tag => new CombinedData() { Display = tag.Display, Value = tag.Client })
            .ToList());

    public static List<CombinedData> AutoRecruitTagShowList
    {
        get => _autoRecruitTagShowList.Value;
    }

    public object[] AutoRecruitFirstList
    {
        get {
            var value = GetTaskConfig<RecruitTask>().Level3PreferTags;
            return value.Select(tag => _autoRecruitTagShowList.Value.FirstOrDefault(i => i.Value == tag)).Where(v => v is not null).Cast<CombinedData>().ToArray();
        }

        set {
            var config = value.Cast<CombinedData>().Select(k => k.Value).ToList();
            SetTaskConfig<RecruitTask>(t => t.Level3PreferTags == config, t => t.Level3PreferTags = config);
        }
    }

    /// <summary>
    /// Gets or sets the maximum times of recruit.
    /// </summary>
    public int RecruitMaxTimes
    {
        get => GetTaskConfig<RecruitTask>().MaxTimes;
        set => SetTaskConfig<RecruitTask>(t => t.MaxTimes == value, t => t.MaxTimes = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to refresh level 3.
    /// </summary>
    public bool RefreshLevel3
    {
        get => GetTaskConfig<RecruitTask>().RefreshLevel3;
        set => SetTaskConfig<RecruitTask>(t => t.RefreshLevel3 == value, t => t.RefreshLevel3 = value);
    }

    /// <summary>
    /// Gets or Sets a value indicating whether to refresh when recruit permit ran out.
    /// </summary>
    public bool ForceRefresh
    {
        get => GetTaskConfig<RecruitTask>().ForceRefresh;
        set => SetTaskConfig<RecruitTask>(t => t.ForceRefresh == value, t => t.ForceRefresh = value);
    }

    public bool? UseExpeditedWithNull
    {
        get => GetTaskConfig<RecruitTask>().UseExpedited;
        set {
            if (value == true)
            {
                value = null;
            }

            SetTaskConfig<RecruitTask>(t => t.UseExpedited == value, t => t.UseExpedited = value);
        }
    }

    public static void ResetRecruitVariables(RecruitTask? recruit)
    {
        recruit?.UseExpedited ??= false;
    }

    /// <summary>
    /// Gets the list of auto recruit selecting extra tags.
    /// </summary>
    public List<GenericCombinedData<int>> AutoRecruitSelectExtraTagsList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("DefaultNoExtraTags"), Value = 0 },
            new() { Display = LocalizationHelper.GetString("SelectExtraTags"), Value = 1 },
            new() { Display = LocalizationHelper.GetString("SelectExtraOnlyRareTags"), Value = 2 },
        ];

    /// <summary>
    /// Gets or sets a value indicating three tags are always selected or select only rare tags as many as possible .
    /// </summary>
    public int SelectExtraTags
    {
        get => GetTaskConfig<RecruitTask>().ExtraTagMode;
        set => SetTaskConfig<RecruitTask>(t => t.ExtraTagMode == value, t => t.ExtraTagMode = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether not to choose level 1.
    /// </summary>
    public bool NotChooseLevel1
    {
        get => GetTaskConfig<RecruitTask>().Level1NotChoose;
        set => SetTaskConfig<RecruitTask>(t => t.Level1NotChoose == value, t => t.Level1NotChoose = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 3.
    /// </summary>
    public bool ChooseLevel3
    {
        get => GetTaskConfig<RecruitTask>().Level3Choose;
        set => SetTaskConfig<RecruitTask>(t => t.Level3Choose == value, t => t.Level3Choose = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 4.
    /// </summary>
    public bool ChooseLevel4
    {
        get => GetTaskConfig<RecruitTask>().Level4Choose;
        set => SetTaskConfig<RecruitTask>(t => t.Level4Choose == value, t => t.Level4Choose = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether to choose level 5.
    /// </summary>
    public bool ChooseLevel5
    {
        get => GetTaskConfig<RecruitTask>().Level5Choose;
        set => SetTaskConfig<RecruitTask>(t => t.Level5Choose == value, t => t.Level5Choose = value);
    }

    [PropertyDependsOn(nameof(ChooseLevel3Time))]
    public int ChooseLevel3Hour
    {
        get => ChooseLevel3Time / 60;
        set => ChooseLevel3Time = (value * 60) + ChooseLevel3Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel3Time))]
    public int ChooseLevel3Min
    {
        get => (ChooseLevel3Time % 60) / 10 * 10;
        set => ChooseLevel3Time = (ChooseLevel3Hour * 60) + value;
    }

    public int ChooseLevel3Time
    {
        get => GetTaskConfig<RecruitTask>().Level3Time;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetTaskConfig<RecruitTask>(t => t.Level3Time == value, t => t.Level3Time = value);
        }
    }

    [PropertyDependsOn(nameof(ChooseLevel4Time))]
    public int ChooseLevel4Hour
    {
        get => ChooseLevel4Time / 60;
        set => ChooseLevel4Time = (value * 60) + ChooseLevel4Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel4Time))]
    public int ChooseLevel4Min
    {
        get => (ChooseLevel4Time % 60) / 10 * 10;
        set => ChooseLevel4Time = (ChooseLevel4Hour * 60) + value;
    }

    public int ChooseLevel4Time
    {
        get => GetTaskConfig<RecruitTask>().Level4Time;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };
            SetTaskConfig<RecruitTask>(t => t.Level4Time == value, t => t.Level4Time = value);
        }
    }

    [PropertyDependsOn(nameof(ChooseLevel5Time))]
    public int ChooseLevel5Hour
    {
        get => ChooseLevel5Time / 60;
        set => ChooseLevel5Time = (value * 60) + ChooseLevel5Min;
    }

    [PropertyDependsOn(nameof(ChooseLevel5Time))]
    public int ChooseLevel5Min
    {
        get => (ChooseLevel5Time % 60) / 10 * 10;
        set => ChooseLevel5Time = (ChooseLevel5Hour * 60) + value;
    }

    public int ChooseLevel5Time
    {
        get => GetTaskConfig<RecruitTask>().Level5Time;
        set {
            value = value switch {
                < 60 => 9 * 60,
                > 9 * 60 => 60,
                _ => value / 10 * 10,
            };

            SetTaskConfig<RecruitTask>(t => t.Level5Time == value, t => t.Level5Time = value);
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is RecruitTask)
        {
            Refresh();
        }
    }

    public override bool? SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize)?.Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        bool? ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not RecruitTask recruit)
            {
                return null;
            }

            var task = new AsstRecruitTask() {
                Refresh = recruit.RefreshLevel3,
                ForceRefresh = recruit.ForceRefresh,
                SetRecruitTime = true,
                RecruitTimes = recruit.MaxTimes,
                UseExpedited = recruit.UseExpedited is not false,
                ExpeditedTimes = recruit.MaxTimes,
                SelectExtraTags = recruit.ExtraTagMode,
                Level3FirstList = recruit.Level3PreferTags,
                NotChooseLevel1 = recruit.Level1NotChoose,
                ChooseLevel3Time = recruit.Level3Time,
                ChooseLevel4Time = recruit.Level4Time,
                ChooseLevel5Time = recruit.Level5Time,
                ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
                ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
                PenguinId = SettingsViewModel.GameSettings.PenguinId,
                YituliuId = SettingsViewModel.GameSettings.PenguinId,
                ServerType = Instances.SettingsViewModel.ServerType,
            };

            if (recruit.Level1NotChoose)
            {
                task.ConfirmList.Add(1);
            }

            if (recruit.Level3Choose)
            {
                task.ConfirmList.Add(3);
            }

            if (recruit.Level4Choose)
            {
                task.SelectList.Add(4);
                task.ConfirmList.Add(4);
            }

            // ReSharper disable once InvertIf
            if (recruit.Level5Choose)
            {
                task.SelectList.Add(5);
                task.ConfirmList.Add(5);
            }

            return taskId switch {
                int id when id > 0 => Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task),
                null => Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Recruit, task),
                _ => null,
            };
        }
    }
}
