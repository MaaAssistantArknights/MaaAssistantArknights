// <copyright file="MiniGameTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json.Linq;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// 牛杂
/// </summary>
public class MiniGameTaskUserControlModel : TaskSettingsViewModel, MiniGameTaskUserControlModel.ISerialize
{
    static MiniGameTaskUserControlModel()
    {
        Instance = new();
    }

    public MiniGameTaskUserControlModel()
    {
        foreach (var item in WeeklyScheduleSource)
        {
            item.PropertyChanged += (_, __) => SaveWeeklySchedule();
        }

        foreach (var item in MonthlyScheduleSource)
        {
            item.PropertyChanged += (_, __) => SaveMonthlySchedule();
        }

        // 本类型为 Instance 单例，构造仅执行一次，订阅后无需取消订阅
        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    public static MiniGameTaskUserControlModel Instance { get; }

    #region 牛杂条目

    public ObservableCollection<GenericCombinedData<string>> MiniGameEntryList { get; } = [];

    // 任务队列里只有绿票/黄票商店能稳定执行，其余牛杂在下拉框中置灰不可选
    private static readonly HashSet<string> _selectableMiniGameEntries =
    [
        "GreenTicket@Store@Begin",
        "YellowTicket@Store@Begin",
    ];

    private string _miniGameName = string.Empty;

    /// <summary>
    /// Gets or sets 选中的牛杂名（tasks.json 中的 key）。
    /// </summary>
    public string MiniGameName
    {
        get => _miniGameName;
        set {
            // 条目列表重建时 SelectedValue 会短暂置空，此时不回写配置，避免清空已保存的选择
            if (value is null)
            {
                return;
            }

            if (!SetAndNotify(ref _miniGameName, value))
            {
                return;
            }

            SetTaskConfig<MiniGameTask>(t => t.MiniGameName == value, t => t.MiniGameName = value);
            NotifyOfPropertyChange(nameof(MiniGameTip));
            NotifyOfPropertyChange(nameof(IsSecretFront));
            NotifyOfPropertyChange(nameof(IsAutoRaisePotential));
        }
    }

    private string? _miniGameTip;

    /// <summary>
    /// Gets 选中牛杂的说明文案，条目列表刷新时一并重置。
    /// </summary>
    public string MiniGameTip => _miniGameTip ??= GetMiniGameTip(MiniGameName);

    /// <summary>
    /// 清空缓存的说明文案，使其在下次读取时按当前语言与选中项重新生成。
    /// </summary>
    private void ResetMiniGameTip()
    {
        _miniGameTip = null;
        NotifyOfPropertyChange(nameof(MiniGameTip));
    }

    /// <summary>
    /// Gets a value indicating whether 选中项是否为隐秘战线（需要结局/系列事件参数）。
    /// </summary>
    public bool IsSecretFront => MiniGameName == "MiniGame@SecretFront";

    /// <summary>
    /// Gets a value indicating whether 选中项是否为自动提升潜能（需要普通信物开关）。
    /// </summary>
    public bool IsAutoRaisePotential => MiniGameName == "MiniGame@AutoRaisePotential@Begin";

    public List<string> SecretFrontEndingList { get; } = ["A", "B", "C", "D", "E"];

    // 值与工具箱页面保持一致：系列事件由插件按界面上的事件名（中文）匹配，不是 tasks.json 的 key
    public LocalizedObservableList<string> SecretFrontEventList { get; } = new(
        (string.Empty, "NotSelected"),
        ("支援作战平台", "MiniGame@SecretFront@Event1"),
        ("游侠", "MiniGame@SecretFront@Event2"),
        ("诡影迷踪", "MiniGame@SecretFront@Event3"));

    public string SecretFrontEnding
    {
        get; set {
            SetAndNotify(ref field, value);
            SetTaskConfig<MiniGameTask>(t => t.SecretFrontEnding == value, t => t.SecretFrontEnding = value);
        }
    } = "A";

    public string SecretFrontEvent
    {
        get; set {
            if (value is null)
            {
                return;
            }

            SetAndNotify(ref field, value);
            SetTaskConfig<MiniGameTask>(t => t.SecretFrontEvent == value, t => t.SecretFrontEvent = value);
        }
    } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 自动提升潜能：中坚信物不足时是否消耗普通信物。
    /// </summary>
    public bool UseNormalToken
    {
        get; set {
            SetAndNotify(ref field, value);
            SetTaskConfig<MiniGameTask>(t => t.UseNormalToken == value, t => t.UseNormalToken = value);
        }
    }

    /// <summary>
    /// 重建牛杂条目列表（活动数据更新或语言切换后调用），保留已选中的牛杂。
    /// </summary>
    public static void UpdateMiniGameEntryList()
    {
        var entries = Instances.StageManager.MiniGameEntries
            .Select(t => new GenericCombinedData<string> {
                Display = string.IsNullOrEmpty(t.DisplayKey)
                    ? t.Display
                    : (LocalizationHelper.TryGetString(t.DisplayKey, out var loc) ? loc : t.Display),
                Value = t.Value,
                IsEnabled = _selectableMiniGameEntries.Contains(t.Value),
            })
            .ToList();

        Execute.OnUIThread(() => {
            Instance.MiniGameEntryList.Clear();
            foreach (var item in entries)
            {
                Instance.MiniGameEntryList.Add(item);
            }

            // 列表清空期间 ComboBox 的 SelectedValue 会被置空（setter 已忽略 null 以免写坏配置），
            // 这里补一次通知把选中项重新推回 UI；提示文案也按当前语言重新取
            Instance.ResetMiniGameTip();
            Instance.NotifyOfPropertyChange(nameof(MiniGameName));
        });
    }

    private static string GetMiniGameTip(string name)
    {
        if (string.IsNullOrEmpty(name))
        {
            return LocalizationHelper.GetString("MiniGameNameEmptyTip");
        }

        var entry = Instances.StageManager.MiniGameEntries.FirstOrDefault(e => e.Value == name);
        if (entry == null)
        {
            return LocalizationHelper.GetString("MiniGameNameEmptyTip");
        }

        if (!string.IsNullOrEmpty(entry.TipKey) && LocalizationHelper.TryGetString(entry.TipKey, out var tipFromKey))
        {
            return tipFromKey;
        }

        if (!string.IsNullOrEmpty(entry.Tip))
        {
            return entry.Tip;
        }

        if (!string.IsNullOrEmpty(entry.DisplayKey) && LocalizationHelper.TryGetString(entry.DisplayKey, out var displayLoc))
        {
            return displayLoc;
        }

        return entry.Display;
    }

    #endregion 牛杂条目

    #region 周/月计划

    public LocalizedObservableList<MiniGameScheduleMode> ScheduleModeList { get; } = new(
        (MiniGameScheduleMode.None, "MiniGameScheduleNone"),
        (MiniGameScheduleMode.Weekly, "MiniGameScheduleWeekly"),
        (MiniGameScheduleMode.Monthly, "MiniGameScheduleMonthly"));

    public MiniGameScheduleMode ScheduleMode
    {
        get => GetTaskConfig<MiniGameTask>().ScheduleMode;
        set {
            if (!SetTaskConfig<MiniGameTask>(t => t.ScheduleMode == value, t => t.ScheduleMode = value))
            {
                return;
            }

            NotifyOfPropertyChange(nameof(ShowWeeklySchedule));
            NotifyOfPropertyChange(nameof(ShowMonthlySchedule));
        }
    }

    public ObservableCollection<FightSettingsUserControlModel.WeeklyScheduleItem> WeeklyScheduleSource { get; } =
        [.. Enum.GetValues<DayOfWeek>().Select(i => new FightSettingsUserControlModel.WeeklyScheduleItem(i))];

    public ObservableCollection<MonthlyScheduleItem> MonthlyScheduleSource { get; } =
        [.. Enumerable.Range(1, 31).Select(i => new MonthlyScheduleItem(i))];

    [PropertyDependsOn(nameof(ScheduleMode))]
    public bool ShowWeeklySchedule => ScheduleMode == MiniGameScheduleMode.Weekly;

    [PropertyDependsOn(nameof(ScheduleMode))]
    public bool ShowMonthlySchedule => ScheduleMode == MiniGameScheduleMode.Monthly;

    private void SaveWeeklySchedule()
    {
        if (IsRefreshingUI)
        {
            return;
        }

        var dict = WeeklyScheduleSource.ToDictionary(i => i.DayOfWeek, i => i.Value);
        SetTaskConfig<MiniGameTask>(t => t.WeeklySchedule.SequenceEqual(dict), t => t.WeeklySchedule = dict);
    }

    private void SaveMonthlySchedule()
    {
        if (IsRefreshingUI)
        {
            return;
        }

        var dict = MonthlyScheduleSource.ToDictionary(i => i.Day, i => i.Value);
        SetTaskConfig<MiniGameTask>(t => t.MonthlySchedule.SequenceEqual(dict), t => t.MonthlySchedule = dict);
    }

    private void RefreshWeeklySchedule()
    {
        var plan = GetTaskConfig<MiniGameTask>().WeeklySchedule;
        foreach (var item in WeeklyScheduleSource)
        {
            item.Value = plan.TryGetValue(item.DayOfWeek, out var value) && value;
        }
    }

    private void RefreshMonthlySchedule()
    {
        var plan = GetTaskConfig<MiniGameTask>().MonthlySchedule;
        foreach (var item in MonthlyScheduleSource)
        {
            item.Value = plan.TryGetValue(item.Day, out var value) && value;
        }
    }

    /// <summary>
    /// 按周/月计划判断该任务今日是否执行。
    /// </summary>
    /// <param name="task">牛杂任务配置。</param>
    /// <returns>是否执行。</returns>
    private static bool IsTodayScheduled(MiniGameTask task)
    {
        var queue = Instances.TaskQueueViewModel;
        switch (task.ScheduleMode)
        {
            case MiniGameScheduleMode.Weekly:
                return !task.WeeklySchedule.TryGetValue(queue.CurDayOfWeek, out var weeklyEnabled) || weeklyEnabled;
            case MiniGameScheduleMode.Monthly:
                return !task.MonthlySchedule.TryGetValue(queue.CurDayOfMonth, out var monthlyEnabled) || monthlyEnabled;
            default:
                return true;
        }
    }

    /// <summary>
    /// 计划未勾选今日时的跳过原因，已执行时返回 null。
    /// </summary>
    /// <param name="task">牛杂任务配置。</param>
    /// <returns>跳过原因。</returns>
    private static string? GetScheduleSkipReason(MiniGameTask task)
    {
        if (IsTodayScheduled(task))
        {
            return null;
        }

        return task.ScheduleMode switch {
            MiniGameScheduleMode.Weekly => LocalizationHelper.GetString("MiniGameSkippedWeeklySchedule"),
            MiniGameScheduleMode.Monthly => LocalizationHelper.GetString("MiniGameSkippedMonthlySchedule"),
            _ => null,
        };
    }

    #endregion 周/月计划

    private void RefreshLocalization()
    {
        UpdateMiniGameEntryList();
        ScheduleModeList.RefreshLocalization();
        SecretFrontEventList.RefreshLocalization();
        foreach (var item in WeeklyScheduleSource)
        {
            item.RefreshLocalization();
        }

        foreach (var item in MonthlyScheduleSource)
        {
            item.RefreshLocalization();
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not MiniGameTask task)
        {
            return;
        }

        IsRefreshingUI = true;
        MiniGameName = task.MiniGameName;
        SecretFrontEnding = string.IsNullOrEmpty(task.SecretFrontEnding) ? "A" : task.SecretFrontEnding;
        SecretFrontEvent = task.SecretFrontEvent ?? string.Empty;
        UseNormalToken = task.UseNormalToken;
        RefreshWeeklySchedule();
        RefreshMonthlySchedule();
        NotifyOfPropertyChange(nameof(ScheduleMode));
        NotifyOfPropertyChange(nameof(ShowWeeklySchedule));
        NotifyOfPropertyChange(nameof(ShowMonthlySchedule));
        IsRefreshingUI = false;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    /// <summary>
    /// 按配置拼出实际下发给 Core 的牛杂任务名（隐秘战线需要结局与系列事件后缀）。
    /// </summary>
    /// <param name="task">牛杂任务配置。</param>
    /// <returns>任务名。</returns>
    public static string GetMiniGameTaskName(MiniGameTask task)
    {
        if (!string.IsNullOrEmpty(task.MiniGameName) && task.MiniGameName == "MiniGame@SecretFront")
        {
            var ending = string.IsNullOrEmpty(task.SecretFrontEnding) ? "A" : task.SecretFrontEnding;
            return $"{task.MiniGameName}@Begin@Ending{ending}{(string.IsNullOrEmpty(task.SecretFrontEvent) ? string.Empty : $"@{task.SecretFrontEvent}")}";
        }

        return task.MiniGameName;
    }

    public class MonthlyScheduleItem(int day) : PropertyChangedBase
    {
        public string Display => Day.ToString(LocalizationHelper.CustomCultureInfo);

        public int Day { get; } = day;

        public bool Value { get => field; set => SetAndNotify(ref field, value); } = true;

        /// <summary>
        /// 语言切换后通知 Display 回读新文化的数字格式，Value（勾选状态）保持不变。
        /// </summary>
        public void RefreshLocalization() => NotifyOfPropertyChange(nameof(Display));
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not MiniGameTask miniGame)
            {
                return (null, []);
            }

            if (string.IsNullOrEmpty(miniGame.MiniGameName))
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("MiniGameSkippedEmptyName"), UiLogColor.Error);
                return (null, []);
            }

            if (GetScheduleSkipReason(miniGame) is { } reason)
            {
                Instances.TaskQueueViewModel.AddLog(reason, UiLogColor.Info);
                return (null, []);
            }

            Instances.TaskQueueViewModel.AddLogSection(miniGame.NameOrTaskType);

            var task = new AsstCustomTask() {
                CustomTasks = [GetMiniGameTaskName(miniGame)],
            };
            if (miniGame.UseNormalToken)
            {
                task.Params = JObject.FromObject(new {
                    auto_raise_potential = new {
                        use_normal_token = true,
                    },
                });
            }

            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.MiniGame, task)),
                _ => (null, []),
            };
        }
    }
}
