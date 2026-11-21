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
using System.Collections.Generic;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.ViewModels.UI;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class SeasonalTaskUserControlModel : TaskSettingsViewModel, SeasonalTaskUserControlModel.ISerialize
{
    public static SeasonalTaskUserControlModel Instance { get; } = new();

    /// <summary>
    /// Gets or sets 通关循环局数，0 = 跳过任务。
    /// </summary>
    public int Rounds
    {
        get => GetTaskConfig<SeasonalTask>().Rounds;
        set => SetTaskConfig<SeasonalTask>(t => t.Rounds == value, t => t.Rounds = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
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

            // 轮数为 0 时跳过任务
            if (task.Rounds <= 0)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskQueue.Task.SeasonalTask.Skipped"), UiLogColor.Info);
                return (null, []);
            }

            Instances.TaskQueueViewModel.AddLogSection(task.NameOrTaskType);

            var (isSuccess, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                TaskType.SeasonalTask,
                new AsstSeasonalTask
                {
                    Rounds = task.Rounds,
                    Target = SettingsViewModel.StrongholdProtocolSettings.BadgeTarget,
                    StrategySettings = SettingsViewModel.StrongholdProtocolSettings.Settings,
                });

            return FromSingle((isSuccess, id));
        }
    }
}
