// <copyright file="CustomSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class CustomSettingsUserControlModel : TaskViewModel
{
    static CustomSettingsUserControlModel()
    {
        Instance = new();
    }

    public static CustomSettingsUserControlModel Instance { get; }

    private string _taskName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.DebugTaskName, string.Empty);

    public string TaskName
    {
        get => _taskName;
        set
        {
            value = value.Replace("，", ",").Replace("；", ";");
            SetAndNotify(ref _taskName, value);
            OnPropertyChanged(nameof(FormattedTaskNames));
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is CustomTask)
        {
            Refresh();
        }
    }

    public void SaveTaskName()
    {
        ConfigurationHelper.SetGlobalValue(ConfigurationKeys.DebugTaskName, TaskName);
    }

    public string FormattedTaskNames
    {
        get
        {
            if (string.IsNullOrWhiteSpace(TaskName))
            {
                return string.Empty;
            }

            var taskGroups = TaskName.Split(';', StringSplitOptions.RemoveEmptyEntries)
                .Select(group =>
                    "[" + string.Join(", ",
                        group.Split(',', StringSplitOptions.RemoveEmptyEntries)
                            .Select(t => $"\"{t.Trim()}\"")) + "]");

            return string.Join("," + Environment.NewLine, taskGroups);
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstCustomTask()
        {
            CustomTasks = [.. TaskName.Split(',', StringSplitOptions.RemoveEmptyEntries).Select(task => task.Trim())],
        };
        return task.Serialize();
    }

    public override bool? SerializeTask(BaseTask baseTask, int? taskId = null)
    {
        if (baseTask is not CustomTask custom)
        {
            return null;
        }

        var task = new AsstCustomTask()
        {
           // CustomTasks = [.. custom.TaskName.Split(',', StringSplitOptions.RemoveEmptyEntries).Select(task => task.Trim())],
        };
        if (taskId is int id)
        {
            return Instances.AsstProxy.AsstSetTaskParamsEncoded(id, task);
        }
        else
        {
            return Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Custom, task);
        }
    }
}
