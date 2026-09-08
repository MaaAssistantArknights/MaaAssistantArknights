// <copyright file="AutoRaiseSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class AutoRaiseSettingsUserControlModel : TaskSettingsViewModel, AutoRaiseSettingsUserControlModel.ISerialize
{
    private const int MaxOperators = 5;

    private static readonly HashSet<string> AllowedFields = ["name", "elite", "skills", "skill", "skill_master"];

    static AutoRaiseSettingsUserControlModel() => Instance = new();

    public static AutoRaiseSettingsUserControlModel Instance { get; }

    private string _planJson = "[]";

    public string PlanJson
    {
        get => _planJson;
        set {
            if (!SetAndNotify(ref _planJson, value))
            {
                return;
            }

            IsCurrentTextValidated = value == GetTaskConfig<AutoRaiseTask>().ValidatedPlanJson;
            SetTaskConfig<AutoRaiseTask>(t => t.PlanJson == value, t => t.PlanJson = value);
            ValidationMessage = IsCurrentTextValidated
                ? LocalizationHelper.GetString("AutoRaisePlanValid")
                : LocalizationHelper.GetString("AutoRaisePlanPending");
        }
    }

    public bool IsCurrentTextValidated { get => field; private set => SetAndNotify(ref field, value); } = true;

    public string ValidationMessage { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

    public ObservableCollection<PlanPreview> PlanPreviewItems { get; } = [];

    /// <summary>可选择的干员名列表，按稀有度降序、名称升序排列，实时取自干员数据</summary>
    public IReadOnlyList<string> OperatorNames => DataHelper.Operators.Values
        .GroupBy(character => character.Name)
        .Select(group => group.OrderByDescending(character => character.Rarity).First())
        .OrderByDescending(character => character.Rarity)
        .ThenBy(character => character.Name)
        .Select(character => character.Name!)
        .ToList();

    private string _selectedOperator = string.Empty;

    public string SelectedOperator
    {
        get => _selectedOperator;
        set {
            if (SetAndNotify(ref _selectedOperator, value))
            {
                NotifyOfPropertyChange(nameof(CanAddOperator));
            }
        }
    }

    /// <summary>未满 5 名干员，或所选干员已在计划中（可直接进入编辑）时允许添加</summary>
    public bool CanAddOperator
    {
        get
        {
            JArray plans = GetValidatedPlans();
            return DistinctOperatorCount(plans) < MaxOperators || FindOperator(plans, _selectedOperator) >= 0;
        }
    }

    // —— 培养目标弹窗 ——
    public bool IsTargetPopupOpen { get => field; set => SetAndNotify(ref field, value); }

    public string PopupTitle { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

    public string PopupConfirmText { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

    public bool PopupHasSelection { get => field; private set => SetAndNotify(ref field, value); }

    public bool PopupSelectElite
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                UpdatePopupHasSelection();
            }
        }
    }

    public int PopupEliteTarget
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                PopupSelectElite = true;
            }
        }
    } = 2;

    public bool PopupSelectSkill
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                UpdatePopupHasSelection();
            }
        }
    }

    public int PopupSkillTarget
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                PopupSelectSkill = true;
            }
        }
    } = 7;

    public bool PopupSelectMastery
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                UpdatePopupHasSelection();
            }
        }
    }

    public int PopupMasterySkillIndex
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                PopupSelectMastery = true;
            }
        }
    } = 1;

    public int PopupMasteryTarget
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                PopupSelectMastery = true;
            }
        }
    } = 3;

    public IReadOnlyList<int> EliteOptions { get; } = [1, 2];

    public IReadOnlyList<int> SkillLevelOptions { get; } = [2, 3, 4, 5, 6, 7];

    public IReadOnlyList<int> MasteryTargetOptions { get; } = [1, 2, 3];

    public IReadOnlyList<int> MasterySkillOptions { get => field; private set => SetAndNotify(ref field, value); } = [1, 2, 3];

    private string _popupOperatorName = string.Empty;

    private int _editOperatorIndex = -1;

    public void ParsePlan()
    {
        try
        {
            ApplyPlans(ParseAndValidate(PlanJson));
        }
        catch (Exception ex) when (ex is JsonException or InvalidOperationException)
        {
            IsCurrentTextValidated = false;
            ValidationMessage = ex.Message;
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not AutoRaiseTask task)
        {
            return;
        }

        _planJson = task.PlanJson;
        IsCurrentTextValidated = task.PlanJson == task.ValidatedPlanJson;
        ValidationMessage = IsCurrentTextValidated
            ? LocalizationHelper.GetString("AutoRaisePlanValid")
            : LocalizationHelper.GetString("AutoRaisePlanPending");
        try
        {
            LoadPreview(ParseAndValidate(task.ValidatedPlanJson));
        }
        catch (Exception)
        {
            PlanPreviewItems.Clear();
            NotifyOfPropertyChange(nameof(CanAddOperator));
        }
        Refresh();
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) =>
        (this as ISerialize).Serialize(baseTask, taskId);

    public void OpenTargetPopup()
    {
        string name = SelectedOperator.Trim();
        if (name.Length == 0)
        {
            return;
        }

        JArray plans = GetValidatedPlans();
        int editIndex = FindOperator(plans, name);
        if (editIndex < 0 && DistinctOperatorCount(plans) >= MaxOperators)
        {
            return;
        }

        BeginTargetPopup(name, plans, editIndex);
    }

    public void EditOperator(PlanPreview item)
    {
        if (item is null)
        {
            return;
        }

        JArray plans = GetValidatedPlans();
        BeginTargetPopup(item.Name, plans, FindOperator(plans, item.Name));
    }

    public void RemoveOperator(PlanPreview item)
    {
        if (item is null)
        {
            return;
        }

        JArray plans = GetValidatedPlans();
        var remaining = new JArray();
        foreach (JObject plan in plans.Cast<JObject>())
        {
            if (plan.Value<string>("name") != item.Name)
            {
                remaining.Add(plan);
            }
        }

        if (remaining.Count == plans.Count)
        {
            return;
        }

        ApplyPlans(remaining);
    }

    public void CancelTargetPopup() => IsTargetPopupOpen = false;

    public void ConfirmTargetPopup()
    {
        string name = _popupOperatorName;
        IsTargetPopupOpen = false;
        if (name.Length == 0 || !PopupHasSelection)
        {
            return;
        }

        JArray plans = GetValidatedPlans();
        var remaining = new JArray();
        foreach (JObject plan in plans.Cast<JObject>())
        {
            if (plan.Value<string>("name") != name)
            {
                remaining.Add(plan);
            }
        }

        int insertAt = _editOperatorIndex >= 0 ? Math.Min(_editOperatorIndex, remaining.Count) : remaining.Count;
        foreach (JObject entry in BuildOperatorPlans(name))
        {
            remaining.Insert(insertAt++, entry);
        }

        ApplyPlans(remaining);
        SelectedOperator = string.Empty;
    }

    /// <summary>把计划写回配置并刷新文本框、校验状态与预览，与 ParsePlan 成功路径一致</summary>
    private void ApplyPlans(JArray plans)
    {
        string normalized = plans.ToString(Formatting.Indented);
        SetTaskConfig<AutoRaiseTask>(
            t => t.PlanJson == normalized && t.ValidatedPlanJson == normalized,
            t => {
                t.PlanJson = normalized;
                t.ValidatedPlanJson = normalized;
            });
        _planJson = normalized;
        NotifyOfPropertyChange(nameof(PlanJson));
        IsCurrentTextValidated = true;
        ValidationMessage = LocalizationHelper.GetStringFormat("AutoRaisePlanParsed", plans.Count);
        LoadPreview(plans);
    }

    /// <summary>构建器始终基于最后一次解析成功的计划操作，与运行时使用的计划一致</summary>
    private JArray GetValidatedPlans()
    {
        try
        {
            return ParseAndValidate(GetTaskConfig<AutoRaiseTask>().ValidatedPlanJson);
        }
        catch (Exception ex) when (ex is JsonException or InvalidOperationException)
        {
            return new JArray();
        }
    }

    private void BeginTargetPopup(string name, JArray plans, int editIndex)
    {
        _popupOperatorName = name;
        _editOperatorIndex = editIndex;
        PopupTitle = LocalizationHelper.GetStringFormat("AutoRaiseTargetTitle", name);
        PopupConfirmText = LocalizationHelper.GetString(editIndex >= 0 ? "AutoRaiseEdit" : "Confirm");
        MasterySkillOptions = GetMasterySkillOptions(name);

        PopupEliteTarget = 2;
        PopupSkillTarget = 7;
        PopupMasterySkillIndex = MasterySkillOptions[0];
        PopupMasteryTarget = 3;
        PopupSelectElite = PopupSelectSkill = PopupSelectMastery = false;
        if (editIndex >= 0)
        {
            foreach (JObject plan in plans.Cast<JObject>().Where(plan => plan.Value<string>("name") == name))
            {
                if (plan.ContainsKey("elite"))
                {
                    PopupEliteTarget = plan.Value<int>("elite");
                    PopupSelectElite = true;
                }
                else if (plan.ContainsKey("skills"))
                {
                    PopupSkillTarget = plan.Value<int>("skills");
                    PopupSelectSkill = true;
                }
                else
                {
                    int skillIndex = plan.Value<int>("skill");
                    PopupMasterySkillIndex = MasterySkillOptions.Contains(skillIndex) ? skillIndex : MasterySkillOptions[0];
                    PopupMasteryTarget = plan.Value<int>("skill_master");
                    PopupSelectMastery = true;
                }
            }
        }

        UpdatePopupHasSelection();
        IsTargetPopupOpen = true;
    }

    private IEnumerable<JObject> BuildOperatorPlans(string name)
    {
        if (PopupSelectElite)
        {
            yield return new JObject { ["name"] = name, ["elite"] = PopupEliteTarget };
        }

        if (PopupSelectSkill)
        {
            yield return new JObject { ["name"] = name, ["skills"] = PopupSkillTarget };
        }

        if (PopupSelectMastery)
        {
            yield return new JObject { ["name"] = name, ["skill"] = PopupMasterySkillIndex, ["skill_master"] = PopupMasteryTarget };
        }
    }

    private void UpdatePopupHasSelection() => PopupHasSelection = PopupSelectElite || PopupSelectSkill || PopupSelectMastery;

    /// <summary>专精可选技能序号按稀有度过滤，规则与 CopilotViewModel 一致：3 技能需 6 星（或阿米娅），2 技能需 4 星</summary>
    private static IReadOnlyList<int> GetMasterySkillOptions(string name)
    {
        var character = DataHelper.GetCharacterByNameOrAlias(name);
        int rarity = character?.Rarity ?? -1;
        int maxSkill = rarity >= 6 || character?.Id == "char_002_amiya" ? 3 : rarity >= 4 ? 2 : 1;
        return Enumerable.Range(1, Math.Clamp(maxSkill, 1, 3)).ToList();
    }

    private static int FindOperator(JArray plans, string name)
    {
        for (int index = 0; index < plans.Count; ++index)
        {
            if (((JObject)plans[index]!).Value<string>("name") == name)
            {
                return index;
            }
        }

        return -1;
    }

    private static int DistinctOperatorCount(JArray plans) =>
        plans.Cast<JObject>().Select(plan => plan.Value<string>("name")).Distinct().Count();

    internal static JArray ParseAndValidate(string json)
    {
        JToken root;
        try
        {
            root = JToken.Parse(json);
        }
        catch (JsonReaderException ex)
        {
            throw new InvalidOperationException(LocalizationHelper.GetStringFormat("AutoRaiseJsonError", ex.LineNumber, ex.LinePosition, ex.Message), ex);
        }

        if (root is not JArray plans)
        {
            throw Error(-1, "$", "AutoRaisePlanMustBeArray");
        }

        for (int index = 0; index < plans.Count; ++index)
        {
            if (plans[index] is not JObject plan)
            {
                throw Error(index, "$", "AutoRaisePlanMustBeObject");
            }

            var unknown = plan.Properties().FirstOrDefault(property => !AllowedFields.Contains(property.Name));
            if (unknown is not null)
            {
                throw Error(index, unknown.Name, "AutoRaiseUnknownField");
            }

            string name = ReadRequiredString(plan, index, "name");
            if (!DataHelper.Operators.Values.Any(character => character.Name == name))
            {
                throw Error(index, "name", "AutoRaiseUnknownOperator");
            }
            plan["name"] = name;

            bool hasElite = plan.ContainsKey("elite");
            bool hasSkills = plan.ContainsKey("skills");
            bool hasSkill = plan.ContainsKey("skill");
            bool hasMastery = plan.ContainsKey("skill_master");
            int actionCount = Convert.ToInt32(hasElite) + Convert.ToInt32(hasSkills) + Convert.ToInt32(hasSkill || hasMastery);
            if (actionCount != 1)
            {
                throw Error(index, "$", "AutoRaiseExactlyOneAction");
            }

            if (hasElite)
            {
                ReadInteger(plan, index, "elite", 1, 2);
            }
            else if (hasSkills)
            {
                ReadInteger(plan, index, "skills", 2, 7);
            }
            else
            {
                if (!hasSkill || !hasMastery)
                {
                    throw Error(index, hasSkill ? "skill_master" : "skill", "AutoRaiseMasteryPairRequired");
                }
                ReadInteger(plan, index, "skill", 1, 3);
                ReadInteger(plan, index, "skill_master", 1, 3);
            }
        }

        if (DistinctOperatorCount(plans) > MaxOperators)
        {
            throw new InvalidOperationException(LocalizationHelper.GetString("AutoRaiseOperatorLimit"));
        }

        return plans;
    }

    private static string ReadRequiredString(JObject plan, int index, string fieldName)
    {
        if (plan[fieldName]?.Type != JTokenType.String || string.IsNullOrWhiteSpace(plan.Value<string>(fieldName)))
        {
            throw Error(index, fieldName, "AutoRaiseStringRequired");
        }
        return plan.Value<string>(fieldName)!.Trim();
    }

    private static int ReadInteger(JObject plan, int index, string fieldName, int minimum, int maximum)
    {
        if (plan[fieldName]?.Type != JTokenType.Integer)
        {
            throw Error(index, fieldName, "AutoRaiseIntegerRequired");
        }
        int value;
        try
        {
            value = plan.Value<int>(fieldName);
        }
        catch (OverflowException ex)
        {
            throw Error(index, fieldName, "AutoRaiseIntegerRequired", ex);
        }
        if (value < minimum || value > maximum)
        {
            throw new InvalidOperationException(LocalizationHelper.GetStringFormat("AutoRaiseFieldRange", index, fieldName, minimum, maximum));
        }
        return value;
    }

    private static InvalidOperationException Error(int index, string fieldName, string localizationKey, Exception? innerException = null) =>
        new(LocalizationHelper.GetStringFormat(localizationKey, index, fieldName), innerException);

    private void LoadPreview(JArray plans)
    {
        PlanPreviewItems.Clear();
        int index = 0;
        foreach (var group in plans.Cast<JObject>().GroupBy(plan => plan.Value<string>("name")!))
        {
            string target = string.Join(LocalizationHelper.GetString("AutoRaiseTargetSeparator"), group.Select(DescribeAction));
            PlanPreviewItems.Add(new(++index, group.Key, target));
        }
        NotifyOfPropertyChange(nameof(CanAddOperator));
    }

    private static string DescribeAction(JObject plan) =>
        plan.ContainsKey("elite")
            ? $"E{plan.Value<int>("elite")}"
            : plan.ContainsKey("skills")
                ? LocalizationHelper.GetStringFormat("AutoRaiseSkillLevelTarget", plan.Value<int>("skills"))
                : LocalizationHelper.GetStringFormat("AutoRaiseMasteryTarget", plan.Value<int>("skill"), plan.Value<int>("skill_master"));

    public sealed record PlanPreview(int Index, string Name, string Target);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not AutoRaiseTask development)
            {
                return (null, []);
            }

            if (SettingsViewModel.GameSettings.ClientType is not ClientType.Official and not ClientType.Bilibili)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AutoRaiseUnsupportedClient"), UiLogColor.Error);
                return (false, []);
            }

            JArray plans;
            try
            {
                plans = ParseAndValidate(development.ValidatedPlanJson);
            }
            catch (Exception ex) when (ex is JsonException or InvalidOperationException)
            {
                Instances.TaskQueueViewModel.AddLog(ex.Message, UiLogColor.Error);
                return (false, []);
            }

            if (plans.Count == 0)
            {
                return (null, []);
            }

            var task = new AsstAutoRaiseTask { Plans = plans };
            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.AutoRaise, task)),
                _ => (null, []),
            };
        }
    }
}
