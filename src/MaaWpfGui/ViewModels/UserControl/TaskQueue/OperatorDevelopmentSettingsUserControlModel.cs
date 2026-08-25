// <copyright file="OperatorDevelopmentSettingsUserControlModel.cs" company="MaaAssistantArknights">
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

public class OperatorDevelopmentSettingsUserControlModel : TaskSettingsViewModel, OperatorDevelopmentSettingsUserControlModel.ISerialize
{
    private static readonly HashSet<string> AllowedFields = ["name", "elite", "skills", "skill", "skill_master"];

    static OperatorDevelopmentSettingsUserControlModel() => Instance = new();

    public static OperatorDevelopmentSettingsUserControlModel Instance { get; }

    private string _planJson = "[]";

    public string PlanJson
    {
        get => _planJson;
        set {
            if (!SetAndNotify(ref _planJson, value))
            {
                return;
            }

            IsCurrentTextValidated = value == GetTaskConfig<OperatorDevelopmentTask>().ValidatedPlanJson;
            SetTaskConfig<OperatorDevelopmentTask>(t => t.PlanJson == value, t => t.PlanJson = value);
            ValidationMessage = IsCurrentTextValidated
                ? LocalizationHelper.GetString("OperatorDevelopmentPlanValid")
                : LocalizationHelper.GetString("OperatorDevelopmentPlanPending");
        }
    }

    public bool IsCurrentTextValidated { get => field; private set => SetAndNotify(ref field, value); } = true;

    public string ValidationMessage { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

    public ObservableCollection<PlanPreview> PlanPreviewItems { get; } = [];

    public void ParsePlan()
    {
        try
        {
            var plans = ParseAndValidate(PlanJson);
            string normalized = plans.ToString(Formatting.Indented);
            SetTaskConfig<OperatorDevelopmentTask>(
                t => t.PlanJson == normalized && t.ValidatedPlanJson == normalized,
                t => {
                    t.PlanJson = normalized;
                    t.ValidatedPlanJson = normalized;
                });
            _planJson = normalized;
            NotifyOfPropertyChange(nameof(PlanJson));
            IsCurrentTextValidated = true;
            ValidationMessage = LocalizationHelper.GetStringFormat("OperatorDevelopmentPlanParsed", plans.Count);
            LoadPreview(plans);
        }
        catch (Exception ex) when (ex is JsonException or InvalidOperationException)
        {
            IsCurrentTextValidated = false;
            ValidationMessage = ex.Message;
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not OperatorDevelopmentTask task)
        {
            return;
        }

        _planJson = task.PlanJson;
        IsCurrentTextValidated = task.PlanJson == task.ValidatedPlanJson;
        ValidationMessage = IsCurrentTextValidated
            ? LocalizationHelper.GetString("OperatorDevelopmentPlanValid")
            : LocalizationHelper.GetString("OperatorDevelopmentPlanPending");
        try
        {
            LoadPreview(ParseAndValidate(task.ValidatedPlanJson));
        }
        catch (Exception)
        {
            PlanPreviewItems.Clear();
        }
        Refresh();
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) =>
        (this as ISerialize).Serialize(baseTask, taskId);

    internal static JArray ParseAndValidate(string json)
    {
        JToken root;
        try
        {
            root = JToken.Parse(json);
        }
        catch (JsonReaderException ex)
        {
            throw new InvalidOperationException(LocalizationHelper.GetStringFormat("OperatorDevelopmentJsonError", ex.LineNumber, ex.LinePosition, ex.Message), ex);
        }

        if (root is not JArray plans)
        {
            throw Error(-1, "$", "OperatorDevelopmentPlanMustBeArray");
        }

        for (int index = 0; index < plans.Count; ++index)
        {
            if (plans[index] is not JObject plan)
            {
                throw Error(index, "$", "OperatorDevelopmentPlanMustBeObject");
            }

            var unknown = plan.Properties().FirstOrDefault(property => !AllowedFields.Contains(property.Name));
            if (unknown is not null)
            {
                throw Error(index, unknown.Name, "OperatorDevelopmentUnknownField");
            }

            string name = ReadRequiredString(plan, index, "name");
            if (!DataHelper.Operators.Values.Any(character => character.Name == name))
            {
                throw Error(index, "name", "OperatorDevelopmentUnknownOperator");
            }
            plan["name"] = name;

            bool hasElite = plan.ContainsKey("elite");
            bool hasSkills = plan.ContainsKey("skills");
            bool hasSkill = plan.ContainsKey("skill");
            bool hasMastery = plan.ContainsKey("skill_master");
            int actionCount = Convert.ToInt32(hasElite) + Convert.ToInt32(hasSkills) + Convert.ToInt32(hasSkill || hasMastery);
            if (actionCount != 1)
            {
                throw Error(index, "$", "OperatorDevelopmentExactlyOneAction");
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
                    throw Error(index, hasSkill ? "skill_master" : "skill", "OperatorDevelopmentMasteryPairRequired");
                }
                ReadInteger(plan, index, "skill", 1, 3);
                ReadInteger(plan, index, "skill_master", 1, 3);
            }
        }

        return plans;
    }

    private static string ReadRequiredString(JObject plan, int index, string fieldName)
    {
        if (plan[fieldName]?.Type != JTokenType.String || string.IsNullOrWhiteSpace(plan.Value<string>(fieldName)))
        {
            throw Error(index, fieldName, "OperatorDevelopmentStringRequired");
        }
        return plan.Value<string>(fieldName)!.Trim();
    }

    private static int ReadInteger(JObject plan, int index, string fieldName, int minimum, int maximum)
    {
        if (plan[fieldName]?.Type != JTokenType.Integer)
        {
            throw Error(index, fieldName, "OperatorDevelopmentIntegerRequired");
        }
        int value;
        try
        {
            value = plan.Value<int>(fieldName);
        }
        catch (OverflowException ex)
        {
            throw Error(index, fieldName, "OperatorDevelopmentIntegerRequired", ex);
        }
        if (value < minimum || value > maximum)
        {
            throw new InvalidOperationException(LocalizationHelper.GetStringFormat("OperatorDevelopmentFieldRange", index, fieldName, minimum, maximum));
        }
        return value;
    }

    private static InvalidOperationException Error(int index, string fieldName, string localizationKey, Exception? innerException = null) =>
        new(LocalizationHelper.GetStringFormat(localizationKey, index, fieldName), innerException);

    private void LoadPreview(JArray plans)
    {
        PlanPreviewItems.Clear();
        for (int index = 0; index < plans.Count; ++index)
        {
            var plan = (JObject)plans[index]!;
            string name = plan.Value<string>("name")!;
            string target = plan.ContainsKey("elite")
                ? $"E{plan.Value<int>("elite")}"
                : plan.ContainsKey("skills")
                    ? LocalizationHelper.GetStringFormat("OperatorDevelopmentSkillLevelTarget", plan.Value<int>("skills"))
                    : LocalizationHelper.GetStringFormat("OperatorDevelopmentMasteryTarget", plan.Value<int>("skill"), plan.Value<int>("skill_master"));
            PlanPreviewItems.Add(new(index + 1, name, target));
        }
    }

    public sealed record PlanPreview(int Index, string Name, string Target);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not OperatorDevelopmentTask development)
            {
                return (null, []);
            }

            if (SettingsViewModel.GameSettings.ClientType is not ClientType.Official and not ClientType.Bilibili)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("OperatorDevelopmentUnsupportedClient"), UiLogColor.Error);
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

            var task = new AsstOperatorDevelopmentTask { Plans = plans };
            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.OperatorDevelopment, task)),
                _ => (null, []),
            };
        }
    }
}
