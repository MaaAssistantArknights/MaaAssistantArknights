// <copyright file="OperProgressTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class OperProgressTaskUserControlModel : TaskSettingsViewModel, OperProgressTaskUserControlModel.ISerialize
{
    // 待确认移除（AllowedFields 已作废：字段权威改为 OperProgressTask.Plan 对象，本次重构后仅 ParseAndValidate 引用）
    private static readonly HashSet<string> AllowedFields = ["name", "elite", "skills", "skill", "skill_master"];

    static OperProgressTaskUserControlModel() => Instance = new();

    public OperProgressTaskUserControlModel()
    {
        Instances.AsstProxy.AsstSubTaskMsgEvent += ProcOperProgressMsg;

        // 语言与干员名语言切换后刷新卡片上的本地化显示
        LocalizationHelper.LanguageChanged += RefreshPlanItemLocalization;
        SettingsViewModel.GuiSettings.OperNameLanguageChanged += RefreshPlanItemLocalization;
    }

    public static OperProgressTaskUserControlModel Instance { get; }

    /// <summary>干员培养计划条目，每项对应一名干员。</summary>
    public ObservableCollection<OperProgressPlanItemViewModel> PlanItems { get; private set => SetAndNotify(ref field, value); } = [];

    /// <summary>为 true 时不响应集合变更，避免刷新期间把条目回写任务配置。</summary>
    private bool _isRefreshing;

    private void RefreshPlanItems(OperProgressTask task)
    {
        var list = task.Plans.Select((plan, index) => {
            int elite = plan.elite;
            int mainSkillLevel = plan.skillLevel switch {
                SkillLevel.BaseLevel baseLevel => baseLevel.Level,
                SkillLevel.Specialization => 7,
                _ => 0,
            };

            var specializationLevel = plan.skillLevel switch {
                SkillLevel.Specialization specialization => specialization,
                _ => new(0, 0, 0),
            };

            return new OperProgressPlanItemViewModel(index, plan.role, plan.name, elite, mainSkillLevel, specializationLevel);
        }).ToList();
        PlanItems = [.. list];
        PlanItems.CollectionChanged += PlanItems_CollectionChanged;
        foreach (var item in PlanItems)
        {
            item.PropertyChanged += PlanItem_PropertyChanged;
        }
    }

    private void SavePlan()
    {
        var list = PlanItems.Select(item => {
            SkillLevel skillLevel;
            if (item.SpecializationSkillLevel.Any(x => x > 0))
            {
                skillLevel = new SkillLevel.Specialization(item.SpecializationSkillLevel.Skill1, item.SpecializationSkillLevel.Skill2, item.SpecializationSkillLevel.Skill3);
            }
            else
            {
                skillLevel = new SkillLevel.BaseLevel(item.MainSkillLevel);
            }

            return new Plan(item.Role, item.Name, item.Elite, null, skillLevel);
        }).ToList();
        SetTaskConfig<OperProgressTask>(t => t.Plans.SequenceEqual(list), t => t.Plans = list);
    }

    public record class OperItem(OperatorRole Role, string Name, string NameDisplay, int Rarity);

    /// <summary>可选择的干员名列表，按稀有度降序、名称升序排列，实时取自干员数据</summary>
    public List<GenericCombinedData<OperItem>> OperatorNames => [.. DataHelper.Operators.Values
        .Select(character => new OperItem(character.Role, character.Name!, DataHelper.GetLocalizedCharacterName(character) ?? character.Name!, character.Rarity))
        .OrderByDescending(entry => entry.Rarity)
        .ThenBy(entry => entry.Name, StringComparer.CurrentCulture)
        .Select(oper => new GenericCombinedData<OperItem>($"{oper.NameDisplay}[{oper.Rarity}★]",  oper))];

    public OperItem? OperSelect { get; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// 任务链结束后删除已完成条目
    /// </summary>
    public bool DeleteCompletedEntries
    {
        get => GetTaskConfig<OperProgressTask>().DeleteOnCompleted;
        set => SetTaskConfig<OperProgressTask>(t => t.DeleteOnCompleted == value, t => t.DeleteOnCompleted = value);
    }

    /// <summary>本轮运行中各条目的回调结果，键为 Core 收到的计划数组下标</summary>
    // 待确认移除（本次重构后下标不再参与判断，改以回调携带的干员名匹配）
    private readonly Dictionary<int, (string Name, bool Completed)> _runEntryResults = [];

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not OperProgressTask task)
        {
            return;
        }

        _isRefreshing = true;
        RefreshPlanItems(task);
        Refresh();
        _isRefreshing = false;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    /// <summary>记录单条培养结果，由 AsstProxy 在 UI 线程回调（回调线程已由 Execute.OnUIThread 保证）</summary>
    // 待确认移除（本次重构后 Key 下标不再参与判断，可简化为按干员名上报）
    public void OnTargetResult(int index, string name, bool completed)
    {
        if (index == 0)
        {
            _runEntryResults.Clear();
        }

        _runEntryResults[index] = (name, completed);
    }

    /// <summary>培养任务链结束：开启开关时删除结果为“成功/已满足”的干员条目，失败与跳过的保留</summary>
    public void OnSummary()
    {
        var completedNames = _runEntryResults.Where(kv => kv.Value.Completed).Select(kv => kv.Value.Name).ToHashSet();
        _runEntryResults.Clear();
        if (!GetTaskConfig<OperProgressTask>().DeleteOnCompleted || completedNames.Count == 0)
        {
            return;
        }

        // 以回调携带的干员名为主键匹配：展开后的计划数组下标与卡片并非一一对应
        var remaining = PlanItems.Where(item => !completedNames.Contains(item.Name)).ToList();
        if (remaining.Count == PlanItems.Count)
        {
            return;
        }

        ReplacePlanItems(remaining);
    }

    /// <summary>
    /// 把当前选择的干员加入计划，新增的卡片自动展开。
    /// </summary>
    public void AddOperator()
    {
        if (OperSelect is null)
        {
            return;
        }

        PlanItems.Add(new OperProgressPlanItemViewModel(PlanItems.Count, OperSelect.Role, OperSelect.Name, 0, 0, new(0, 0, 0)));
    }

    /// <summary>
    /// 从计划中移除指定的干员条目
    /// </summary>
    public void RemovePlan(OperProgressPlanItemViewModel? item)
    {
        if (item is null || !PlanItems.Remove(item))
        {
            return;
        }
    }

    private void PlanItems_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        foreach (var item in e.NewItems?.OfType<OperProgressPlanItemViewModel>() ?? [])
        {
            item.PropertyChanged += PlanItem_PropertyChanged;
        }

        if (_isRefreshing)
        {
            return;
        }

        ReindexPlanItems();
        SavePlan();
    }

    private void PlanItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_isRefreshing || !OperProgressPlanItemViewModel.IsPersistedProperty(e.PropertyName))
        {
            return;
        }

        SavePlan();
    }

    /// <summary>语言或干员名语言切换后刷新卡片上的本地化显示。</summary>
    private void RefreshPlanItemLocalization()
    {
        foreach (var item in PlanItems)
        {
            item.RefreshLocalizedText();
        }
    }

    /// <summary>按当前顺序重排各条目的序号。</summary>
    private void ReindexPlanItems()
    {
        for (int index = 0; index < PlanItems.Count; ++index)
        {
            PlanItems[index].Index = index;
        }
    }

    /// <summary>批量替换全部条目并重排序号，期间的集合变更不回写任务配置。</summary>
    private void ReplacePlanItems(IEnumerable<OperProgressPlanItemViewModel> items)
    {
        PlanItems = new(items);
        PlanItems.CollectionChanged += PlanItems_CollectionChanged;
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not OperProgressTask operProgress)
            {
                return (null, []);
            }

            if (operProgress.Plans.Count == 0)
            {
                return (null, []);
            }

            var task = new AsstOperProgressTask { Plans = operProgress.Plans };
            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.OperProgress, task)),
                _ => (null, []),
            };
        }
    }

    private static void ProcOperProgressMsg(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (type != AsstMsg.SubTaskExtraInfo || msg?.TaskChain != nameof(TaskType.OperProgress))
        {
            return;
        }
        switch (msg.What)
        {
            case "OperProgressTargetStart":
                Instances.TaskQueueViewModel.AddLog(
                    LocalizationHelper.GetStringFormat(
                        "OperProgressTargetStartLog",
                        (int)(msg.Details?["index"] ?? 0) + 1,
                        ProcOperProgressTargetName(msg.Details),
                        ProcOperProgressTargetDescription(msg.Details)),
                    UiLogColor.Info,
                    splitMode: TaskQueueViewModel.LogCardSplitMode.Before);
                break;

            case "OperProgressTargetResult":
                string action = msg.Details?["action"]?.ToString() ?? string.Empty;
                string result = msg.Details?["result"]?.ToString() ?? "unsupported";
                int? recognized = msg.Details?.Value<int?>("recognized");
                string recognizedKey = action switch {
                    "elite" => "OperProgressRecognizedElite",
                    "skills" => "OperProgressRecognizedSkillLevel",
                    "mastery" => "OperProgressRecognizedMastery",
                    _ => string.Empty,
                };
                string recognizedText = recognized is null || recognizedKey.Length == 0
                    ? string.Empty
                    : LocalizationHelper.GetStringFormat(recognizedKey, recognized.Value);
                Instances.TaskQueueViewModel.AddLog(
                    LocalizationHelper.GetStringFormat(
                        "OperProgressTargetResultLog",
                        (int)(msg.Details?["index"] ?? 0) + 1,
                        ProcOperProgressTargetName(msg.Details),
                        ProcOperProgressTargetDescription(msg.Details),
                        result) + recognizedText,
                    result is "completed" or "already_satisfied" ? UiLogColor.Success :
                    result is "skipped" or "formula_locked" ? UiLogColor.Warning : UiLogColor.Error);
                Instance.OnTargetResult(
                    (int)(msg.Details?["index"] ?? -1),
                    msg.Details?["name"]?.ToString() ?? string.Empty,
                    result is "completed" or "already_satisfied");
                break;

            case "OperProgressSummary":
                Instances.TaskQueueViewModel.AddLog(
                    LocalizationHelper.GetStringFormat(
                        "OperProgressSummaryLog",
                        msg.Details?["completed"] ?? 0,
                        msg.Details?["already_satisfied"] ?? 0,
                        msg.Details?["failed"] ?? 0,
                        msg.Details?["skipped"] ?? 0),
                    (int)(msg.Details?["failed"] ?? 0) == 0 ? UiLogColor.Success : UiLogColor.Warning);
                Instance.OnSummary();
                break;
        }
    }

    private static string ProcOperProgressTargetName(JToken? details)
    {
        var name = details?["name"]?.ToString() ?? string.Empty;
        return DataHelper.GetLocalizedCharacterName(name) ?? name;
    }

    // 与干员培养设置页的预览行（OperProgressTaskUserControlModel.DescribeAction）保持同一格式。
    private static string ProcOperProgressTargetDescription(JToken? details)
    {
        string action = details?["action"]?.ToString() ?? string.Empty;
        int target = details?["target"]?.Value<int>() ?? 0;
        return action switch {
            "elite" => LocalizationHelper.GetStringFormat("OperProgressEliteTarget", target),
            "skills" => LocalizationHelper.GetStringFormat("OperProgressSkillLevelTarget", target),
            "mastery" => LocalizationHelper.GetStringFormat("OperProgressMasteryTarget", details?["skill"]?.Value<int>() ?? 0, target),
            _ => action,
        };
    }

    public static List<GenericCombinedData<int>> EliteList => [
        new("---", 0),
        new(LocalizationHelper.GetStringFormat("OperProgressEliteTarget", 1), 1),
        new(LocalizationHelper.GetStringFormat("OperProgressEliteTarget", 2), 2),
    ];

    public static List<GenericCombinedData<int>> MainSkillLevelList => [
        new("---", 0),
        new("Lv. 2", 2),
        new("Lv. 3", 3),
        new("Lv. 4", 4),
        new("Lv. 5", 5),
        new("Lv. 6", 6),
        new("Lv. 7", 7),
    ];

    public static List<GenericCombinedData<int>> SpecializationSkillLevelList => [
        new("---", 0),
        new(LocalizationHelper.GetStringFormat("OperProgressMastery", 1), 1),
        new(LocalizationHelper.GetStringFormat("OperProgressMastery", 2), 2),
        new(LocalizationHelper.GetStringFormat("OperProgressMastery", 3), 3),
    ];
}
