// <copyright file="ITaskQueueItemViewModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.ViewModels;

/// <summary>
/// Interface for items that can appear in the task queue list.
/// Both <see cref="TaskItemViewModel"/> and <see cref="TaskGroupViewModel"/> implement this.
/// </summary>
public interface ITaskQueueItemViewModel
{
    string Name { get; set; }

    bool? IsEnable { get; set; }

    int Index { get; set; }

    bool EnableSetting { get; set; }

    TaskItemStatus StatusDisplay { get; set; }

    /// <summary>
    /// Gets the task IDs associated with this item.
    /// For <see cref="TaskItemViewModel"/>, returns the actual task IDs.
    /// For <see cref="TaskGroupViewModel"/>, returns an empty list (children manage their own IDs).
    /// </summary>
    IReadOnlyList<int> TaskIds { get; }

    /// <summary>
    /// Sets the task IDs for this item.
    /// </summary>
    void SetTaskIds(IEnumerable<int> taskIds);
}
