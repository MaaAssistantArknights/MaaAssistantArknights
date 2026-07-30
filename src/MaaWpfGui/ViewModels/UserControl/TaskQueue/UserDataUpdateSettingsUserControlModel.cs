// <copyright file="UserDataUpdateSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class UserDataUpdateSettingsUserControlModel : TaskSettingsViewModel, UserDataUpdateSettingsUserControlModel.ISerialize
{
    static UserDataUpdateSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public static UserDataUpdateSettingsUserControlModel Instance { get; }

    public bool UpdateOperBox
    {
        get => GetTaskConfig<UserDataUpdateTask>().UpdateOperBox;
        set => SetTaskConfig<UserDataUpdateTask>(t => t.UpdateOperBox == value, t => t.UpdateOperBox = value);
    }

    public bool UpdateDepot
    {
        get => GetTaskConfig<UserDataUpdateTask>().UpdateDepot;
        set => SetTaskConfig<UserDataUpdateTask>(t => t.UpdateDepot == value, t => t.UpdateDepot = value);
    }

    public UserDataUpdateTriggerInterval TriggerInterval
    {
        get => GetTaskConfig<UserDataUpdateTask>().TriggerInterval;
        set => SetTaskConfig<UserDataUpdateTask>(t => t.TriggerInterval == value, t => t.TriggerInterval = value);
    }

    public LocalizedObservableList<UserDataUpdateTriggerInterval> TriggerIntervalList { get; } = new(
        (UserDataUpdateTriggerInterval.EveryTime, "EveryTime"),
        (UserDataUpdateTriggerInterval.Daily, "Daily"),
        (UserDataUpdateTriggerInterval.Weekly, "Weekly"));

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is UserDataUpdateTask)
        {
            Refresh();
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not UserDataUpdateTask updateTask)
            {
                return (null, []);
            }

            if (taskId is int id && id > 0)
            {
                Instances.TaskQueueViewModel.AddLog("Unable to modify existing UserDataUpdateTask.", UiLogColor.Error);
                return (null, []);
            }

            if (!updateTask.IsTriggered)
            {
                return (null, []);
            }

            bool operBoxTriggerDue = updateTask.UpdateOperBox && IsTriggerDue(Instances.ToolboxViewModel.LastOperBoxSyncTime, updateTask.TriggerInterval);
            bool depotTriggerDue = updateTask.UpdateDepot && IsTriggerDue(Instances.ToolboxViewModel.LastDepotSyncTime, updateTask.TriggerInterval);

            if (!operBoxTriggerDue && !depotTriggerDue)
            {
                return (null, []);
            }

            List<int> ids = [];
            if (operBoxTriggerDue)
            {
                bool operBoxRet = Instances.ToolboxViewModel.StartOperBoxRecognitionTask(startImmediately: false);
                if (!operBoxRet)
                {
                    return (false, []);
                }

                int operBoxTaskId = Instances.AsstProxy.TasksStatus.Last().Key;
                Instances.ToolboxViewModel.MarkOperBoxRecognitionDataForReset(operBoxTaskId);
                ids.Add(operBoxTaskId);
            }

            if (depotTriggerDue)
            {
                var (result, depotTaskId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Depot, (AsstTaskType.Depot, null));
                if (!result)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanUpdateDepotFailed"), UiLogColor.Error);
                    return (false, []);
                }
                Instances.ToolboxViewModel.MarkDepotRecognitionSyncTimeForReset(depotTaskId);
                ids.Add(depotTaskId);
            }

            if (operBoxTriggerDue && depotTriggerDue)
            {
                AchievementTrackerHelper.Instance.Unlock(AchievementIds.DoubleSync);
            }

            return ids.Count > 0 ? (true, ids) : (null, []);
        }

        private static bool IsTriggerDue(DateTimeOffset? lastSyncTime, UserDataUpdateTriggerInterval triggerInterval)
        {
            if (triggerInterval == UserDataUpdateTriggerInterval.EveryTime)
            {
                return true;
            }

            if (!lastSyncTime.HasValue)
            {
                return true;
            }

            var now = DateTimeOffset.UtcNow.ToYjDateTime().Date;
            var lastDate = lastSyncTime.Value.ToYjDateTime().Date;

            return triggerInterval switch {
                UserDataUpdateTriggerInterval.Daily => now > lastDate,
                UserDataUpdateTriggerInterval.Weekly => ISOWeek.GetYear(now) != ISOWeek.GetYear(lastDate) || ISOWeek.GetWeekOfYear(now) != ISOWeek.GetWeekOfYear(lastDate),
                _ => true,
            };
        }
    }

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        TriggerIntervalList.RefreshLocalization();
    }
}
