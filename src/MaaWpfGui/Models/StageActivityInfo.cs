// <copyright file="StageActivityInfo.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;

namespace MaaWpfGui.Models
{
    /// <summary>
    /// Stage activity info
    /// </summary>
    public class StageActivityInfo
    {
        /// <summary>
        /// Gets or sets the activity tip
        /// </summary>
        public string Tip { get; set; }

        /// <summary>
        /// Gets or sets the stage name
        /// </summary>
        public string StageName { get; set; }

        /// <summary>
        /// Gets or sets the activity UTC expire time
        /// </summary>
        public DateTime UtcExpireTime { get; set; }

        /// <summary>
        /// Gets or sets the activity UTC start time
        /// </summary>
        public DateTime UtcStartTime { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the activity is a resource collection activity
        /// </summary>
        /// <remarks>
        /// Sets to true indicates a resource collection activity,
        /// when the activity expires, <seealso cref="StageManager"/> will continue to check stage open days.
        /// </remarks>
        public bool IsResourceCollection { get; set; } = false;

        /// <summary>
        /// Gets a value indicating whether the activity is open or not
        /// </summary>
        public bool BeingOpen => !NotOpenYet && !IsExpired;

        /// <summary>
        /// Gets a value indicating whether the activity is expired
        /// </summary>
        public bool IsExpired => DateTime.UtcNow >= UtcExpireTime;

        /// <summary>
        /// Gets a value indicating whether the activity is expired
        /// </summary>
        public bool NotOpenYet => DateTime.UtcNow <= UtcStartTime;
    }
}
