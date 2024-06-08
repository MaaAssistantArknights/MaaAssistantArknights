// <copyright file="StageInfo.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;

namespace MaaWpfGui.Models
{
    /// <summary>
    /// Stage info
    /// </summary>
    public class StageInfo : CombinedData
    {
        /// <summary>
        /// Gets or sets the stage tip
        /// </summary>
        public string Tip { get; set; }

        /// <summary>
        /// Gets or sets the stage open days
        /// </summary>
        public IEnumerable<DayOfWeek> OpenDays { get; set; }

        /// <summary>
        /// Gets or sets the stage associated activity
        /// </summary>
        public StageActivityInfo Activity { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the stage is hidden
        /// </summary>
        public bool IsHidden { get; set; }

        /// <summary>
        /// Gets or sets the stage drop
        /// </summary>
        public string Drop { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="StageInfo"/> class.
        /// </summary>
        public StageInfo()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="StageInfo"/> class with tip and open days.
        /// </summary>
        /// <param name="name">Stage name</param>
        /// <param name="tipKey">Localization key of tip</param>
        /// <param name="openDays">Open days of week</param>
        /// <param name="activity">Associated activity</param>
        public StageInfo(string name, string tipKey, IEnumerable<DayOfWeek> openDays, StageActivityInfo activity)
        {
            Value = name;
            Display = LocalizationHelper.GetString(name);
            OpenDays = openDays;
            Activity = activity;

            if (!string.IsNullOrEmpty(tipKey))
            {
                Tip = LocalizationHelper.GetString(tipKey);
            }
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
            if (OpenDays != null && OpenDays.Any())
            {
                return OpenDays.Contains(dayOfWeek);
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
}
