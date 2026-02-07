// <copyright file="StageInfo.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;

namespace MaaWpfGui.Models;

/// <summary>
/// Stage info
/// </summary>
public class StageInfo : CombinedData
{
    /// <summary>
    /// Gets or sets the stage tip
    /// </summary>
    public string Tip { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets the days of week when the stage is open (used for resource stages like CE-6, PR-A-1)
    /// Null or Empty means the stage is always open
    /// </summary>
    public IEnumerable<DayOfWeek>? OpenDaysOfWeek { get; set; } = null;

    /// <summary>
    /// Gets or sets the stage associated activity
    /// </summary>
    public StageActivityInfo? Activity { get; set; } = null;

    /// <summary>
    /// Gets or sets a value indicating whether the stage is hidden
    /// </summary>
    public bool IsHidden { get; set; }

    /// <summary>
    /// Gets or sets the stage drop
    /// </summary>
    public string? Drop { get; set; } = null;

    /// <summary>
    /// Gets or sets the stage drops grouped by stage (e.g. for chip stages PR-A-1 and PR-A-2)
    /// Each sub-list represents drops for one stage variant
    /// Example: [[drop1_stageA, drop2_stageA], [drop1_stageB, drop2_stageB]]
    /// </summary>
    public List<List<string>>? DropGroups { get; set; } = null;

    /// <summary>
    /// Initializes a new instance of the <see cref="StageInfo"/> class.
    /// </summary>
    public StageInfo()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StageInfo"/> class for resource stages with weekly schedule.
    /// Used for stages like CE-6 (Tuesday/Thursday/Saturday/Sunday), PR-A-1 (Monday/Thursday/Friday/Sunday), etc.
    /// </summary>
    /// <param name="name">Stage name</param>
    /// <param name="tipKey">Localization key of tip</param>
    /// <param name="openDaysOfWeek">Days of week when this stage is available</param>
    /// <param name="activity">Associated activity (typically resource collection activity)</param>
    /// <param name="dropGroups">Grouped drop items (used for chip stages with multiple variants)</param>
    public StageInfo(string name, string tipKey, IEnumerable<DayOfWeek> openDaysOfWeek, StageActivityInfo activity, List<List<string>>? dropGroups = null)
    {
        Value = name;
        Display = LocalizationHelper.GetString(name);
        OpenDaysOfWeek = openDaysOfWeek;
        Activity = activity;
        DropGroups = dropGroups;

        if (!string.IsNullOrEmpty(tipKey))
        {
            Tip = LocalizationHelper.GetString(tipKey);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="StageInfo"/> class for activity stages.
    /// </summary>
    /// <param name="display">Stage display name</param>
    /// <param name="value">Stage value</param>
    /// <param name="drop">Stage drop item</param>
    /// <param name="activity">Associated activity</param>
    public StageInfo(string display, string value, string? drop, StageActivityInfo activity)
    {
        Display = display;
        Value = value;
        Drop = drop;
        Activity = activity;
    }

    /// <summary>
    /// Determine whether the stage associated activity is closed
    /// </summary>
    /// <returns>Whether activity is closed</returns>
    public bool IsActivityClosed()
    {
        return Activity is { BeingOpen: false, IsResourceCollection: false };
    }

    /// <summary>
    /// Determine whether the stage is open
    /// </summary>
    /// <param name="dayOfWeek">Current day of week</param>
    /// <returns>Whether stage is open</returns>
    public bool IsStageOpen(DayOfWeek dayOfWeek)
    {
        if (Activity != null)
        {
            if (Activity.BeingOpen)
            {
                return true;
            }

            // expired activity
            if (!Activity.IsResourceCollection)
            {
                return false;
            }

            // expired resource activity, check open days
        }

        // resource stage
        if (OpenDaysOfWeek != null && OpenDaysOfWeek.Any())
        {
            return OpenDaysOfWeek.Contains(dayOfWeek);
        }

        // regular stage, always open
        return true;
    }

    /// <summary>
    /// Determine whether the stage is open or will open
    /// </summary>
    /// <returns>Whether stage is open</returns>
    public bool IsStageOpenOrWillOpen()
    {
        // 只有活动会过期且不开放
        if (Activity == null)
        {
            return true;
        }

        return !Activity.IsExpired ||
            Activity.IsResourceCollection;
    }
}
