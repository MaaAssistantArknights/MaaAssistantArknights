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
using System.Text;
using System.Windows;
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
using Serilog;
using static MaaWpfGui.Configuration.Single.MaaTask.OperProgressTask;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class OperProgressTaskUserControlModel : TaskSettingsViewModel, OperProgressTaskUserControlModel.ISerialize
{
    private static readonly ILogger _logger = Log.ForContext<OperProgressTaskUserControlModel>();

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
            int elite = plan.Elite;
            int mainSkillLevel = plan.SkillLevel;
            var specializationLevel = plan.SkillMastery;
            return new OperProgressPlanItemViewModel(index, plan.Role, plan.Name, elite, mainSkillLevel, specializationLevel);
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
            var elite = item.IsEliteSelected ? item.Elite : 0;
            var mainSkillLevel = item.IsMainSkillLevelSelected ? item.MainSkillLevel : 0;
            var mastery = SkillMastery.Of(item.SpecializationSkillLevel[0], item.SpecializationSkillLevel[1], item.SpecializationSkillLevel[2]);
            return new Plan(item.Role, item.Name, elite, mainSkillLevel, mastery);
        }).ToList();
        SetTaskConfig<OperProgressTask>(t => t.Plans.SequenceEqual(list), t => t.Plans = list);
    }

    public record class OperItem(string Id, OperatorRole Role, string Name, string NameDisplay, int Rarity);

    /// <summary>可选择的干员名列表，按稀有度降序、名称升序排列，实时取自干员数据</summary>
    public List<GenericCombinedData<OperItem>> OperatorNames => [.. DataHelper.Operators.Values
        .Select(character => new OperItem(character.Id, character.Role, character.Name!, DataHelper.GetLocalizedCharacterName(character) ?? character.Name!, character.Rarity))
        .OrderByDescending(entry => entry.Rarity)
        .ThenBy(entry => entry.Name, StringComparer.CurrentCulture)
        .Select(oper => new GenericCombinedData<OperItem>($"{oper.NameDisplay}[{oper.Rarity}★]",  oper))];

    public OperItem? OperSelect { get; set => SetAndNotify(ref field, value); }

    public override void RefreshUI(BaseTask? baseTask)
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

    /// <summary>
    /// 完成后移除已完成的条目
    /// </summary>
    /// <param name="taskId">任务 ID</param>
    public void RemoveFinishedPlans(int taskId)
    {
        var task = GetConfigByTaskId<OperProgressTask>(taskId);
        if (task is null)
        {
            _logger.Error("RemoveFinishedPlans: Could not find task with ID {TaskId}", taskId);
            return;
        }

        var list = task.Plans.ToList();
        if (list.Count == 0)
        {
            return;
        }

        foreach (var plan in list)
        {
            if (plan.Elite == 0 && plan.SkillLevel == 0 && plan.SkillMastery.ToArray().All(x => x == 0))
            {
                task.Plans.Remove(plan);
            }
        }
        RefreshUI(TaskSettingVisibilityInfo.CurrentTask);
    }

    public void CollapseAll()
    {
        foreach (var item in PlanItems)
        {
            item.IsExpanded = false;
        }
    }

    public void ClearPlan()
    {
        PlanItems.Clear();
    }

    public void ParsePlan()
    {
        if (Clipboard.ContainsText())
        {
            var str = Clipboard.GetText().Trim();

            // {"role": "Warrior","name":"陈","elite":2,"skill_level": 7, "skill_mastery":[0,0,3]}
            try
            {
                var json = JArray.Parse(str);
                var list = GetTaskConfig<OperProgressTask>()?.Plans.ToList() ?? [];
                foreach (var item in json)
                {
                    var plan = ParsePlan(item as JObject);
                    if (plan.Role == OperatorRole.Unknown && DataHelper.Operators.FirstOrDefault(c => c.Value.Name == plan.Name) is { } character)
                    {
                        plan = plan with { Role = character.Value.Role };
                    }
                    if (plan.Role == OperatorRole.Unknown || string.IsNullOrEmpty(plan.Name))
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ParseFailed") + $"\nunknown oper: {plan.Role}-{plan.Name}", UiLogColor.Error);
                        return;
                    }
                    list.Add(new Plan(plan.Role, plan.Name, plan.Elite ?? 0, plan.MainSkillLevel ?? 0, plan.SkillMastery ?? SkillMastery.Of(0, 0, 0)));
                }
                SetTaskConfig<OperProgressTask>(t => t.Plans.SequenceEqual(list), t => t.Plans = list);
                RefreshUI(TaskSettingVisibilityInfo.CurrentTask);
            }
            catch (Exception ex)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ParseFailed") + $"\n{ex.Message}", UiLogColor.Error);
                return;
            }
        }
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

        PlanItems.Add(new OperProgressPlanItemViewModel(PlanItems.Count, OperSelect.Role, OperSelect.Name, 2, 7, SkillMastery.Of(3, 3, 3)) { IsExpanded = true });
    }

    /// <summary>
    /// 从计划中移除指定的干员条目
    /// </summary>
    /// <param name="item">要移除的干员条目</param>
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
        if (_isRefreshing || !OperProgressPlanItemViewModel.MainProperty.Contains(e.PropertyName))
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

    /// <summary>
    /// 刷新Index
    /// </summary>
    private void ReindexPlanItems()
    {
        for (int index = 0; index < PlanItems.Count; ++index)
        {
            PlanItems[index].Index = index;
        }
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

    private void ProcOperProgressMsg(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (type != AsstMsg.SubTaskExtraInfo || msg?.TaskChain != nameof(TaskType.OperProgress))
        {
            return;
        }
        switch (msg.What)
        {
            case "OperProgressSummary":
                var summary = msg.Details?.ToObject<ProgressSummary>();
                Instances.TaskQueueViewModel.AddLog(
                    LocalizationHelper.GetStringFormat(
                        "OperProgress.Summary",
                        summary?.Success ?? -1,
                        summary?.Failed ?? -1,
                        summary?.Skipped ?? -1),
                    (summary?.Failed ?? -1) > 0 ? UiLogColor.Warning : UiLogColor.Success);
                Instance.RemoveFinishedPlans(msg.TaskId);
                break;

            case "OperProgressDetail":
                {
                    var callback = ParsePlan(msg.Details);
                    var task = GetConfigByTaskId<OperProgressTask>(msg.TaskId);
                    var list = task?.Plans.ToList();
                    var plan = list?.FirstOrDefault(p => p.Role == callback.Role && p.Name == callback.Name) ?? task?.Plans.FirstOrDefault(p => p.Name == callback.Name);
                    if (callback is null || task is null || list is null || plan is null)
                    {
                        Instances.TaskQueueViewModel.AddLog("Could not find matching plan for OperProgressDetail", UiLogColor.Error);
                        break;
                    }

                    var index = list.IndexOf(plan);
                    bool isModified = false;
                    if (callback.Elite is int elite && elite >= plan.Elite)
                    {
                        isModified = true;
                        plan = plan with { Elite = 0 };
                    }
                    if (callback.MainSkillLevel is int mainSkill && mainSkill >= plan.SkillLevel)
                    {
                        isModified = true;
                        plan = plan with { SkillLevel = 0 };
                    }
                    if (callback.SkillMastery is SkillMastery callbackMastery)
                    {
                        int[] skillSpec = plan.SkillMastery.ToArray();
                        int[] callbackSkillSpec = callbackMastery.ToArray();
                        for (int i = 0; i < 3; ++i)
                        {
                            if (callbackSkillSpec[i] >= skillSpec[i])
                            {
                                skillSpec[i] = 0;
                            }
                        }
                        if (!skillSpec.SequenceEqual(plan.SkillMastery.ToArray()))
                        {
                            if (skillSpec.All(i => i == 0))
                            {
                                isModified = true; // 如果全部专精已完成, 则将技能等级重置为基础等级0
                                plan = plan with { SkillMastery = SkillMastery.Of(0, 0, 0) };
                            }
                            else
                            {
                                isModified = true;
                                plan = plan with { SkillMastery = SkillMastery.Of(skillSpec[0], skillSpec[1], skillSpec[2]) };
                            }
                        }
                    }
                    if (isModified)
                    {
                        list[index] = plan;
                        task.Plans = [.. list];
                    }
                    var status = msg.Details?["result"]?.ToString() ?? string.Empty;
                    var operName = DataHelper.GetLocalizedCharacterName(callback.Name) ?? callback.Name;
                    Instances.TaskQueueViewModel.AddLog(
                        LocalizationHelper.GetStringFormat(
                            "OperProgress.Detail",
                            list.IndexOf(plan),
                            operName,
                            LocalizationHelper.GetStringFormat(status)) + BuildTargetCallbackDescription(callback),
                        UiLogColor.Info);

                    break;
                }
        }
    }

    private static ProgressCallback ParsePlan(JObject? json)
    {
        OperatorRole role = Enum.TryParse<OperatorRole>(json?.Value<string>("role") ?? string.Empty, out var parsedRole) ? parsedRole : OperatorRole.Unknown;
        string name = json?["name"]?.ToString() ?? string.Empty;
        int? elite = json?.Value<int?>("elite");
        int? mainSkillLevel = json?.Value<int?>("skill_level");
        SkillMastery? skillMastery = null;
        var skillLevelObj = json?["skill_mastery"];
        if (skillLevelObj?.Type == JTokenType.Array)
        {
            var list = skillLevelObj.ToObject<int[]>() ?? [];
            if (list.Length < 3)
            {
                Array.Resize(ref list, 3);
            }
            skillMastery = SkillMastery.Of(list[0], list[1], list[2]);
        }

        return new ProgressCallback(role, name, elite, mainSkillLevel, skillMastery);
    }

    private record ProgressCallback(OperatorRole Role, string Name, int? Elite, int? MainSkillLevel, SkillMastery? SkillMastery);

    private record ProgressSummary(int Success, int Failed, int Skipped);

    private static string BuildTargetCallbackDescription(ProgressCallback callback)
    {
        var str = new StringBuilder();
        if (callback.Elite is int elite)
        {
            str.AppendLine();
            str.Append(LocalizationHelper.GetStringFormat("OperProgress.EliteTarget", elite));
        }
        if (callback.MainSkillLevel is int mainSkill)
        {
            str.AppendLine();
            str.Append(LocalizationHelper.GetStringFormat("OperProgress.SkillLevelTarget", mainSkill));
        }
        if (callback.SkillMastery is SkillMastery mastery)
        {
            foreach (var (index, level) in mastery.ToArray().Index())
            {
                if (level > 0)
                {
                    str.AppendLine();
                    str.Append(LocalizationHelper.GetStringFormat("OperProgress.MasteryTarget", index + 1, level));
                }
            }
        }
        return str.ToString();
    }

    public static List<GenericCombinedData<int>> EliteList => [
        new("---", 0),
        new("1", 1),
        new("2", 2),
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
