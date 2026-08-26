// <copyright file="AutoRaiseTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// Ordered operator development plan.
/// </summary>
public class AutoRaiseTask : BaseTask
{
    public AutoRaiseTask() => TaskType = TaskType.AutoRaise;

    /// <summary>
    /// Gets or sets the editable JSON shown in the settings page.
    /// </summary>
    public string PlanJson { get; set; } = "[]";

    /// <summary>
    /// Gets or sets the last plan which passed strict validation.
    /// Editing <see cref="PlanJson"/> does not change the plan used by a run until validation succeeds again.
    /// </summary>
    public string ValidatedPlanJson { get; set; } = "[]";
}
