// <copyright file="SideStorySettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class SideStorySettingsUserControlModel : TaskSettingsViewModel, SideStorySettingsUserControlModel.ISerialize
{
    static SideStorySettingsUserControlModel()
    {
        Instance = new();
    }

    public static SideStorySettingsUserControlModel Instance { get; }

    public bool TaskAward { get => field; set => SetAndNotify(ref field, value); } = true;

    public bool StageReopen { get => field; set => SetAndNotify(ref field, value); } = true;

    public ObservableCollection<StageItem> StageList { get; set; } = [.. Enumerable.Range(1, 10).Select(i => new StageItem(i))];

    public bool IsSideStoryReopenEnable { get => field; set => SetAndNotify(ref field, value); }

    public string ActivityInfo { get => field; private set => SetAndNotify(ref field, value); } = string.Empty;

    public override void RefreshUI(BaseTask baseTask)
    {
        if (Instances.StageManager.ActivityList.TryGetValue("SSReopen", out var activity))
        {
            IsSideStoryReopenEnable = true;
            ActivityInfo = $"{LocalizationHelper.GetStringFormat("SideStoryReopen", activity.Info.StageName)}";
        }
        else
        {
            ActivityInfo = LocalizationHelper.GetString("NoActivity");
            IsSideStoryReopenEnable = false;
        }
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    public class StageItem(int stage) : PropertyChangedBase
    {
        private int Stage = stage;

        public string Display => "MT-" + Stage.ToString();

        public bool Value { get => field; set => SetAndNotify(ref field, value); } = true;
    }

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not CustomTask custom)
            {
                return (null, []);
            }

            var task = new AsstCustomTask() { };
            return taskId switch {
                int id when id > 0 => (Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task), [id]),
                null => FromSingle(Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Custom, task)),
                _ => (null, []),
            };
        }
    }
}
