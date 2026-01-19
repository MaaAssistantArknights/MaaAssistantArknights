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
            SetAndNotify(ref _isEnable, value);
            ConfigFactory.CurrentConfig.TaskQueue[Index].IsEnable = value;
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
    /// Gets or sets 任务id，默认为0，添加后任务id应 > 0；执行后应置为0
    /// </summary>
    public int TaskId { get; set; }

    public int Status { get => field; set => SetAndNotify(ref field, value); }
}
