// <copyright file="TaskQueueDropHandler.cs" company="MaaAssistantArknights">
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
using System.Collections;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using GongSolutions.Wpf.DragDrop;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels;
using Serilog;

namespace MaaWpfGui.Models;

/// <summary>
/// Custom <see cref="IDropTarget"/> for the task queue ListBox and group ItemsControls.
/// Supports:
/// - Dragging a top-level task INTO a task group
/// - Dragging a child task OUT OF a task group (back to top level)
/// - Dragging a child task BETWEEN groups
/// - Standard reorder within the same list
/// </summary>
public class TaskQueueDropHandler : DefaultDropHandler
{
    private static readonly ILogger _logger = Log.ForContext<TaskQueueDropHandler>();

    /// <summary>
    /// Called continuously while the user drags over a drop target.
    /// Always shows "move" feedback for task queue items.
    /// </summary>
    public override void DragOver(IDropInfo dropInfo)
    {
        if (dropInfo.Data is ITaskQueueItemViewModel or TaskGroupViewModel or TaskItemViewModel)
        {
            dropInfo.Effects = DragDropEffects.Move;
            dropInfo.DropTargetAdorner = DropTargetAdorners.Insert;
        }
    }

    /// <summary>
    /// Called when the user releases the drag. Handles cross-level moves (into/out of groups)
    /// and falls back to the default handler for same-level reorder.
    /// </summary>
    public override void Drop(IDropInfo dropInfo)
    {
        var sourceItem = dropInfo.Data;
        var targetItem = dropInfo.TargetItem;
        var sourceCollection = dropInfo.DragInfo.SourceCollection;
        var targetCollection = dropInfo.TargetCollection;

        if (sourceItem == null)
        {
            return;
        }

        // ─────────────────────────────────────────────────
        // Scenario A: Drag a top-level TaskItemViewModel ONTO a TaskGroupViewModel
        //   → move the task INTO the group
        // ─────────────────────────────────────────────────
        if (sourceItem is TaskItemViewModel sourceTask
            && targetItem is TaskGroupViewModel targetGroup
            && sourceCollection is ObservableCollection<ITaskQueueItemViewModel>)
        {
            HandleMoveIntoGroup(sourceTask, targetGroup);
            return;
        }

        // ─────────────────────────────────────────────────
        // Scenario B: Drag a TaskItemViewModel from a group's ItemsControl
        // to the top-level ListBox
        //   → move the task OUT of the group
        // ─────────────────────────────────────────────────
        if (sourceItem is TaskItemViewModel childTask
            && sourceCollection is ObservableCollection<TaskItemViewModel> sourceChildrenCollection
            && targetCollection is ObservableCollection<ITaskQueueItemViewModel>)
        {
            HandleMoveOutOfGroup(childTask, sourceChildrenCollection, dropInfo.InsertIndex);
            return;
        }

        // ─────────────────────────────────────────────────
        // Scenario C: Drag a TaskItemViewModel between two different groups
        //   → move from source group to target group
        // ─────────────────────────────────────────────────
        if (sourceItem is TaskItemViewModel crossTask
            && sourceCollection is ObservableCollection<TaskItemViewModel> srcChildren
            && targetCollection is ObservableCollection<TaskItemViewModel> dstChildren
            && !ReferenceEquals(srcChildren, dstChildren))
        {
            HandleMoveBetweenGroups(crossTask, srcChildren, dstChildren, dropInfo.InsertIndex);
            return;
        }

        // ─────────────────────────────────────────────────
        // Scenario D (fallback): Same-level reorder
        //   → let DefaultDropHandler handle it
        // ─────────────────────────────────────────────────
        base.Drop(dropInfo);

        // After default reorder, refresh indices
        if (targetCollection is ObservableCollection<ITaskQueueItemViewModel> topLevel)
        {
            RefreshTopLevelIndices();
        }
        else if (targetCollection is ObservableCollection<TaskItemViewModel> groupChildren)
        {
            RefreshGroupChildIndices(groupChildren);
        }
    }

    /// <summary>
    /// Moves a top-level task INTO a task group.
    /// </summary>
    private static void HandleMoveIntoGroup(TaskItemViewModel sourceTask, TaskGroupViewModel targetGroup)
    {
        int sourceIndex = sourceTask.Index;
        if (sourceIndex < 0 || sourceIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            return;
        }

        var task = ConfigFactory.CurrentConfig.TaskQueue[sourceIndex];
        if (task is TaskGroup)
        {
            _logger.Warning("Cannot drag a TaskGroup into another group.");
            return;
        }

        _logger.Information("Moving task {TaskType} into group {GroupName}",
            task.TaskType, targetGroup.Name);

        // Remove from top-level Config
        ConfigFactory.CurrentConfig.TaskQueue.RemoveAt(sourceIndex);

        // Remove from ViewModel list
        Instances.TaskQueueViewModel.TaskItemViewModels.Remove(sourceTask);

        // Dispose the old event subscription (the AddChild will create a new ViewModel)
        sourceTask.Dispose();

        // Add to the group (both data model and ViewModel)
        targetGroup.AddChild(task);

        // Refresh indices for remaining top-level items
        RefreshTopLevelIndices();
    }

