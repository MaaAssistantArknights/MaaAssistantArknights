// <copyright file="SeasonalTaskUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class SeasonalTaskUserControlModel : TaskSettingsViewModel, SeasonalTaskUserControlModel.ISerialize
{
    static SeasonalTaskUserControlModel()
    {
        Instance = new();
    }

    public static SeasonalTaskUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets 卫戍协议（极速模拟）轮数，从进入对局到返回主界面记一轮，打满后结束；0 表示跳过本任务。
    /// </summary>
    public int SpaRounds
    {
        get => field;
        set {
            if (SetAndNotify(ref field, value))
            {
                SetTaskConfig<SeasonalTask>(t => t.SpaRounds == value, t => t.SpaRounds = value);
            }
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not SeasonalTask task)
        {
            return;
        }

        IsRefreshingUI = true;
        SpaRounds = task.SpaRounds;
        IsRefreshingUI = false;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not SeasonalTask task || taskId > 0)
            {
                return (null, []);
            }

            // 未设置轮数时跳过任务
            if (task.SpaRounds <= 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskQueue.SeasonalTask.Skipped"), UiLogColor.Info);
                return (null, []);
            }

            Instances.TaskQueueViewModel.AddLogSection(task.NameOrTaskType);

            var (isSuccess, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                TaskType.SeasonalTask,
                new AsstSeasonalTask { SpaRounds = task.SpaRounds });
            if (!isSuccess)
            {
                return (false, []);
            }

            return (true, [id]);
        }
    }
}
