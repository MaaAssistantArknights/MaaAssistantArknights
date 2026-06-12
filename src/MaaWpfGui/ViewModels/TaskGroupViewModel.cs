// <copyright file="TaskGroupViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using Stylet;

namespace MaaWpfGui.ViewModels;

/// <summary>
/// ViewModel for a task group that contains multiple child tasks.
/// Supports expand/collapse via Expander in the UI.
/// </summary>
public class TaskGroupViewModel : PropertyChangedBase, IDisposable, ITaskQueueItemViewModel
{
    private readonly TaskGroup _taskGroup;

    public TaskGroupViewModel(TaskGroup taskGroup)
    {
        _taskGroup = taskGroup;
        _name = taskGroup.NameOrTaskType;
        _isExpanded = taskGroup.IsExpanded;
        Children = new ObservableCollection<TaskItemViewModel>();
        _isEnable = ComputeAggregatedIsEnable();
        foreach (var child in taskGroup.Children)
        {
            var childVm = new TaskItemViewModel(child.NameOrTaskType, child.IsEnable, taskReference: child)
            {
                Index = Children.Count,
            };
            childVm.PropertyChanged += OnChildPropertyChanged;
            Children.Add(childVm);
        }

        Instances.AsstProxy.OnTaskStatusChanged += OnTaskStatusChanged;
    }

    private string _name;

    public string Name
    {
        get => _name;
        set {
            if (!SetAndNotify(ref _name, value))
            {
                return;
            }

            _taskGroup.Name = value;
        }
    }

    private bool _isExpanded;

    /// <summary>
    /// Gets or sets a value indicating whether the group is expanded in the UI.
    /// </summary>
    public bool IsExpanded
    {
        get => _isExpanded;
        set {
            if (!SetAndNotify(ref _isExpanded, value))
            {
                return;
            }

            _taskGroup.IsExpanded = value;
        }
    }

    private bool? _isEnable;

    /// <summary>
    /// Gets or sets the aggregated IsEnable state.
    /// true = all children enabled, false = all disabled, null = mixed.
    /// When set, propagates the value to all children.
    /// </summary>
    public bool? IsEnable
    {
        get => _isEnable;
        set {
            if (!SetAndNotify(ref _isEnable, value))
            {
                return;
            }

            if (value.HasValue)
            {
                foreach (var child in Children)
                {
                    child.IsEnable = value.Value;
                }
            }

            _taskGroup.IsEnable = value;
        }
    }

    /// <summary>
    /// Gets the children ViewModels displayed inside the group.
    /// </summary>
    public ObservableCollection<TaskItemViewModel> Children { get; }

    public int Index { get => field; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets or sets a value indicating whether the settings panel is enabled for this group.
    /// </summary>
    public bool EnableSetting
    {
        get => field;
        set {
            SetAndNotify(ref field, value);
            if (value)
            {
                // Groups don't have their own settings panel; select the first child instead
                var firstChild = Children.FirstOrDefault();
                if (firstChild != null)
                {
                    firstChild.EnableSetting = true;
                }
            }
        }
    }

    /// <summary>
    /// Gets or sets the aggregated status display from children.
    /// </summary>
    public TaskItemStatus StatusDisplay
    {
        get => field;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets an empty list — groups do not own task IDs directly; children manage their own.
    /// </summary>
    public IReadOnlyList<int> TaskIds => [];

    /// <summary>
    /// No-op — groups do not own task IDs directly; children manage their own.
    /// </summary>
    public void SetTaskIds(IEnumerable<int> taskIds)
    {
    }

    /// <summary>
    /// Recalculates the aggregated IsEnable state based on children's states.
    /// </summary>
    public void UpdateAggregatedIsEnable()
    {
        IsEnable = ComputeAggregatedIsEnable();
    }

    private bool? ComputeAggregatedIsEnable()
    {
        if (Children.Count == 0)
        {
            return true;
        }

        bool allOn = Children.All(c => c.IsEnable == true);
        bool allOff = Children.All(c => c.IsEnable == false);

        if (allOn)
        {
            return true;
        }

        if (allOff)
        {
            return false;
        }

        return null; // mixed
    }

    internal void OnChildPropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(TaskItemViewModel.IsEnable))
        {
            // Recalculate aggregated IsEnable when a child's IsEnable changes
            var newAggregated = ComputeAggregatedIsEnable();
            if (newAggregated != _isEnable)
            {
                // Directly set the field to avoid re-propagation loop
                _isEnable = newAggregated;
                NotifyOfPropertyChange(nameof(IsEnable));
                _taskGroup.IsEnable = newAggregated;
            }
        }
    }

    private void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        // Aggregate status: Error > InProgress > Completed > Skipped > Idle
        bool hasError = Children.Any(c => c.StatusDisplay == TaskItemStatus.Error);
        bool hasInProgress = Children.Any(c => c.StatusDisplay == TaskItemStatus.InProgress);
        bool allCompleted = Children.Count > 0 && Children.All(c => c.StatusDisplay == TaskItemStatus.Completed);

        if (hasError)
        {
            StatusDisplay = TaskItemStatus.Error;
        }
        else if (hasInProgress)
        {
            StatusDisplay = TaskItemStatus.InProgress;
        }
        else if (allCompleted)
        {
            StatusDisplay = TaskItemStatus.Completed;
        }
        else
        {
            StatusDisplay = TaskItemStatus.Idle;
        }
    }

    /// <summary>
    /// Adds a child ViewModel to this group, linked to the given BaseTask.
    /// </summary>
    public void AddChild(BaseTask task)
    {
        _taskGroup.Children.Add(task);
        var childVm = new TaskItemViewModel(task.NameOrTaskType, task.IsEnable, taskReference: task)
        {
            Index = Children.Count,
        };
        childVm.PropertyChanged += OnChildPropertyChanged;
        Children.Add(childVm);
        UpdateAggregatedIsEnable();
    }

    /// <summary>
    /// Removes a child ViewModel at the given index and returns the associated BaseTask.
    /// </summary>
    public BaseTask? RemoveChildAt(int index)
    {
        if (index < 0 || index >= Children.Count || index >= _taskGroup.Children.Count)
        {
            return null;
        }

        var task = _taskGroup.Children[index];
        var childVm = Children[index];
        childVm.PropertyChanged -= OnChildPropertyChanged;
        childVm.Dispose();
        Children.RemoveAt(index);
        _taskGroup.Children.RemoveAt(index);

        // Update indices
        for (int i = index; i < Children.Count; i++)
        {
            Children[i].Index = i;
        }

        UpdateAggregatedIsEnable();
        return task;
    }

    public void Dispose()
    {
        Instances.AsstProxy.OnTaskStatusChanged -= OnTaskStatusChanged;
        foreach (var child in Children)
        {
            child.PropertyChanged -= OnChildPropertyChanged;
            child.Dispose();
        }

        Children.Clear();
    }
}