    /// <summary>
    /// Moves a child task OUT OF a group, back to the top-level task queue.
    /// </summary>
    private static void HandleMoveOutOfGroup(
        TaskItemViewModel childTask,
        IList sourceChildrenCollection,
        int insertIndex)
    {
        var parentGroup = FindParentGroup(sourceChildrenCollection);
        if (parentGroup == null)
        {
            _logger.Warning("Could not find parent group for child task.");
            return;
        }

        int childIndex = childTask.Index;
        int groupQueueIndex = parentGroup.Index;

        if (groupQueueIndex < 0 || groupQueueIndex >= ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            return;
        }

        var group = ConfigFactory.CurrentConfig.TaskQueue[groupQueueIndex] as TaskGroup;
        if (group == null)
        {
            return;
        }

        if (childIndex < 0 || childIndex >= group.Children.Count || childIndex >= parentGroup.Children.Count)
        {
            return;
        }

        _logger.Information("Moving child task {TaskType} out of group {GroupName}",
            group.Children[childIndex].TaskType, parentGroup.Name);

        var task = parentGroup.RemoveChildAt(childIndex);
        if (task == null)
        {
            return;
        }

        // Calculate the insert position in the top-level TaskQueue
        // Insert after the group, but respect the insertIndex offset from the drag operation
        int queueInsertIndex = groupQueueIndex + 1;

        // Apply the drop position offset: insertIndex is within the group's ItemsControl,
        // so we clamp the target position
        if (insertIndex >= 0)
        {
            queueInsertIndex += insertIndex;
        }

        // Clamp
        if (queueInsertIndex > ConfigFactory.CurrentConfig.TaskQueue.Count)
        {
            queueInsertIndex = ConfigFactory.CurrentConfig.TaskQueue.Count;
        }

        // Insert into Config
        ConfigFactory.CurrentConfig.TaskQueue.Insert(queueInsertIndex, task);

        // Create ViewModel with taskReference for the new top-level item
        var newVm = new TaskItemViewModel(task.NameOrTaskType, task.IsEnable, taskReference: task)
        {
            Index = queueInsertIndex,
        };

        Instances.TaskQueueViewModel.TaskItemViewModels.Insert(queueInsertIndex, newVm);

        // Refresh indices
        RefreshTopLevelIndices();
    }

    /// <summary>
    /// Moves a child task from one group to another.
    /// </summary>
    private static void HandleMoveBetweenGroups(
        TaskItemViewModel childTask,
        IList sourceChildrenCollection,
        IList targetChildrenCollection,
        int insertIndex)
    {
        var sourceGroup = FindParentGroup(sourceChildrenCollection);
        var targetGroup = FindParentGroup(targetChildrenCollection);

        if (sourceGroup == null || targetGroup == null)
        {
            _logger.Warning("Could not find source or target group for cross-group move.");
            return;
        }

        int childIndex = childTask.Index;

        var sourceTaskData = ConfigFactory.CurrentConfig.TaskQueue[sourceGroup.Index] as TaskGroup;
        var targetTaskData = ConfigFactory.CurrentConfig.TaskQueue[targetGroup.Index] as TaskGroup;

        if (sourceTaskData == null || targetTaskData == null)
        {
            return;
        }

        if (childIndex < 0 || childIndex >= sourceTaskData.Children.Count)
        {
            return;
        }

        _logger.Information("Moving child task from group {Source} to group {Target}",
            sourceGroup.Name, targetGroup.Name);

        // Remove from source group
        var task = sourceGroup.RemoveChildAt(childIndex);
        if (task == null)
        {
            return;
        }

        // Add to target group
        if (insertIndex >= 0 && insertIndex < targetGroup.Children.Count)
        {
            // Insert at specific position in target
            targetTaskData.Children.Insert(insertIndex, task);
            var targetChildVm = new TaskItemViewModel(task.NameOrTaskType, task.IsEnable, taskReference: task)
            {
                Index = insertIndex,
            };
            targetChildVm.PropertyChanged += targetGroup.OnChildPropertyChanged;
            targetGroup.Children.Insert(insertIndex, targetChildVm);
        }
        else
        {
            targetGroup.AddChild(task);
        }

        RefreshGroupChildIndices(targetGroup.Children);
        RefreshGroupChildIndices(sourceGroup.Children);
    }

    /// <summary>
    /// Finds the <see cref="TaskGroupViewModel"/> whose <see cref="TaskGroupViewModel.Children"/>
    /// matches the given collection.
    /// </summary>
    private static TaskGroupViewModel? FindParentGroup(IList childrenCollection)
    {
        foreach (var item in Instances.TaskQueueViewModel.TaskItemViewModels)
        {
            if (item is TaskGroupViewModel gvm && ReferenceEquals(gvm.Children, childrenCollection))
            {
                return gvm;
            }
        }

        return null;
    }

    private static void RefreshTopLevelIndices()
    {
        for (int i = 0; i < Instances.TaskQueueViewModel.TaskItemViewModels.Count; i++)
        {
            Instances.TaskQueueViewModel.TaskItemViewModels[i].Index = i;
        }
    }

    private static void RefreshGroupChildIndices(ObservableCollection<TaskItemViewModel> children)
    {
        for (int i = 0; i < children.Count; i++)
        {
            children[i].Index = i;
        }
    }
}
