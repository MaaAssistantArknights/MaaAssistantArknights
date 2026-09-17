// <copyright file="TaskGroupItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Configuration.Single;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels;

public class TaskGroupItemViewModel(TaskGroup group, TaskQueueViewModel owner) : PropertyChangedBase
{
    public TaskGroup Group { get; } = group;

    public bool IsGroup => true;

    public bool IsGrouped => false;

    public bool EnableSetting { get; set; }

    public string Name
    {
        get => string.IsNullOrWhiteSpace(Group.Name) ? LocalizationHelper.GetString("TaskGroup") : Group.Name;
        set {
            Group.Name = value;
            NotifyOfPropertyChange();
        }
    }

    public bool? IsEnable
    {
        get {
            var tasks = owner.GetGroupTaskItems(Group.Id);
            if (tasks.Count == 0)
            {
                return Group.IsEnable;
            }

            var value = tasks[0].IsEnable;
            return tasks.All(task => task.IsEnable == value) ? value : null;
        }
        set => owner.SetGroupEnabled(Group.Id, value);
    }

    public bool IsExpanded
    {
        get => Group.IsExpanded;
        set {
            if (Group.IsExpanded == value)
            {
                return;
            }

            Group.IsExpanded = value;
            NotifyOfPropertyChange();
            owner.RefreshTaskQueueItems();
        }
    }

    public TaskItemStatus StatusDisplay
    {
        get {
            var statuses = owner.GetGroupTaskItems(Group.Id).Select(task => task.StatusDisplay).ToList();
            if (statuses.Any(status => status == TaskItemStatus.Error))
            {
                return TaskItemStatus.Error;
            }

            if (statuses.Any(status => status == TaskItemStatus.InProgress))
            {
                return TaskItemStatus.InProgress;
            }

            if (statuses.Count > 0 && statuses.All(status => status == TaskItemStatus.Completed))
            {
                return TaskItemStatus.Completed;
            }

            if (statuses.Any(status => status == TaskItemStatus.Skipped))
            {
                return TaskItemStatus.Skipped;
            }

            return TaskItemStatus.Idle;
        }
    }

    public void RefreshState()
    {
        NotifyOfPropertyChange(nameof(IsEnable));
        NotifyOfPropertyChange(nameof(StatusDisplay));
    }
}
