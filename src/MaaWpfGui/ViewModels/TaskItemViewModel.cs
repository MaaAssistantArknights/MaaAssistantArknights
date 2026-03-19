// <copyright file="TaskItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using Stylet;

namespace MaaWpfGui.ViewModels;

public class TaskItemViewModel : PropertyChangedBase, IDisposable
{
    public TaskItemViewModel(string name, bool? isCheckedWithNull = true)
    {
        _name = name;
        _isEnable = isCheckedWithNull;
        Instances.AsstProxy.OnTaskItemStatusChanged += OnTaskStatusChanged;
    }

    private string _name;

    public string Name
    {
        get => _name;
        set {
            SetAndNotify(ref _name, value);
            ConfigFactory.CurrentConfig.TaskQueue[Index].Name = value;
        }
    }

    private bool? _isEnable;

    public bool? IsEnable
    {
        get => _isEnable;
        set {
            if (!SetAndNotify(ref _isEnable, value))
            {
                return;
            }

            ConfigFactory.CurrentConfig.TaskQueue[Index].IsEnable = value;
            StatusDisplay = TaskItemStatus.Idle;
        }
    }

    public int Index { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether gets or sets whether the setting enabled.
    /// </summary>
    public bool EnableSetting
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            TaskSettingVisibilityInfo.Instance.Set(Index, value);
        }
    }

    /// <summary>
    /// Gets or sets 任务id，默认为[]，添加后任务id应 > 0；执行后应置为[]
    /// </summary>
    private List<int> _taskIds = [];

    public IReadOnlyList<int> TaskIds => _taskIds;

    public void SetTaskIds(IEnumerable<int> taskIds)
    {
        _taskIds = [.. taskIds];
        StatusList = [.. Enumerable.Repeat(TaskItemStatus.Idle, TaskIds.Count)];
    }

    private List<TaskItemStatus> StatusList { get; set; } = [];

    /// <summary>
    /// Gets or sets 上次状态, 可能和当前不一致
    /// </summary>
    public TaskItemStatus StatusDisplay { get => field; set => SetAndNotify(ref field, value); }

    private void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        if (taskId < 0)
        {
            return;
        }
        int index = _taskIds.IndexOf(taskId);
        if (index < 0)
        {
            return;
        }
        StatusList[index] = status;
        if (StatusList.Any(s => s == TaskItemStatus.Error))
        {
            StatusDisplay = TaskItemStatus.Error;
        }
        else if (StatusList.Any(s => s == TaskItemStatus.InProgress))
        {
            StatusDisplay = TaskItemStatus.InProgress;
        }
        else if (StatusList.All(s => s == TaskItemStatus.Completed))
        {
            StatusDisplay = TaskItemStatus.Completed;
        }
        else if (StatusList.Any(s => s == TaskItemStatus.Skipped))
        {
            StatusDisplay = TaskItemStatus.Skipped;
        }
        else
        {
            StatusDisplay = TaskItemStatus.Idle;
        }
    }

    void IDisposable.Dispose() => Instances.AsstProxy.OnTaskItemStatusChanged -= OnTaskStatusChanged;
}
