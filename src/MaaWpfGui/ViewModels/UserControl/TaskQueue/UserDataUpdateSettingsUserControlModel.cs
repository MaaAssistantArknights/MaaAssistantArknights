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
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
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
            bool operBoxSyncedWithoutTask = false;
            if (operBoxTriggerDue)
            {
                if (SettingsViewModel.ThirdPartyServiceSettings.EnableOperBoxYituliuApi)
                {
                    if (string.IsNullOrWhiteSpace(SettingsViewModel.ThirdPartyServiceSettings.YituliuOpenApiToken))
                    {
                        // 错误才打任务分区标题提供上下文，正常执行不需要
                        Instances.TaskQueueViewModel.AddLogSection(baseTask.NameOrTaskType);
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("YituliuTokenEmpty"), UiLogColor.Error);
                        return (false, []);
                    }

                    // 一图流 OpenAPI 模式：不进 core 队列，后台直接拉取，没有 core 任务 id，完成后自行更新条目状态
                    _ = SyncOperBoxFromYituliuApiAsync(baseTask);
                    operBoxSyncedWithoutTask = true;
                }
                else
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

            return ids.Count > 0 || operBoxSyncedWithoutTask ? (true, ids) : (null, []);
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
    /// 从一图流拉取干员数据并在完成后写任务日志、更新任务条目状态。
    /// 拉取是后台并行的，只在结束时输出一条日志，避免与队列启动日志交错。
    /// </summary>
    /// <param name="baseTask">发起拉取的任务，用于定位任务条目</param>
    /// <returns>Task</returns>
    private static async Task SyncOperBoxFromYituliuApiAsync(BaseTask baseTask)
    {
        var success = await Instances.ToolboxViewModel.StartOperBoxFromYituliuApiAsync();
        if (success)
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("YituliuOperBoxCompleted"), UiLogColor.Info, splitMode: TaskQueueViewModel.LogCardSplitMode.Both);
        }

        var index = ConfigFactory.CurrentConfig.TaskQueue.IndexOf(baseTask);
        if (index >= 0)
        {
            Instances.TaskQueueViewModel.TaskItemViewModels.ElementAtOrDefault(index)?.StatusDisplay = success ? TaskItemStatus.Completed : TaskItemStatus.Error;
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
