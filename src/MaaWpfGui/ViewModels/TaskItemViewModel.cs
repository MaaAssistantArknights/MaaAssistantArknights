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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Models;
using Stylet;

namespace MaaWpfGui.ViewModels;

public class TaskItemViewModel : PropertyChangedBase
{
    public TaskItemViewModel(string name, bool? isCheckedWithNull = true)
    {
        _name = name;
        _isEnable = isCheckedWithNull;
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
            Status = 0;
        }
    }

    private int _index;

    public int Index
    {
        get => _index;
        set => SetAndNotify(ref _index, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether gets or sets whether the setting enabled.
    /// </summary>
    private bool _enableSetting;

    public bool EnableSetting
    {
        get => _enableSetting;
        set {
            SetAndNotify(ref _enableSetting, value);
            TaskSettingVisibilityInfo.Instance.Set(Index, value);
        }
    }

    /// <summary>
    /// Gets or sets 任务id，默认为0，添加后任务id应 > 0；执行后应置为0
    /// </summary>
    private int _taskId;

    public int TaskId
    {
        get => _taskId;
        set => SetTaskIds(value > 0 ? [value] : []);
    }

    private IReadOnlyList<int> _taskIds = [];

    public IReadOnlyList<int> TaskIds
    {
        get => _taskIds;
        private set => SetAndNotify(ref _taskIds, value);
    }

    public void SetTaskIds(IEnumerable<int> taskIds)
    {
        var ids = taskIds.Where(id => id > 0).Distinct().ToArray();
        TaskIds = ids;
        SetAndNotify(ref _taskId, ids.LastOrDefault(), nameof(TaskId));
    }

    public bool ContainsTaskId(int taskId) => taskId > 0 && TaskIds.Contains(taskId);

    private int _status;

    public int Status
    {
        get => _status;
        set => SetAndNotify(ref _status, value);
    }
}
